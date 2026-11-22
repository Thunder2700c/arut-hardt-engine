#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

namespace arut::core {

struct TerrainVertex final {
    float x{0}, y{0}, z{0};
    float nx{0}, ny{1}, nz{0};
};

struct TerrainMesh final {
    std::uint32_t resolution{0};
    float world_size{0};
    std::vector<TerrainVertex> vertices;
    std::vector<std::uint32_t> indices;
    [[nodiscard]] bool valid() const noexcept {
        const std::size_t side = static_cast<std::size_t>(resolution) + 1U;
        return resolution >= 2U && world_size > 0.0F &&
            vertices.size() == side * side &&
            indices.size() == static_cast<std::size_t>(resolution) * resolution * 6U;
    }
};

class TerrainSystem final {
public:
    // Deterministic height field: broad rolling hills plus a small detail octave.
    [[nodiscard]] static float height(float x, float z) noexcept {
        return 2.8F * std::sin(x * 0.035F) * std::cos(z * 0.029F) +
               0.65F * std::sin((x + z) * 0.11F);
    }

    [[nodiscard]] static TerrainMesh build(float world_size, std::uint32_t resolution) {
        if (!std::isfinite(world_size) || world_size <= 0.0F ||
            resolution < 2U || resolution > 2048U)
            throw std::invalid_argument("invalid terrain dimensions");
        TerrainMesh mesh{};
        mesh.resolution = resolution;
        mesh.world_size = world_size;
        const std::size_t side = static_cast<std::size_t>(resolution) + 1U;
        mesh.vertices.reserve(side * side);
        mesh.indices.reserve(static_cast<std::size_t>(resolution) * resolution * 6U);
        const float half = world_size * 0.5F;
        const float step = world_size / static_cast<float>(resolution);
        for (std::uint32_t z = 0; z <= resolution; ++z) {
            for (std::uint32_t x = 0; x <= resolution; ++x) {
                const float px = -half + static_cast<float>(x) * step;
                const float pz = -half + static_cast<float>(z) * step;
                constexpr float e = 0.1F;
                const float dx = (height(px + e, pz) - height(px - e, pz)) / (2.0F * e);
                const float dz = (height(px, pz + e) - height(px, pz - e)) / (2.0F * e);
                const float inv = 1.0F / std::sqrt(dx * dx + 1.0F + dz * dz);
                mesh.vertices.push_back({px, height(px, pz), pz, -dx * inv, inv, -dz * inv});
            }
        }
        for (std::uint32_t z = 0; z < resolution; ++z) {
            for (std::uint32_t x = 0; x < resolution; ++x) {
                const std::uint32_t a = z * (resolution + 1U) + x;
                const std::uint32_t b = a + 1U;
                const std::uint32_t c = a + resolution + 1U;
                const std::uint32_t d = c + 1U;
                mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
            }
        }
        return mesh;
    }
};

enum class TerrainLod : std::uint8_t { High = 0, Medium = 1, Low = 2, Impostor = 3 };

class LodSystem final {
public:
    struct Thresholds final {
        float high_to_medium{45.0F};
        float medium_to_low{100.0F};
        float low_to_impostor{220.0F};
        float hysteresis{8.0F};
    };

    LodSystem() : LodSystem(Thresholds{}) {}

    explicit LodSystem(Thresholds thresholds) : thresholds_(thresholds) {
        if (!(thresholds_.high_to_medium > 0.0F &&
              thresholds_.medium_to_low > thresholds_.high_to_medium &&
              thresholds_.low_to_impostor > thresholds_.medium_to_low &&
              thresholds_.hysteresis >= 0.0F))
            throw std::invalid_argument("LOD thresholds must be strictly increasing");
    }

    [[nodiscard]] TerrainLod select(std::uint64_t object_id, float distance) {
        if (!std::isfinite(distance) || distance < 0.0F)
            throw std::invalid_argument("LOD distance must be finite and non-negative");
        const auto found = levels_.find(object_id);
        TerrainLod level = found == levels_.end() ? TerrainLod::High : found->second;
        const float h = thresholds_.hysteresis;
        if (level == TerrainLod::High && distance > thresholds_.high_to_medium + h)
            level = TerrainLod::Medium;
        if (level == TerrainLod::Medium) {
            if (distance < thresholds_.high_to_medium - h) level = TerrainLod::High;
            else if (distance > thresholds_.medium_to_low + h) level = TerrainLod::Low;
        }
        if (level == TerrainLod::Low) {
            if (distance < thresholds_.medium_to_low - h) level = TerrainLod::Medium;
            else if (distance > thresholds_.low_to_impostor + h) level = TerrainLod::Impostor;
        }
        if (level == TerrainLod::Impostor && distance < thresholds_.low_to_impostor - h)
            level = TerrainLod::Low;
        levels_[object_id] = level;
        return level;
    }

    void forget(std::uint64_t object_id) noexcept { levels_.erase(object_id); }
    [[nodiscard]] std::size_t tracked_count() const noexcept { return levels_.size(); }

private:
    Thresholds thresholds_;
    std::unordered_map<std::uint64_t, TerrainLod> levels_;
};

struct WorldCell final {
    std::int32_t x{0}, z{0};
    friend bool operator==(WorldCell, WorldCell) noexcept = default;
};

struct WorldCellHash final {
    [[nodiscard]] std::size_t operator()(WorldCell cell) const noexcept {
        const auto x = static_cast<std::uint32_t>(cell.x);
        const auto z = static_cast<std::uint32_t>(cell.z);
        return std::hash<std::uint32_t>{}(x) ^ (std::hash<std::uint32_t>{}(z) + 0x9e3779b9U + (static_cast<std::size_t>(x) << 6U) + (static_cast<std::size_t>(x) >> 2U));
    }
};

class WorldPartition final {
public:
    explicit WorldPartition(float cell_size = 128.0F) : cell_size_(cell_size) {
        if (!std::isfinite(cell_size_) || cell_size_ <= 0.0F)
            throw std::invalid_argument("world cell size must be positive");
    }

