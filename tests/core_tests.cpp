#include "arut/core/version.hpp"
#include "arut/rendering/pbr_brdf.hpp"
#include "arut/rendering/mesh.hpp"
#include "arut/rendering/image.hpp"
#include "arut/rendering/environment.hpp"
#include "arut/rendering/brdf_lut.hpp"
#include "arut/rendering/visibility.hpp"
#include "arut/rendering/shadow.hpp"
#include "arut/rendering/render_quality.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <cassert>
#include <cmath>
#include <string_view>
#include <vector>

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
    constexpr auto medium = arut::rendering::quality_config(arut::rendering::RenderQualityTier::Medium);
    constexpr auto high = arut::rendering::quality_config(arut::rendering::RenderQualityTier::High);
    constexpr auto ultra = arut::rendering::quality_config(arut::rendering::RenderQualityTier::Ultra);
    static_assert(low.render_scale < high.render_scale);
    static_assert(low.shadows == false);
    static_assert(medium.shadow_map_resolution == 512);
    static_assert(medium.shadow_cascades == 1);
    static_assert(high.shadow_map_resolution == 1024 && high.shadow_cascades == 2);
    static_assert(ultra.shadow_map_resolution == 1536 && ultra.shadow_cascades == 4);

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

    constexpr int polar_steps = 48;
    constexpr int azimuth_steps = 96;
    constexpr float pi = 3.14159265358979323846F;
    const Vec3 albedo{0.78F, 0.42F, 0.18F};
    for (const float metallic : {0.0F, 0.5F, 1.0F}) {
        for (const float roughness : {0.10F, 0.50F, 0.90F}) {
            Vec3 integrated{};
            for (int polar = 0; polar < polar_steps; ++polar) {
                const float z = (static_cast<float>(polar) + 0.5F) /
                                static_cast<float>(polar_steps);
                const float radial = std::sqrt(std::max(0.0F, 1.0F - z * z));
                for (int azimuth = 0; azimuth < azimuth_steps; ++azimuth) {
                    const float phi = 2.0F * pi *
                        (static_cast<float>(azimuth) + 0.5F) /
                        static_cast<float>(azimuth_steps);
                    const Vec3 light_direction{radial * std::cos(phi), radial * std::sin(phi), z};
                    const Vec3 response = evaluate_direct_brdf(
                        {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, light_direction,
                        albedo, metallic, roughness, {1.0F, 1.0F, 1.0F});
                    integrated = integrated + response;
                }
            }
            const float solid_angle_per_sample =
                (2.0F * pi) / static_cast<float>(polar_steps * azimuth_steps);
            integrated = integrated * solid_angle_per_sample;
            assert(integrated.x >= 0.0F && integrated.x <= 1.05F);
            assert(integrated.y >= 0.0F && integrated.y <= 1.05F);
            assert(integrated.z >= 0.0F && integrated.z <= 1.05F);
        }
    }

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
    constexpr std::array<std::uint8_t, 75> png_2x2{{
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
        0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
        0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02,
        0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d,
        0x24, 0x00, 0x00, 0x00, 0x12, 0x49, 0x44, 0x41,
        0x54, 0x78, 0x9c, 0x63, 0xf8, 0xcf, 0xc0, 0xf0,
        0x1f, 0x0c, 0x81, 0x34, 0x18, 0x00, 0x00, 0x49,
        0xc8, 0x09, 0xf7, 0xf9, 0xab, 0xb6, 0x0d, 0x00,
        0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
        0x42, 0x60, 0x82
    }};
    arut::rendering::Rgba8Image decoded{};
    const auto decode_result = arut::rendering::decode_image_rgba8(png_2x2, decoded);
    assert(decode_result && decoded.width == 2 && decoded.height == 2);
    assert(decoded.pixels.size() == 16);
    std::vector<arut::rendering::Rgba8Mip> srgb_mips;
    assert(arut::rendering::build_rgba8_mip_chain(decoded, true, srgb_mips));
    assert(srgb_mips.size() == 2 && srgb_mips[1].width == 1 && srgb_mips[1].height == 1);
    assert(srgb_mips[1].pixels[0] >= 187 && srgb_mips[1].pixels[0] <= 189);
    constexpr std::array<std::uint8_t, 34> normal_tga{{
        0,0,2,0,0,0,0,0,0,0,0,0,2,0,2,0,32,40,
        255,128,128,255,255,128,128,255,255,128,128,255,255,128,128,255
    }};
    arut::rendering::Rgba8Image decoded_normal{};
    assert(arut::rendering::decode_image_rgba8(normal_tga, decoded_normal));
    assert(decoded_normal.width == 2 && decoded_normal.height == 2);
    assert(decoded_normal.pixels[0] == 128 && decoded_normal.pixels[1] == 128 &&
           decoded_normal.pixels[2] == 255 && decoded_normal.pixels[3] == 255);

    constexpr std::array<std::uint8_t, 34> metallic_roughness_tga{{
        0,0,2,0,0,0,0,0,0,0,0,0,2,0,2,0,32,40,
        64,180,0,255,64,180,0,255,64,180,0,255,64,180,0,255
    }};
    arut::rendering::Rgba8Image decoded_mr{};
    assert(arut::rendering::decode_image_rgba8(metallic_roughness_tga, decoded_mr));
    assert(decoded_mr.pixels[0] == 0 && decoded_mr.pixels[1] == 180 &&
           decoded_mr.pixels[2] == 64 && decoded_mr.pixels[3] == 255);

    std::vector<arut::rendering::Rgba8Mip> linear_mips;
    assert(arut::rendering::build_rgba8_mip_chain(decoded, false, linear_mips));
    assert(linear_mips[1].pixels[0] >= 127 && linear_mips[1].pixels[0] <= 128);
    arut::rendering::Rgba8Image brdf_lut{};
    assert(arut::rendering::build_brdf_integration_lut(16, 32, brdf_lut));
    assert(brdf_lut.width == 16 && brdf_lut.height == 16 && brdf_lut.valid());
    for (std::size_t i = 0; i < brdf_lut.pixels.size(); i += 4U) {
        assert(brdf_lut.pixels[i] <= 255U);
        assert(brdf_lut.pixels[i + 1U] <= 255U);
        assert(brdf_lut.pixels[i + 3U] == 255U);
    }
    arut::rendering::Rgba8Image invalid_brdf_lut{};
    assert(!arut::rendering::build_brdf_integration_lut(0, 32, invalid_brdf_lut));
    assert(invalid_brdf_lut.pixels.empty());

    std::vector<arut::rendering::EnvironmentCubeMip> environment_mips;
    assert(arut::rendering::build_procedural_environment_cube(8, environment_mips));
    assert(environment_mips.size() == 4);
    assert(environment_mips[0].width == 8 && environment_mips[0].height == 8);
    assert(environment_mips[0].pixels.size() == 8U * 8U * 4U * 6U);
    assert(environment_mips[3].width == 1 && environment_mips[3].height == 1);
    assert(environment_mips[3].pixels.size() == 4U * 6U);
    arut::rendering::EnvironmentCubeMip irradiance{};
    assert(arut::rendering::build_diffuse_irradiance_cube(environment_mips.front(), 4U, 64U, irradiance));
    assert(irradiance.width == 4U && irradiance.height == 4U);
    assert(irradiance.pixels.size() == 4U * 4U * 4U * 6U);
    assert(irradiance.pixels[3] == 255U);
    assert(irradiance.pixels[(2U * 4U * 4U * 4U) + 3U] == 255U);
    arut::rendering::EnvironmentCubeMip invalid_irradiance{};
    assert(!arut::rendering::build_diffuse_irradiance_cube(environment_mips.front(), 4U, 4U, invalid_irradiance));
    assert(invalid_irradiance.pixels.empty());
    assert(!arut::rendering::build_procedural_environment_cube(0, environment_mips));
    assert(environment_mips.empty());
    assert(!arut::rendering::build_procedural_environment_cube(512, environment_mips));
    arut::rendering::Rgba8Image invalid_image{};
    assert(!arut::rendering::decode_image_rgba8({}, invalid_image));
    assert(invalid_image.pixels.empty());
    const auto cull_view = arut::rendering::look_at_rh({0.0F, 0.0F, 3.0F},
                                                       {0.0F, 0.0F, 0.0F},
                                                       {0.0F, 1.0F, 0.0F});
    const auto cull_projection = arut::rendering::perspective_rh_zo(1.04719755F, 1.0F, 0.1F, 50.0F);
    const auto frustum = arut::rendering::extract_vulkan_frustum(
        arut::rendering::multiply(cull_projection, cull_view));
    assert(arut::rendering::sphere_visible(frustum, {{0.0F, 0.0F, 0.0F}, 0.5F}));
    assert(!arut::rendering::sphere_visible(frustum, {{100.0F, 0.0F, 0.0F}, 0.5F}));
    assert(!arut::rendering::sphere_visible(frustum, {{0.0F, 0.0F, 5.0F}, 0.5F}));
    arut::rendering::Mat4 unsnapped = arut::rendering::Mat4::identity();
    unsnapped.value[12] = 0.1234F;
    unsnapped.value[13] = -0.2876F;
    const auto snapped = arut::rendering::stabilize_shadow_matrix(unsnapped, 1024U);
    const float texel = 2.0F / 1024.0F;
    assert(near(snapped.value[12] / texel, std::round(snapped.value[12] / texel)));
    assert(near(snapped.value[13] / texel, std::round(snapped.value[13] / texel)));
    assert(near(arut::rendering::cascade_blend_factor(89.0F, 100.0F), 0.0F));
    assert(near(arut::rendering::cascade_blend_factor(95.0F, 100.0F), 0.5F));
    assert(near(arut::rendering::cascade_blend_factor(100.0F, 100.0F), 1.0F));
    const auto shadow_projection = arut::rendering::orthographic_rh_zo(-4.0F, 4.0F, -4.0F, 4.0F, 0.1F, 20.0F);
    assert(near(shadow_projection.value[0], 0.25F));
    assert(near(shadow_projection.value[5], -0.25F));
    const auto invalid_ortho = arut::rendering::orthographic_rh_zo(1.0F, 1.0F, -1.0F, 1.0F, 0.1F, 10.0F);
    for (const auto value : invalid_ortho.value) assert(near(value, 0.0F));
    const auto invalid_projection = arut::rendering::perspective_rh_zo(1.0F, 0.0F, 0.1F, 100.0F);
    for (const auto value : invalid_projection.value) assert(near(value, 0.0F));
    return 0;
}
