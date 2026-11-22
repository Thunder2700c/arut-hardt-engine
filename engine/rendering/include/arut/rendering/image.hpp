#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace arut::rendering {

struct Rgba32fImage final {
    std::uint32_t width{0};
    std::uint32_t height{0};
    // Row-major linear-light RGBA floats. RGB may exceed 1.0 for HDR radiance.
    std::vector<float> pixels;
    [[nodiscard]] bool valid() const noexcept {
        return width > 0 && height > 0 &&
               pixels.size() == static_cast<std::size_t>(width) * height * 4U;
    }
};

struct Rgba8Image final {
    std::uint32_t width{0};
    std::uint32_t height{0};
    // Row-major, tightly packed RGBA8 bytes. Row zero is the first decoded row.
    std::vector<std::uint8_t> pixels;

    [[nodiscard]] bool valid() const noexcept {
        return width > 0 && height > 0 &&
               pixels.size() == static_cast<std::size_t>(width) * height * 4U;
    }
};

enum class ImageDecodeError : std::uint8_t {
    None,
    EmptyInput,
    InputTooLarge,
    UnsupportedOrCorrupt,
    DimensionsOutOfRange,
    AllocationFailed
};

struct ImageDecodeResult final {
    ImageDecodeError error{ImageDecodeError::None};
    [[nodiscard]] explicit operator bool() const noexcept { return error == ImageDecodeError::None; }
};

struct Rgba8Mip final {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::vector<std::uint8_t> pixels;
};

// Supports Radiance RGBE HDR input. RGB values remain linear and may exceed 1.0.
[[nodiscard]] ImageDecodeResult decode_image_rgba32f(
    std::span<const std::uint8_t> encoded, Rgba32fImage& output) noexcept;

// Supports PNG, JPEG, BMP and TGA. Decoded images are normalized to four channels.
[[nodiscard]] ImageDecodeResult decode_image_rgba8(
    std::span<const std::uint8_t> encoded, Rgba8Image& output) noexcept;

[[nodiscard]] bool build_rgba8_mip_chain(
    const Rgba8Image& source, bool source_is_srgb,
    std::vector<Rgba8Mip>& output) noexcept;

} // namespace arut::rendering