    [[nodiscard]] WorldCell cell_at(float x, float z) const {
        if (!std::isfinite(x) || !std::isfinite(z))
            throw std::invalid_argument("world position must be finite");
        const double cell_x = std::floor(static_cast<double>(x) / cell_size_);
        const double cell_z = std::floor(static_cast<double>(z) / cell_size_);
        if (cell_x < std::numeric_limits<std::int32_t>::min() ||
            cell_x > std::numeric_limits<std::int32_t>::max() ||
            cell_z < std::numeric_limits<std::int32_t>::min() ||
            cell_z > std::numeric_limits<std::int32_t>::max())
            throw std::out_of_range("world position exceeds partition coordinate range");
        return {static_cast<std::int32_t>(cell_x), static_cast<std::int32_t>(cell_z)};
    }

    // Computes the desired square streaming window. Returns newly activated cells;
    // cells outside the window are evicted from the active set.
    [[nodiscard]] std::vector<WorldCell> update_streaming(
        float center_x, float center_z, std::uint32_t radius) {
        const WorldCell center = cell_at(center_x, center_z);
        std::unordered_set<WorldCell, WorldCellHash> desired;
        const std::int64_t r = radius;
        const std::int64_t count = (2 * r + 1) * (2 * r + 1);
        if (count > 1000000) throw std::length_error("streaming window is too large");
        desired.reserve(static_cast<std::size_t>(count));
        for (std::int64_t z = -r; z <= r; ++z) {
            for (std::int64_t x = -r; x <= r; ++x) {
                const std::int64_t cell_x = static_cast<std::int64_t>(center.x) + x;
                const std::int64_t cell_z = static_cast<std::int64_t>(center.z) + z;
                if (cell_x < std::numeric_limits<std::int32_t>::min() ||
                    cell_x > std::numeric_limits<std::int32_t>::max() ||
                    cell_z < std::numeric_limits<std::int32_t>::min() ||
                    cell_z > std::numeric_limits<std::int32_t>::max())
                    throw std::out_of_range("streaming window exceeds partition coordinate range");
                desired.insert({static_cast<std::int32_t>(cell_x),
                                static_cast<std::int32_t>(cell_z)});
            }
        }
        std::vector<WorldCell> activated;
        for (const auto cell : desired)
            if (active_.insert(cell).second) activated.push_back(cell);
        for (auto it = active_.begin(); it != active_.end();) {
            if (!desired.contains(*it)) it = active_.erase(it);
            else ++it;
        }
        return activated;
    }

    [[nodiscard]] bool active(WorldCell cell) const noexcept { return active_.contains(cell); }
    [[nodiscard]] std::size_t active_count() const noexcept { return active_.size(); }
    [[nodiscard]] std::vector<WorldCell> active_cells() const {
        std::vector<WorldCell> cells;
        cells.reserve(active_.size());
        for (const auto cell : active_) cells.push_back(cell);
        std::sort(cells.begin(), cells.end(), [](WorldCell a, WorldCell b) {
            return a.z == b.z ? a.x < b.x : a.z < b.z;
        });
        return cells;
    }
    [[nodiscard]] float cell_size() const noexcept { return cell_size_; }

private:
    float cell_size_;
    std::unordered_set<WorldCell, WorldCellHash> active_;
};

struct WorldCellContent final {
    WorldCell cell{};
    TerrainMesh terrain{};
};

struct WorldStreamingReport final {
    std::size_t loaded{0};
    std::size_t unloaded{0};
    std::size_t failed{0};
    std::size_t resident{0};
};

// Owns actual cell payloads, not just the partition's active-cell keys.
// The loader/unloader callbacks keep storage policy outside the partition.
class WorldCellStreamer final {
public:
    using Loader = std::function<bool(WorldCell, WorldCellContent&)>;
    using Unloader = std::function<void(WorldCell, WorldCellContent&)>;

    [[nodiscard]] WorldStreamingReport update(
        WorldPartition& partition, float center_x, float center_z,
        std::uint32_t radius, const Loader& loader, const Unloader& unloader = {}) {
        if (!loader) throw std::invalid_argument("world cell loader is required");
        (void)partition.update_streaming(center_x, center_z, radius);
        const auto active = partition.active_cells();
        WorldStreamingReport report{};
        for (const auto cell : active) {
            if (resident_.contains(cell)) continue;
            WorldCellContent content{};
            content.cell = cell;
            bool loaded = false;
            try { loaded = loader(cell, content) && content.cell == cell && content.terrain.valid(); }
            catch (...) { loaded = false; }
            if (!loaded) { ++report.failed; continue; }
            resident_.emplace(cell, std::move(content));
            ++report.loaded;
        }
        for (auto it = resident_.begin(); it != resident_.end();) {
            if (partition.active(it->first)) { ++it; continue; }
            if (unloader) {
                try { unloader(it->first, it->second); }
                catch (...) { /* residency cleanup must continue */ }
            }
            it = resident_.erase(it);
            ++report.unloaded;
        }
        report.resident = resident_.size();
        return report;
    }

    [[nodiscard]] const WorldCellContent* find(WorldCell cell) const noexcept {
        const auto found = resident_.find(cell);
        return found == resident_.end() ? nullptr : &found->second;
    }
    [[nodiscard]] std::size_t resident_count() const noexcept { return resident_.size(); }

private:
    std::unordered_map<WorldCell, WorldCellContent, WorldCellHash> resident_;
};

} // namespace arut::core
