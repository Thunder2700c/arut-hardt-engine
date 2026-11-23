#pragma once

#include "arut/core/ecs.hpp"
#include "arut/core/scene_graph_ecs.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace arut::core {

struct CubeTransformComponent final {
    std::array<float, 3> position{};
    float scale{1.0F};
    float yaw{0.0F};
};

struct CubeMeshComponent final {
    std::uint32_t mesh_id{0U};
    std::uint32_t material_id{0U};
};

struct CubeBoundsComponent final {
    float radius{0.0F};
};

// Renderer-neutral instance packet extracted from ECS component data.
struct alignas(16) CubeRenderInstance final {
    std::array<float, 4> bounds{};
    std::array<float, 16> model{};
};

class CubeSpawnSystem final : public System {
public:
    static constexpr std::uint32_t grid_x = 20U;
    static constexpr std::uint32_t grid_y = 10U;
    static constexpr std::uint32_t grid_z = 25U;
    static constexpr std::uint32_t target_count = grid_x * grid_y * grid_z;
    static constexpr float spacing = 2.0F;

    void update(World& world) override {
        if (spawned_) return;

        entities_.clear();
        entities_.reserve(target_count);
        try {
            for (std::uint32_t iy = 0; iy < grid_y; ++iy) {
                for (std::uint32_t iz = 0; iz < grid_z; ++iz) {
                    for (std::uint32_t ix = 0; ix < grid_x; ++ix) {
                        const float x = (static_cast<float>(ix) -
                            static_cast<float>(grid_x - 1U) * 0.5F) * spacing;
                        const float y = (static_cast<float>(iy) -
                            static_cast<float>(grid_y - 1U) * 0.5F) * spacing;
                        const float z = (static_cast<float>(iz) -
                            static_cast<float>(grid_z - 1U) * 0.5F) * spacing;
                        const float variation = static_cast<float>(
                            (ix * 17U + iy * 31U + iz * 13U) % 7U);
                        const float scale = 0.34F + variation * 0.025F;
                        const float yaw = static_cast<float>((ix + iz * 3U) % 16U) *
                            0.196349541F;

                        const Entity entity = world.create();
                        entities_.push_back(entity);
                        world.emplace<CubeTransformComponent>(
                            entity, CubeTransformComponent{{x, y, z}, scale, yaw});
                        SceneLocalTransformComponent scene_transform{};
                        scene_transform.value = TransformMatrix::identity();
                        const float cosine = std::cos(yaw);
                        const float sine = std::sin(yaw);
                        scene_transform.value.value[0] = cosine * scale;
                        scene_transform.value.value[2] = -sine * scale;
                        scene_transform.value.value[5] = scale;
                        scene_transform.value.value[8] = sine * scale;
                        scene_transform.value.value[10] = cosine * scale;
                        scene_transform.value.value[12] = x;
                        scene_transform.value.value[13] = y;
                        scene_transform.value.value[14] = z;
                        world.emplace<SceneLocalTransformComponent>(entity, scene_transform);
                        world.emplace<CubeMeshComponent>(
                            entity, CubeMeshComponent{0U, 0U});
                        world.emplace<CubeBoundsComponent>(
                            entity, CubeBoundsComponent{scale * 0.866025404F});
                    }
                }
            }
        } catch (...) {
            for (auto it = entities_.rbegin(); it != entities_.rend(); ++it)
                (void)world.destroy(*it);
            entities_.clear();
            throw;
        }

        if (entities_.size() != target_count) {
            for (auto it = entities_.rbegin(); it != entities_.rend(); ++it)
                (void)world.destroy(*it);
            entities_.clear();
            throw std::logic_error("CubeSpawnSystem did not create the requested entity count");
        }
        spawned_ = true;
    }

    [[nodiscard]] std::size_t spawned_count() const noexcept { return entities_.size(); }
    [[nodiscard]] bool has_spawned() const noexcept { return spawned_; }
    [[nodiscard]] const std::vector<Entity>& entities() const noexcept { return entities_; }

private:
    std::vector<Entity> entities_{};
    bool spawned_{false};
};

class CubeRenderExtractionSystem final : public System {
public:
    void update(World& world) override { update(world, nullptr); }

    void update(World& world, const SceneGraphSystem* scene_graph) {
        instances_.clear();
        instances_.reserve(world.entity_count());
        extracted_count_ = world.each<CubeTransformComponent, CubeMeshComponent,
                                       CubeBoundsComponent>(
            [this, &world, scene_graph](Entity entity, const CubeTransformComponent& transform,
                   const CubeMeshComponent&, const CubeBoundsComponent& bounds) {
                constexpr float identity_diagonal = 1.0F;
                CubeRenderInstance instance{};
                const float cosine = std::cos(transform.yaw);
                const float sine = std::sin(transform.yaw);
                bool used_scene_graph = false;
                if (scene_graph && world.has<SceneGraphBindingComponent>(entity)) {
                    const auto binding = world.get<SceneGraphBindingComponent>(entity);
                    const auto world_transform = scene_graph->graph().world_transform(binding.node);
                    if (world_transform) {
                        instance.model = world_transform->value;
                        used_scene_graph = true;
                    }
                }
                if (!used_scene_graph) {
                    instance.model[0] = cosine * transform.scale;
                    instance.model[2] = -sine * transform.scale;
                    instance.model[5] = transform.scale;
                    instance.model[8] = sine * transform.scale;
                    instance.model[10] = cosine * transform.scale;
                    instance.model[15] = identity_diagonal;
                    instance.model[12] = transform.position[0];
                    instance.model[13] = transform.position[1];
                    instance.model[14] = transform.position[2];
                }
                instance.bounds = {transform.position[0], transform.position[1],
                                   transform.position[2], bounds.radius};
                instances_.push_back(instance);
            });
        if (extracted_count_ != instances_.size())
            throw std::logic_error("Cube render extraction count mismatch");
    }

    [[nodiscard]] std::size_t extracted_count() const noexcept { return extracted_count_; }
    [[nodiscard]] const std::vector<CubeRenderInstance>& instances() const noexcept {
        return instances_;
    }

private:
    std::vector<CubeRenderInstance> instances_{};
    std::size_t extracted_count_{0U};
};

} // namespace arut::core
