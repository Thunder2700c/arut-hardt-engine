#include "arut/rendering/environment.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace arut::rendering {
namespace {
struct Vec3 final { float x, y, z; };

[[nodiscard]] Vec3 operator+(Vec3 a, Vec3 b) noexcept { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
[[nodiscard]] Vec3 operator-(Vec3 a, Vec3 b) noexcept { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
[[nodiscard]] Vec3 operator*(Vec3 a, float value) noexcept { return {a.x*value,a.y*value,a.z*value}; }

[[nodiscard]] Vec3 normalize(Vec3 value) noexcept {
    const float length_squared = value.x * value.x + value.y * value.y + value.z * value.z;
    if (!(length_squared > 1.0e-12F) || !std::isfinite(length_squared)) return {0.0F, 1.0F, 0.0F};
    return value * (1.0F / std::sqrt(length_squared));
}

[[nodiscard]] float dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

[[nodiscard]] Vec3 cross(Vec3 a, Vec3 b) noexcept {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}

[[nodiscard]] Vec3 face_direction(std::uint32_t face, float u, float v) noexcept {
    const float s = 2.0F * u - 1.0F;
    const float t = 2.0F * v - 1.0F;
    switch (face) {
        case 0: return normalize({1.0F, -t, -s});
        case 1: return normalize({-1.0F, -t, s});
        case 2: return normalize({s, 1.0F, t});
        case 3: return normalize({s, -1.0F, -t});
        case 4: return normalize({s, -t, 1.0F});
        default: return normalize({-s, -t, -1.0F});
    }
}

[[nodiscard]] std::uint8_t to_byte(float value) noexcept {
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0F, 1.0F) * 255.0F));
}

[[nodiscard]] std::array<std::uint8_t, 4> evaluate_sky(Vec3 direction) noexcept {
    constexpr Vec3 ground{0.055F, 0.065F, 0.080F};
    constexpr Vec3 horizon{0.62F, 0.48F, 0.34F};
    constexpr Vec3 zenith{0.10F, 0.22F, 0.42F};
    const Vec3 sun_direction = normalize({-0.45F, 0.78F, -0.36F});
    const float elevation = std::clamp(direction.y, -1.0F, 1.0F);
    Vec3 color{};
    if (elevation >= 0.0F) {
        const float blend = std::pow(elevation, 0.62F);
        color = horizon * (1.0F - blend) + zenith * blend;
    } else {
        const float blend = std::clamp(-elevation * 3.0F, 0.0F, 1.0F);
        color = horizon * (1.0F - blend) + ground * blend;
    }
    const float sun = std::pow(std::max(0.0F, dot(direction, sun_direction)), 128.0F) * 2.8F;
    color.x += sun;
    color.y += sun * 0.84F;
    color.z += sun * 0.58F;
    return {to_byte(color.x), to_byte(color.y), to_byte(color.z), 255U};
}

[[nodiscard]] float radical_inverse_vdc(std::uint32_t bits) noexcept {
    bits = (bits << 16U) | (bits >> 16U);
    bits = ((bits & 0x55555555U) << 1U) | ((bits & 0xAAAAAAAAU) >> 1U);
    bits = ((bits & 0x33333333U) << 2U) | ((bits & 0xCCCCCCCCU) >> 2U);
    bits = ((bits & 0x0F0F0F0FU) << 4U) | ((bits & 0xF0F0F0F0U) >> 4U);
    bits = ((bits & 0x00FF00FFU) << 8U) | ((bits & 0xFF00FF00U) >> 8U);
    return static_cast<float>(bits) * 2.3283064365386963e-10F;
}

[[nodiscard]] Vec3 sample_base_cube(const std::array<std::vector<std::uint8_t>, 6>& faces,
                                    std::uint32_t size, Vec3 direction) noexcept {
    const Vec3 d = normalize(direction);
    const float ax = std::abs(d.x), ay = std::abs(d.y), az = std::abs(d.z);
    std::uint32_t face = 0;
    float s = 0.0F, t = 0.0F, major = 1.0F;
    if (ax >= ay && ax >= az) {
        major = ax;
        if (d.x >= 0.0F) { face = 0; s = -d.z / major; t = -d.y / major; }
        else { face = 1; s = d.z / major; t = -d.y / major; }
    } else if (ay >= az) {
        major = ay;
        if (d.y >= 0.0F) { face = 2; s = d.x / major; t = d.z / major; }
        else { face = 3; s = d.x / major; t = -d.z / major; }
    } else {
        major = az;
        if (d.z >= 0.0F) { face = 4; s = d.x / major; t = -d.y / major; }
        else { face = 5; s = -d.x / major; t = -d.y / major; }
    }
    const float px = std::clamp((s * 0.5F + 0.5F) * static_cast<float>(size) - 0.5F,
                                0.0F, static_cast<float>(size - 1U));
    const float py = std::clamp((t * 0.5F + 0.5F) * static_cast<float>(size) - 0.5F,
                                0.0F, static_cast<float>(size - 1U));
    const std::uint32_t x0 = static_cast<std::uint32_t>(std::floor(px));
    const std::uint32_t y0 = static_cast<std::uint32_t>(std::floor(py));
    const std::uint32_t x1 = std::min(size - 1U, x0 + 1U);
    const std::uint32_t y1 = std::min(size - 1U, y0 + 1U);
    const float tx = px - static_cast<float>(x0);
    const float ty = py - static_cast<float>(y0);
    const auto read = [&](std::uint32_t x, std::uint32_t y) {
        const std::size_t offset = (static_cast<std::size_t>(y) * size + x) * 4U;
        return Vec3{
            static_cast<float>(faces[face][offset]) / 255.0F,
            static_cast<float>(faces[face][offset + 1U]) / 255.0F,
            static_cast<float>(faces[face][offset + 2U]) / 255.0F
        };
    };
    const Vec3 a = read(x0, y0) * (1.0F - tx) + read(x1, y0) * tx;
    const Vec3 b = read(x0, y1) * (1.0F - tx) + read(x1, y1) * tx;
    return a * (1.0F - ty) + b * ty;
}

