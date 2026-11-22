#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <vector>

namespace arut::rendering {

struct EnvironmentProbeAabb final {
    float min_x{0.0F}, min_y{0.0F}, min_z{0.0F};
    float max_x{0.0F}, max_y{0.0F}, max_z{0.0F};
};

struct EnvironmentProbeInfluence final {
    EnvironmentProbeAabb bounds{};
    float blend_distance{0.0F};
    std::int32_t priority{0};
};

// Returns normalized influence weights in input order. Only probes at the
// highest priority with non-zero influence participate in the blend.
[[nodiscard]] inline bool evaluate_environment_probe_weights(
    std::span<const EnvironmentProbeInfluence> probes,
    float x, float y, float z, std::vector<float>& weights) noexcept {
    weights.clear();
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
    try {
        weights.assign(probes.size(), 0.0F);
        std::int32_t highest_priority = 0;
        bool has_influence = false;
        float total = 0.0F;
        for (std::size_t i = 0; i < probes.size(); ++i) {
            const auto& probe = probes[i];
            const auto& b = probe.bounds;
            if (!std::isfinite(b.min_x) || !std::isfinite(b.min_y) ||
                !std::isfinite(b.min_z) || !std::isfinite(b.max_x) ||
                !std::isfinite(b.max_y) || !std::isfinite(b.max_z) ||
                b.min_x > b.max_x || b.min_y > b.max_y || b.min_z > b.max_z ||
                !std::isfinite(probe.blend_distance)) continue;

            const float dx = std::max({b.min_x - x, 0.0F, x - b.max_x});
            const float dy = std::max({b.min_y - y, 0.0F, y - b.max_y});
            const float dz = std::max({b.min_z - z, 0.0F, z - b.max_z});
            const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
            float weight = 0.0F;
            if (distance <= 0.0F) {
                weight = 1.0F;
            } else if (probe.blend_distance > 0.0F &&
                       distance < probe.blend_distance) {
                const float t = 1.0F - distance / probe.blend_distance;
                weight = t * t * (3.0F - 2.0F * t);
            }
            if (!(weight > 0.0F)) continue;
            if (!has_influence || probe.priority > highest_priority) {
                highest_priority = probe.priority;
                has_influence = true;
                total = 0.0F;
                std::fill(weights.begin(), weights.end(), 0.0F);
            }
            if (probe.priority == highest_priority) {
                weights[i] = weight;
                total += weight;
            }
        }
        if (total > 0.0F) {
            for (float& weight : weights) weight /= total;
        }
    } catch (...) {
        weights.clear();
        return false;
    }
    return true;
}

} // namespace arut::rendering
