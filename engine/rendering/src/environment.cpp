#include "arut/rendering/environment.hpp"
#include "arut/rendering/image.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace arut::rendering {
namespace {
struct Vec3 final { float x, y, z; };

[[nodiscard]] Vec3 normalize(Vec3 value) noexcept {
    const float length_squared = value.x * value.x + value.y * value.y + value.z * value.z;
    if (!(length_squared > 1.0e-12F) || !std::isfinite(length_squared)) return {0.0F, 1.0F, 0.0F};
    const float inverse = 1.0F / std::sqrt(length_squared);
    return {value.x * inverse, value.y * inverse, value.z * inverse};
}

[[nodiscard]] float dot(Vec3 a, Vec3 b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
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
        color = {
            horizon.x * (1.0F - blend) + zenith.x * blend,
            horizon.y * (1.0F - blend) + zenith.y * blend,
            horizon.z * (1.0F - blend) + zenith.z * blend
        };
    } else {
        const float blend = std::clamp(-elevation * 3.0F, 0.0F, 1.0F);
        color = {
            horizon.x * (1.0F - blend) + ground.x * blend,
            horizon.y * (1.0F - blend) + ground.y * blend,
            horizon.z * (1.0F - blend) + ground.z * blend
        };
    }
    const float sun = std::pow(std::max(0.0F, dot(direction, sun_direction)), 128.0F) * 2.8F;
    color.x += sun;
    color.y += sun * 0.84F;
    color.z += sun * 0.58F;
    return {to_byte(color.x), to_byte(color.y), to_byte(color.z), 255U};
}
}

bool build_procedural_environment_cube(
    std::uint32_t base_size, std::vector<EnvironmentCubeMip>& output) noexcept {
    output.clear();
    if (base_size == 0U || base_size > 256U) return false;
    try {
        std::array<std::vector<Rgba8Mip>, 6> face_chains{};
        for (std::uint32_t face = 0; face < 6U; ++face) {
            Rgba8Image base{};
            base.width = base_size;
            base.height = base_size;
            base.pixels.resize(static_cast<std::size_t>(base_size) * base_size * 4U);
            for (std::uint32_t y = 0; y < base_size; ++y) {
                for (std::uint32_t x = 0; x < base_size; ++x) {
                    const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(base_size);
                    const float v = (static_cast<float>(y) + 0.5F) / static_cast<float>(base_size);
                    const auto color = evaluate_sky(face_direction(face, u, v));
                    const std::size_t offset = (static_cast<std::size_t>(y) * base_size + x) * 4U;
                    std::copy(color.begin(), color.end(), base.pixels.begin() + static_cast<std::ptrdiff_t>(offset));
                }
            }
            if (!build_rgba8_mip_chain(base, false, face_chains[face])) {
                output.clear();
                return false;
            }
        }

        const std::size_t mip_count = face_chains[0].size();
        output.reserve(mip_count);
        for (std::size_t level = 0; level < mip_count; ++level) {
            EnvironmentCubeMip mip{};
            mip.width = face_chains[0][level].width;
            mip.height = face_chains[0][level].height;
            const std::size_t face_bytes = face_chains[0][level].pixels.size();
            mip.pixels.reserve(face_bytes * 6U);
            for (std::uint32_t face = 0; face < 6U; ++face) {
                if (face_chains[face].size() != mip_count ||
                    face_chains[face][level].width != mip.width ||
                    face_chains[face][level].height != mip.height ||
                    face_chains[face][level].pixels.size() != face_bytes) {
                    output.clear();
                    return false;
                }
                mip.pixels.insert(mip.pixels.end(),
                                  face_chains[face][level].pixels.begin(),
                                  face_chains[face][level].pixels.end());
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
