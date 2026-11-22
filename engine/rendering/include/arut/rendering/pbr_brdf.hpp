#pragma once

#include <algorithm>
#include <cmath>

namespace arut::rendering::pbr {

struct Vec3 final {
    float x{0.0F}, y{0.0F}, z{0.0F};
};

[[nodiscard]] inline constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
[[nodiscard]] inline constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
[[nodiscard]] inline constexpr Vec3 operator*(Vec3 a, float s) noexcept {
    return {a.x * s, a.y * s, a.z * s};
}
[[nodiscard]] inline constexpr Vec3 operator*(Vec3 a, Vec3 b) noexcept {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}
[[nodiscard]] inline constexpr Vec3 operator/(Vec3 a, float s) noexcept {
    return s != 0.0F ? Vec3{a.x / s, a.y / s, a.z / s} : Vec3{};
}
[[nodiscard]] inline constexpr float dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
[[nodiscard]] inline Vec3 normalize(Vec3 v) noexcept {
    const float length_squared = dot(v, v);
    if (!std::isfinite(length_squared) || length_squared <= 1.0e-12F) return {0.0F, 0.0F, 1.0F};
    return v * (1.0F / std::sqrt(length_squared));
}
[[nodiscard]] inline Vec3 saturate(Vec3 v) noexcept {
    return {std::clamp(v.x, 0.0F, 1.0F), std::clamp(v.y, 0.0F, 1.0F), std::clamp(v.z, 0.0F, 1.0F)};
}
[[nodiscard]] inline float saturate(float v) noexcept {
    return std::clamp(std::isfinite(v) ? v : 0.0F, 0.0F, 1.0F);
}

// Schlick approximation to Fresnel reflectance at the grazing angle.
[[nodiscard]] inline Vec3 fresnel_schlick(float cos_theta, Vec3 f0) noexcept {
    const float f = std::pow(1.0F - saturate(cos_theta), 5.0F);
    return f0 + (Vec3{1.0F, 1.0F, 1.0F} - f0) * f;
}

// Trowbridge-Reitz GGX normal distribution function.
[[nodiscard]] inline float distribution_ggx(float n_dot_h, float roughness) noexcept {
    const float alpha = std::max(0.045F, saturate(roughness));
    const float a2 = alpha * alpha * alpha * alpha;
    const float n = saturate(n_dot_h);
    const float denominator = n * n * (a2 - 1.0F) + 1.0F;
    return a2 / std::max(1.0e-7F, 3.14159265358979323846F * denominator * denominator);
}

[[nodiscard]] inline float geometry_schlick_ggx(float n_dot_v, float roughness) noexcept {
    const float r = saturate(roughness) + 1.0F;
    const float k = (r * r) * 0.125F;
    const float n = saturate(n_dot_v);
    return n / std::max(1.0e-7F, n * (1.0F - k) + k);
}

[[nodiscard]] inline float geometry_smith(float n_dot_v, float n_dot_l, float roughness) noexcept {
    return geometry_schlick_ggx(n_dot_v, roughness) *
           geometry_schlick_ggx(n_dot_l, roughness);
}

// Cook-Torrance microfacet BRDF for one directional light. Inputs are linear-light values.
[[nodiscard]] inline Vec3 evaluate_direct_brdf(
    Vec3 normal, Vec3 view, Vec3 light, Vec3 base_color,
    float metallic, float roughness, Vec3 radiance) noexcept {
    const Vec3 n = normalize(normal);
    const Vec3 v = normalize(view);
    const Vec3 l = normalize(light);
    const Vec3 h = normalize(v + l);
    const float n_dot_v = std::max(1.0e-4F, dot(n, v));
    const float n_dot_l = std::max(0.0F, dot(n, l));
    if (n_dot_l <= 0.0F) return {};
    const float n_dot_h = std::max(0.0F, dot(n, h));
    const float h_dot_v = std::max(0.0F, dot(h, v));
    const float m = saturate(metallic);
    const float r = std::clamp(saturate(roughness), 0.045F, 1.0F);
    const Vec3 f0 = Vec3{0.04F, 0.04F, 0.04F} * (1.0F - m) + saturate(base_color) * m;
    const Vec3 f = fresnel_schlick(h_dot_v, f0);
    const float d = distribution_ggx(n_dot_h, r);
    const float g = geometry_smith(n_dot_v, n_dot_l, r);
    const Vec3 specular = f * (d * g / std::max(1.0e-4F, 4.0F * n_dot_v * n_dot_l));
    const Vec3 diffuse_weight = (Vec3{1.0F, 1.0F, 1.0F} - f) * (1.0F - m);
    const Vec3 diffuse = diffuse_weight * saturate(base_color) * (1.0F / 3.14159265358979323846F);
    return (diffuse + specular) * radiance * n_dot_l;
}

[[nodiscard]] inline Vec3 tone_map_aces(Vec3 linear_color) noexcept {
    const auto curve = [](float x) {
        x = std::max(0.0F, std::isfinite(x) ? x : 0.0F);
        return saturate((x * (2.51F * x + 0.03F)) /
                        (x * (2.43F * x + 0.59F) + 0.14F));
    };
    return {curve(linear_color.x), curve(linear_color.y), curve(linear_color.z)};
}

} // namespace arut::rendering::pbr
