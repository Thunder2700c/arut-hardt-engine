#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <type_traits>
#include <utility>
#include <vector>

namespace arut::core {

// Handles are stable across vector growth and become invalid after destruction.
struct Entity final {
    static constexpr std::uint32_t invalid_index = std::numeric_limits<std::uint32_t>::max();
    std::uint32_t index{invalid_index};
    std::uint32_t generation{0};

    [[nodiscard]] constexpr bool valid() const noexcept { return index != invalid_index; }
    friend constexpr bool operator==(Entity, Entity) noexcept = default;
};

class World final {
    struct PoolBase {
        virtual ~PoolBase() = default;
        virtual void erase(Entity entity) = 0;
        [[nodiscard]] virtual std::type_index type() const noexcept = 0;
    };

    template <typename T>
    class Pool final : public PoolBase {
    public:
        [[nodiscard]] std::type_index type() const noexcept override { return typeid(T); }
        std::vector<Entity> entities;
        std::vector<T> values;
        std::vector<std::uint32_t> sparse;

        template <typename... Args>
        T& emplace(Entity entity, Args&&... args) {
            ensure_sparse(entity.index);
            if (contains(entity)) throw std::logic_error("entity already has component");
            const auto slot = static_cast<std::uint32_t>(values.size());
            entities.push_back(entity);
            try {
                values.emplace_back(std::forward<Args>(args)...);
            } catch (...) {
                entities.pop_back();
                throw;
            }
            sparse[entity.index] = slot + 1U;
            return values.back();
        }

        [[nodiscard]] bool contains(Entity entity) const noexcept {
            if (entity.index >= sparse.size()) return false;
            const std::uint32_t encoded = sparse[entity.index];
            return encoded != 0U && encoded <= entities.size() &&
                   entities[encoded - 1U] == entity;
        }

        T& get(Entity entity) {
            if (!contains(entity)) throw std::out_of_range("component not found");
            return values[sparse[entity.index] - 1U];
        }
        const T& get(Entity entity) const {
            if (!contains(entity)) throw std::out_of_range("component not found");
            return values[sparse[entity.index] - 1U];
        }

        void erase(Entity entity) override {
            if (!contains(entity)) return;
            const std::uint32_t slot = sparse[entity.index] - 1U;
            const std::uint32_t last = static_cast<std::uint32_t>(values.size() - 1U);
            if (slot != last) {
                entities[slot] = entities[last];
                values[slot] = std::move(values[last]);
                sparse[entities[slot].index] = slot + 1U;
            }
            entities.pop_back();
            values.pop_back();
            sparse[entity.index] = 0U;
        }

        bool remove(Entity entity) {
            if (!contains(entity)) return false;
            erase(entity);
            return true;
        }

    private:
        void ensure_sparse(std::uint32_t index) {
            if (index >= sparse.size()) sparse.resize(static_cast<std::size_t>(index) + 1U, 0U);
        }
    };

public:
    World() = default;
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) noexcept = default;
    World& operator=(World&&) noexcept = default;

    [[nodiscard]] Entity create() {
        std::uint32_t index;
        if (!free_indices_.empty()) {
            index = free_indices_.back();
            free_indices_.pop_back();
        } else {
            if (generations_.size() >= Entity::invalid_index)
                throw std::length_error("entity index capacity exhausted");
            index = static_cast<std::uint32_t>(generations_.size());
            generations_.push_back(0U);
            is_allocated_.push_back(0U);
        }
        is_allocated_[index] = 1U;
        ++alive_count_;
        return {index, generations_[index]};
    }

    bool destroy(Entity entity) {
        if (!alive(entity)) return false;
        for (auto& pool : pools_) if (pool) pool->erase(entity);
        ++generations_[entity.index];
        is_allocated_[entity.index] = 0U;
        free_indices_.push_back(entity.index);
        --alive_count_;
        return true;
    }

    [[nodiscard]] bool alive(Entity entity) const noexcept {
        return entity.valid() && entity.index < generations_.size() &&
               generations_[entity.index] == entity.generation &&
               is_allocated_[entity.index] != 0U;
    }

    template <typename T, typename... Args>
    T& emplace(Entity entity, Args&&... args) {
        require_alive(entity);
        return pool<T>().emplace(entity, std::forward<Args>(args)...);
    }

    template <typename T>
    [[nodiscard]] bool has(Entity entity) const noexcept {
        if (!alive(entity)) return false;
        const auto* storage = find_pool<T>();
        return storage && storage->contains(entity);
    }

    template <typename T>
    T& get(Entity entity) {
        require_alive(entity);
        return pool<T>().get(entity);
    }

    template <typename T>
    const T& get(Entity entity) const {
        require_alive(entity);
        const auto* storage = find_pool<T>();
        if (!storage) throw std::out_of_range("component pool not found");
        return storage->get(entity);
    }

    template <typename T>
    bool remove(Entity entity) {
        if (!alive(entity)) return false;
        auto* storage = find_pool<T>();
        return storage && storage->remove(entity);
    }

    [[nodiscard]] std::uint32_t entity_count() const noexcept { return alive_count_; }

private:
    void require_alive(Entity entity) const {
        if (!alive(entity)) throw std::invalid_argument("stale or invalid entity handle");
    }

    template <typename T>
    Pool<T>& pool() {
        static_assert(std::is_move_assignable_v<T>,
                      "ECS components must be move-assignable for packed storage");
        const std::type_index key{typeid(T)};
        for (auto& entry : pools_) if (entry && entry_type(entry.get()) == key)
            return *static_cast<Pool<T>*>(entry.get());
        auto created = std::make_unique<Pool<T>>();
        auto* result = created.get();
        pools_.push_back(std::move(created));
        return *result;
    }

    template <typename T>
    const Pool<T>* find_pool() const noexcept {
        const std::type_index key{typeid(T)};
        for (const auto& entry : pools_) if (entry && entry_type(entry.get()) == key)
            return static_cast<const Pool<T>*>(entry.get());
        return nullptr;
    }

    template <typename T>
    Pool<T>* find_pool() noexcept {
        return const_cast<Pool<T>*>(static_cast<const World*>(this)->find_pool<T>());
    }

    static std::type_index entry_type(const PoolBase* entry) noexcept {
        return entry->type();
    }

    std::vector<std::unique_ptr<PoolBase>> pools_;
    std::vector<std::uint32_t> generations_;
    std::vector<std::uint8_t> is_allocated_;
    std::vector<std::uint32_t> free_indices_;
    std::uint32_t alive_count_{0};
};

} // namespace arut::core
