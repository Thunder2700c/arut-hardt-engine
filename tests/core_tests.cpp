#include "arut/core/version.hpp"
#include "arut/rendering/render_quality.hpp"

#include <cassert>
#include <string_view>

int main() {
    constexpr auto current = arut::core::version();
    static_assert(current.major == 0);
    static_assert(current.minor == 1);
    static_assert(current.patch == 0);
    assert(arut::core::name() == std::string_view{"ARUT-HARDT Engine"});
    constexpr auto low = arut::rendering::quality_config(arut::rendering::RenderQualityTier::Low);
    constexpr auto high = arut::rendering::quality_config(arut::rendering::RenderQualityTier::High);
    static_assert(low.render_scale < high.render_scale);
    static_assert(low.shadows == false);
    static_assert(high.shadow_map_resolution > low.shadow_map_resolution);
    arut::rendering::PbrMaterial material{};
    material.metallic = 2.0F;
    material.roughness = -1.0F;
    const auto safe = arut::rendering::normalized(material);
    assert(safe.metallic == 1.0F);
    assert(safe.roughness >= 0.045F);
    return 0;
}
