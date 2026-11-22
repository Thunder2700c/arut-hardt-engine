#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace arut::rendering {

enum class RenderQualityTier : std::uint8_t { Low, Medium, High, Ultra };
enum class ToneMapOperator : std::uint8_t { None, Reinhard, Aces };

struct RenderQualityConfig final {
    RenderQualityTier tier{RenderQualityTier::Medium};
    float render_scale{1.0F};
    std::uint16_t shadow_map_resolution{1024};
    std::uint8_t shadow_cascades{1};
    std::uint8_t max_local_lights{8};
    bool shadows{true};
    bool instancing{true};
    bool environment_lighting{true};
    ToneMapOperator tone_mapping{ToneMapOperator::Aces};
};

[[nodiscard]] constexpr RenderQualityConfig quality_config(RenderQualityTier tier) noexcept {
    switch (tier) {
        case RenderQualityTier::Low:
            return {tier, 0.70F, 512, 1, 4, false, true, false, ToneMapOperator::Reinhard};
        case RenderQualityTier::Medium:
            return {tier, 0.85F, 512, 1, 8, true, true, true, ToneMapOperator::Aces};
        case RenderQualityTier::High:
            return {tier, 1.00F, 1024, 2, 16, true, true, true, ToneMapOperator::Aces};
        case RenderQualityTier::Ultra:
            return {tier, 1.00F, 1536, 4, 32, true, true, true, ToneMapOperator::Aces};
    }
    return quality_config(RenderQualityTier::Medium);
}

struct PbrMaterial final {
    std::array<float, 4> base_color{1.0F, 1.0F, 1.0F, 1.0F};
    std::array<float, 3> emissive{0.0F, 0.0F, 0.0F};
    float metallic{0.0F};
    float roughness{1.0F};
    float normal_scale{1.0F};
    float occlusion_strength{1.0F};
    float alpha_cutoff{0.5F};
    std::uint32_t flags{0};
};

[[nodiscard]] inline PbrMaterial normalized(PbrMaterial material) noexcept {
    const auto finite_or = [](float value, float fallback) {
        return std::isfinite(value) ? value : fallback;
    };
    for (float& channel : material.base_color)
        channel = std::clamp(finite_or(channel, 1.0F), 0.0F, 1.0F);
    for (float& channel : material.emissive)
        channel = std::max(0.0F, finite_or(channel, 0.0F));
    material.metallic = std::clamp(finite_or(material.metallic, 0.0F), 0.0F, 1.0F);
    material.roughness = std::clamp(finite_or(material.roughness, 1.0F), 0.045F, 1.0F);
    material.normal_scale = std::max(0.0F, finite_or(material.normal_scale, 1.0F));
    material.occlusion_strength = std::clamp(finite_or(material.occlusion_strength, 1.0F), 0.0F, 1.0F);
    material.alpha_cutoff = std::clamp(finite_or(material.alpha_cutoff, 0.5F), 0.0F, 1.0F);
    return material;
}

struct RenderProfileSnapshot final {
    double cpu_frame_ms{0.0};
    double gpu_frame_ms{0.0};
    double visibility_pass_ms{0.0};
    double shadow_pass_ms{0.0};
    double main_pass_ms{0.0};
    std::uint32_t draw_calls{0};
    std::uint32_t visible_objects{0};
    std::uint32_t triangles{0};
    std::uint64_t gpu_memory_bytes{0};
    std::uint64_t gpu_memory_budget_bytes{0};
    std::uint64_t gpu_memory_usage_bytes{0};
    bool gpu_memory_budget_available{false};
    bool gpu_timing_available{false};
    bool gpu_visibility_enabled{false};
};

} // namespace arut::rendering
