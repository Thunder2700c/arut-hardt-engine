#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace arut::core {

struct SceneNodeId final {
    static constexpr std::uint32_t invalid_index = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t index{invalid_index};
    std::uint32_t generation{0};
    [[nodiscard]] constexpr bool valid() const noexcept { return index != invalid_index; }
    friend constexpr bool operator==(SceneNodeId, SceneNodeId) noexcept = default;
};

struct TransformMatrix final {
    // Column-major 4x4 matrix, matching GLSL conventions.
    std::array<float, 16> value{};
    [[nodiscard]] static constexpr TransformMatrix identity() noexcept {
        TransformMatrix result{};
        result.value[0] = result.value[5] = result.value[10] = result.value[15] = 1.0F;
        return result;
    }
};

[[nodiscard]] inline TransformMatrix multiply(const TransformMatrix& a,
                                               const TransformMatrix& b) noexcept {
    TransformMatrix result{};
    for (std::size_t column = 0; column < 4U; ++column)
        for (std::size_t row = 0; row < 4U; ++row)
            for (std::size_t k = 0; k < 4U; ++k)
                result.value[column * 4U + row] +=
                    a.value[k * 4U + row] * b.value[column * 4U + k];
    return result;
}

class SceneGraph final {
    struct Node final {
        std::uint32_t generation{0};
        bool alive{false};
        TransformMatrix local{TransformMatrix::identity()};
        std::optional<SceneNodeId> parent{};
        std::vector<SceneNodeId> children{};
    };

public:
    [[nodiscard]] SceneNodeId create(std::optional<SceneNodeId> parent = std::nullopt) {
        if (parent && !alive(*parent)) throw std::invalid_argument("invalid scene parent");
        std::uint32_t index;
        if (free_indices_.empty()) {
            if (nodes_.size() >= SceneNodeId::invalid_index)
                throw std::length_error("scene node capacity exhausted");
            index = static_cast<std::uint32_t>(nodes_.size());
            nodes_.emplace_back();
        } else {
            index = free_indices_.back();
            free_indices_.pop_back();
        }
        Node& node = nodes_[index];
        node.alive = true;
        node.local = TransformMatrix::identity();
        node.parent.reset();
        node.children.clear();
        const SceneNodeId id{index, node.generation};
        if (parent) attach_unchecked(id, *parent);
        ++alive_count_;
        return id;
    }

    [[nodiscard]] bool alive(SceneNodeId id) const noexcept {
        return id.valid() && id.index < nodes_.size() &&
               nodes_[id.index].alive && nodes_[id.index].generation == id.generation;
    }

    [[nodiscard]] bool set_parent(SceneNodeId child, std::optional<SceneNodeId> parent) {
        if (!alive(child) || (parent && !alive(*parent)) || (parent && *parent == child))
            return false;
        if (parent) {
            auto cursor = parent;
            while (cursor) {
                if (*cursor == child) return false;
                cursor = nodes_[cursor->index].parent;
            }
        }
        detach(child);
        if (parent) attach_unchecked(child, *parent);
        return true;
    }

    [[nodiscard]] bool set_local_transform(SceneNodeId id, const TransformMatrix& transform) {
        if (!alive(id)) return false;
        nodes_[id.index].local = transform;
        return true;
    }

    [[nodiscard]] const TransformMatrix* local_transform(SceneNodeId id) const noexcept {
        return alive(id) ? &nodes_[id.index].local : nullptr;
    }

    [[nodiscard]] std::optional<SceneNodeId> parent(SceneNodeId id) const noexcept {
        return alive(id) ? nodes_[id.index].parent : std::nullopt;
    }

    [[nodiscard]] const std::vector<SceneNodeId>* children(SceneNodeId id) const noexcept {
        return alive(id) ? &nodes_[id.index].children : nullptr;
    }

    [[nodiscard]] std::optional<TransformMatrix> world_transform(SceneNodeId id) const {
        if (!alive(id)) return std::nullopt;
        std::vector<SceneNodeId> chain;
        auto cursor = std::optional<SceneNodeId>{id};
        while (cursor) {
            chain.push_back(*cursor);
            cursor = nodes_[cursor->index].parent;
        }
        TransformMatrix world = TransformMatrix::identity();
        for (auto it = chain.rbegin(); it != chain.rend(); ++it)
            world = multiply(world, nodes_[it->index].local);
        return world;
    }

    // Removing a node removes its complete subtree and invalidates all its handles.
    bool destroy_subtree(SceneNodeId root) {
        if (!alive(root)) return false;
        std::vector<SceneNodeId> pending{root};
        std::vector<SceneNodeId> order;
        while (!pending.empty()) {
            const SceneNodeId current = pending.back();
            pending.pop_back();
            order.push_back(current);
            for (const auto child : nodes_[current.index].children) pending.push_back(child);
        }
        detach(root);
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            Node& node = nodes_[it->index];
            node.children.clear();
            node.parent.reset();
            node.alive = false;
            ++node.generation;
            free_indices_.push_back(it->index);
            --alive_count_;
        }
        return true;
    }

    [[nodiscard]] std::uint32_t node_count() const noexcept { return alive_count_; }

private:
    void detach(SceneNodeId child) {
        Node& node = nodes_[child.index];
        if (!node.parent) return;
        auto& siblings = nodes_[node.parent->index].children;
        for (auto it = siblings.begin(); it != siblings.end(); ++it) {
            if (*it == child) { siblings.erase(it); break; }
        }
        node.parent.reset();
    }

    void attach_unchecked(SceneNodeId child, SceneNodeId parent_id) {
        nodes_[child.index].parent = parent_id;
        nodes_[parent_id.index].children.push_back(child);
    }

    std::vector<Node> nodes_;
    std::vector<std::uint32_t> free_indices_;
    std::uint32_t alive_count_{0};
};

} // namespace arut::core