[[nodiscard]] Vec3 importance_sample_ggx(float xi_x, float xi_y, float roughness,
                                          Vec3 normal) noexcept {
    const float alpha = roughness * roughness;
    const float alpha_squared = alpha * alpha;
    const float phi = 6.28318530717958647692F * xi_x;
    const float cos_theta = std::sqrt((1.0F - xi_y) /
        std::max(1.0e-6F, 1.0F + (alpha_squared - 1.0F) * xi_y));
    const float sin_theta = std::sqrt(std::max(0.0F, 1.0F - cos_theta * cos_theta));
    const Vec3 half_tangent{std::cos(phi) * sin_theta, std::sin(phi) * sin_theta, cos_theta};
    const Vec3 up = std::abs(normal.y) < 0.999F ? Vec3{0.0F, 1.0F, 0.0F} : Vec3{1.0F, 0.0F, 0.0F};
    const Vec3 tangent = normalize(cross(up, normal));
    const Vec3 bitangent = cross(normal, tangent);
    return normalize(tangent * half_tangent.x + bitangent * half_tangent.y + normal * half_tangent.z);
}
}

bool build_procedural_environment_cube(
    std::uint32_t base_size, std::vector<EnvironmentCubeMip>& output) noexcept {
    output.clear();
    if (base_size == 0U || base_size > 256U) return false;
    try {
        std::array<std::vector<std::uint8_t>, 6> base_faces{};
        for (std::uint32_t face = 0; face < 6U; ++face) {
            base_faces[face].resize(static_cast<std::size_t>(base_size) * base_size * 4U);
            for (std::uint32_t y = 0; y < base_size; ++y) {
                for (std::uint32_t x = 0; x < base_size; ++x) {
                    const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(base_size);
                    const float v = (static_cast<float>(y) + 0.5F) / static_cast<float>(base_size);
                    const auto color = evaluate_sky(face_direction(face, u, v));
                    const std::size_t offset = (static_cast<std::size_t>(y) * base_size + x) * 4U;
                    std::copy(color.begin(), color.end(),
                              base_faces[face].begin() + static_cast<std::ptrdiff_t>(offset));
                }
            }
        }

        std::uint32_t mip_count = 1U;
        for (std::uint32_t dimension = base_size; dimension > 1U; dimension /= 2U) ++mip_count;
        output.reserve(mip_count);
        EnvironmentCubeMip base{};
        base.width = base_size;
        base.height = base_size;
        for (const auto& face : base_faces)
            base.pixels.insert(base.pixels.end(), face.begin(), face.end());
        output.push_back(std::move(base));

        constexpr std::uint32_t kSampleCount = 64U;
        for (std::uint32_t level = 1U; level < mip_count; ++level) {
            const std::uint32_t dimension = std::max(1U, base_size >> level);
            const float roughness = static_cast<float>(level) / static_cast<float>(mip_count - 1U);
            EnvironmentCubeMip mip{};
            mip.width = dimension;
            mip.height = dimension;
            mip.pixels.resize(static_cast<std::size_t>(dimension) * dimension * 4U * 6U);
            for (std::uint32_t face = 0; face < 6U; ++face) {
                for (std::uint32_t y = 0; y < dimension; ++y) {
                    for (std::uint32_t x = 0; x < dimension; ++x) {
                        const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(dimension);
                        const float v = (static_cast<float>(y) + 0.5F) / static_cast<float>(dimension);
                        const Vec3 normal = face_direction(face, u, v);
                        const Vec3 view = normal;
                        Vec3 accumulated{};
                        float total_weight = 0.0F;
                        for (std::uint32_t sample = 0; sample < kSampleCount; ++sample) {
                            const float xi_x = static_cast<float>(sample) / static_cast<float>(kSampleCount);
                            const float xi_y = radical_inverse_vdc(sample);
                            const Vec3 half_vector = importance_sample_ggx(xi_x, xi_y, roughness, normal);
                            const Vec3 light = normalize(half_vector * (2.0F * dot(view, half_vector)) - view);
                            const float n_dot_l = std::max(dot(normal, light), 0.0F);
                            if (n_dot_l > 0.0F) {
                                accumulated = accumulated + sample_base_cube(base_faces, base_size, light) * n_dot_l;
                                total_weight += n_dot_l;
                            }
                        }
                        const Vec3 filtered = total_weight > 1.0e-6F
                            ? accumulated * (1.0F / total_weight)
                            : sample_base_cube(base_faces, base_size, normal);
                        const std::size_t offset =
                            (static_cast<std::size_t>(face) * dimension * dimension +
                             static_cast<std::size_t>(y) * dimension + x) * 4U;
                        mip.pixels[offset] = to_byte(filtered.x);
                        mip.pixels[offset + 1U] = to_byte(filtered.y);
                        mip.pixels[offset + 2U] = to_byte(filtered.z);
                        mip.pixels[offset + 3U] = 255U;
                    }
                }
            }
            output.push_back(std::move(mip));
        }
    } catch (...) {
        output.clear();
        return false;
    }
    return !output.empty();
}

} // namespace arut::rendering
