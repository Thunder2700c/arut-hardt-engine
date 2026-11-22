#pragma once

#include "arut/core/ecs.hpp"
#include "arut/core/scene_graph.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace arut::core {

// Entities carrying a local transform participate in the scene hierarchy.
struct SceneLocalTransformComponent final {
    TransformMatrix value{TransformMatrix::identity()};
};

// Optional parent relationship. The parent must also carry a local transform.
struct SceneParentComponent final {
    Entity parent{};
};

struct SceneGraphBindingComponent final {
    SceneNodeId node{};
};

struct EntityHandleHash final {
    [[nodiscard]] std::size_t operator()(Entity entity) const noexcept {
        const auto combined = (static_cast<std::uint64_t>(entity.generation) << 32U) |
                              static_cast<std::uint64_t>(entity.index);
        return static_cast<std::size_t>(combined ^ (combined >> 33U));
    }
};

// Synchronizes ECS transform/parent components into one authoritative scene graph.
// ECS owns entity lifetime; SceneGraphSystem owns node lifetime and hierarchy.
class SceneGraphSystem final : public System {
public:
    void update(World& world) override {
        // Remove nodes whose owning ECS entities have been destroyed.
        for (auto it = entities_.begin(); it != entities_.end();) {
            const Entity entity = it->first;
            const bool no_longer_in_scene =
                !world.alive(entity) || !world.has<SceneLocalTransformComponent>(entity);
            if (no_longer_in_scene || !graph_.alive(it->second)) {
                if (graph_.alive(it->second)) (void)graph_.destroy_subtree(it->second);
                if (world.alive(entity))
                    (void)world.remove<SceneGraphBindingComponent>(entity);
                it = entities_.erase(it);
            } else {
                ++it;
            }
        }

        std::vector<Entity> transformed_entities;
        transformed_entities.reserve(world.entity_count());
        world.each<SceneLocalTransformComponent>(
            [&](Entity entity, const SceneLocalTransformComponent&) {
                transformed_entities.push_back(entity);
                auto found = entities_.find(entity);
                if (found == entities_.end() || !graph_.alive(found->second)) {
                    const SceneNodeId node = graph_.create();
                    entities_[entity] = node;
                    if (world.has<SceneGraphBindingComponent>(entity))
                        world.get<SceneGraphBindingComponent>(entity).node = node;
                    else
                        world.emplace<SceneGraphBindingComponent>(
                            entity, SceneGraphBindingComponent{node});
                }
            });

        // Apply local transforms only after all nodes exist, so parent order is irrelevant.
        for (const Entity entity : transformed_entities) {
            const auto node = entities_.at(entity);
            const auto& transform = world.get<SceneLocalTransformComponent>(entity).value;
            if (!graph_.set_local_transform(node, transform))
                throw std::logic_error("SceneGraphSystem failed to set local transform");
        }

        // First detach roots/children with no parent, then attach requested relationships.
        for (const Entity entity : transformed_entities) {
            if (world.has<SceneParentComponent>(entity)) continue;
            if (!graph_.set_parent(entities_.at(entity), std::nullopt))
                throw std::logic_error("SceneGraphSystem failed to detach scene node");
        }
        for (const Entity entity : transformed_entities) {
            if (!world.has<SceneParentComponent>(entity)) continue;
            const Entity parent = world.get<SceneParentComponent>(entity).parent;
            if (!world.alive(parent))
                throw std::invalid_argument("SceneGraphSystem parent entity is stale");
            const auto parent_node = entities_.find(parent);
            if (parent_node == entities_.end() || !graph_.alive(parent_node->second))
                throw std::invalid_argument("SceneGraphSystem parent has no scene node");
            if (!graph_.set_parent(entities_.at(entity), parent_node->second))
                throw std::logic_error("SceneGraphSystem rejected an invalid or cyclic parent");
        }
    }

    [[nodiscard]] const SceneGraph& graph() const noexcept { return graph_; }
    [[nodiscard]] SceneGraph& graph() noexcept { return graph_; }
    [[nodiscard]] std::size_t bound_entity_count() const noexcept { return entities_.size(); }

private:
    SceneGraph graph_{};
    std::unordered_map<Entity, SceneNodeId, EntityHandleHash> entities_{};
};

} // namespace arut::core
