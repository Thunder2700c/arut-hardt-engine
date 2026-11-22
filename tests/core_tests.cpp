#include "arut/core/version.hpp"
#include "arut/rendering/pbr_brdf.hpp"
#include "arut/rendering/mesh.hpp"
#include "arut/rendering/render_quality.hpp"

#include <cassert>
#include <cmath>
#include <string_view>

namespace {
bool near(float a, float b, float epsilon = 1.0e-4F) {
    return std::abs(a - b) <= epsilon;
}
}

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

    using namespace arut::rendering::pbr;
    const Vec3 f0{0.04F, 0.04F, 0.04F};
    const Vec3 normal_incidence = fresnel_schlick(1.0F, f0);
    assert(near(normal_incidence.x, 0.04F));
    const Vec3 grazing = fresnel_schlick(0.0F, f0);
    assert(near(grazing.x, 1.0F));

    const float peak = distribution_ggx(1.0F, 0.5F);
    assert(std::isfinite(peak) && peak > 0.0F);
    assert(geometry_smith(0.8F, 0.7F, 0.5F) > 0.0F);

    const Vec3 lit = evaluate_direct_brdf(
        {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F},
        {0.8F, 0.2F, 0.1F}, 0.0F, 0.5F, {2.0F, 2.0F, 2.0F});
    assert(lit.x > 0.0F && lit.y > 0.0F && lit.z > 0.0F);

    const Vec3 unlit = evaluate_direct_brdf(
        {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, -1.0F},
        {0.8F, 0.2F, 0.1F}, 0.0F, 0.5F, {2.0F, 2.0F, 2.0F});
    assert(near(unlit.x, 0.0F) && near(unlit.y, 0.0F) && near(unlit.z, 0.0F));

    const Vec3 mapped = tone_map_aces({4.0F, 1.0F, 0.25F});
    assert(mapped.x <= 1.0F && mapped.x > mapped.y && mapped.y > mapped.z);
    const auto cube = arut::rendering::make_unit_cube();
    assert(cube.vertices.size() == 24);
    assert(cube.indices.size() == 36);
    for (const auto index : cube.indices) assert(index < cube.vertices.size());
    for (const auto& vertex : cube.vertices) {
        const float normal_length = vertex.normal[0] * vertex.normal[0] +
                                    vertex.normal[1] * vertex.normal[1] +
                                    vertex.normal[2] * vertex.normal[2];
        assert(near(normal_length, 1.0F));
    }
    const auto projection = arut::rendering::perspective_rh_zo(1.04719755F, 16.0F / 9.0F, 0.1F, 100.0F);
    assert(projection.value[0] > 0.0F);
    assert(projection.value[5] < 0.0F);
    assert(near(projection.value[11], -1.0F));
    const auto view = arut::rendering::look_at_rh({0.0F, 0.0F, 3.0F},
                                                  {0.0F, 0.0F, 0.0F},
                                                  {0.0F, 1.0F, 0.0F});
    assert(near(view.value[14], -3.0F));
    const auto view_projection = arut::rendering::multiply(projection, view);
    assert(std::isfinite(view_projection.value[0]));
    assert(std::isfinite(view_projection.value[14]));
    const auto invalid_projection = arut::rendering::perspective_rh_zo(1.0F, 0.0F, 0.1F, 100.0F);
    for (const auto value : invalid_projection.value) assert(near(value, 0.0F));
    return 0;
}
