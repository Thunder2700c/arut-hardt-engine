#include "arut/rendering/brdf_lut.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace arut::rendering {
namespace {
struct Vec3 final { float x, y, z; };
[[nodiscard]] float dot(Vec3 a, Vec3 b) noexcept { return a.x*b.x + a.y*b.y + a.z*b.z; }
[[nodiscard]] Vec3 operator*(Vec3 value, float scalar) noexcept {
    return {value.x*scalar, value.y*scalar, value.z*scalar};
}
[[nodiscard]] Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return {a.x-b.x, a.y-b.y, a.z-b.z};
}
[[nodiscard]] Vec3 normalize(Vec3 value) noexcept {
    const float length_squared = dot(value, value);
    if (!(length_squared > 1.0e-12F) || !std::isfinite(length_squared)) return {0.0F, 0.0F, 1.0F};
    return value * (1.0F / std::sqrt(length_squared));
}
[[nodiscard]] float geometry_schlick_ggx(float n_dot_v, float roughness) noexcept {
    const float k = (roughness * roughness) * 0.5F;
    return n_dot_v / std::max(1.0e-6F, n_dot_v * (1.0F-k) + k);
}
[[nodiscard]] float geometry_smith(float n_dot_v, float n_dot_l, float roughness) noexcept {
    return geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness);
}
[[nodiscard]] float radical_inverse(std::uint32_t bits) noexcept {
    bits = (bits << 16U) | (bits >> 16U);
    bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
    bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
    bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
    bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
    return static_cast<float>(bits) * 2.3283064365386963e-10F;
}
[[nodiscard]] Vec3 importance_sample_ggx(float xi_x, float xi_y, float roughness) noexcept {
    const float alpha = roughness * roughness;
    const float alpha_squared = alpha * alpha;
    const float phi = 6.28318530717958647692F * xi_x;
    const float cos_theta = std::sqrt((1.0F-xi_y) /
        std::max(1.0e-6F, 1.0F+(alpha_squared-1.0F)*xi_y));
    const float sin_theta = std::sqrt(std::max(0.0F,1.0F-cos_theta*cos_theta));
    return {std::cos(phi)*sin_theta, std::sin(phi)*sin_theta, cos_theta};
}
[[nodiscard]] std::uint8_t to_byte(float value) noexcept {
    return static_cast<std::uint8_t>(std::lround(std::clamp(value,0.0F,1.0F)*255.0F));
}
}

bool build_brdf_integration_lut(
    std::uint32_t dimension, std::uint32_t sample_count,
    Rgba8Image& output) noexcept {
    output = {};
    if (dimension < 4U || dimension > 256U || sample_count < 8U || sample_count > 2048U)
        return false;
    try {
        output.width = dimension;
        output.height = dimension;
        output.pixels.resize(static_cast<std::size_t>(dimension) * dimension * 4U);
        for (std::uint32_t y=0; y<dimension; ++y) {
            const float roughness = (static_cast<float>(y)+0.5F)/static_cast<float>(dimension);
            for (std::uint32_t x=0; x<dimension; ++x) {
                const float n_dot_v = (static_cast<float>(x)+0.5F)/static_cast<float>(dimension);
                const Vec3 view{std::sqrt(std::max(0.0F,1.0F-n_dot_v*n_dot_v)),0.0F,n_dot_v};
                float scale = 0.0F;
                float bias = 0.0F;
                for (std::uint32_t sample=0; sample<sample_count; ++sample) {
                    const float xi_x = static_cast<float>(sample)/static_cast<float>(sample_count);
                    const float xi_y = radical_inverse(sample);
                    const Vec3 half_vector = importance_sample_ggx(xi_x,xi_y,roughness);
                    const Vec3 light = normalize(half_vector*(2.0F*dot(view,half_vector))-view);
                    const float n_dot_l = std::max(light.z,0.0F);
                    const float n_dot_h = std::max(half_vector.z,0.0F);
                    const float v_dot_h = std::max(dot(view,half_vector),0.0F);
                    if (n_dot_l > 0.0F && n_dot_h > 0.0F && v_dot_h > 0.0F) {
                        const float visibility = geometry_smith(n_dot_v,n_dot_l,roughness) *
                            v_dot_h / std::max(1.0e-6F,n_dot_h*n_dot_v);
                        const float fresnel = std::pow(1.0F-v_dot_h,5.0F);
                        scale += (1.0F-fresnel)*visibility;
                        bias += fresnel*visibility;
                    }
                }
                scale /= static_cast<float>(sample_count);
                bias /= static_cast<float>(sample_count);
                const std::size_t offset=(static_cast<std::size_t>(y)*dimension+x)*4U;
                output.pixels[offset]=to_byte(scale);
                output.pixels[offset+1U]=to_byte(bias);
                output.pixels[offset+2U]=0U;
                output.pixels[offset+3U]=255U;
            }
        }
    } catch (...) {
        output = {};
        return false;
    }
    return output.valid();
}

} // namespace arut::rendering
