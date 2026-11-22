#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "arut/rendering/image.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <utility>

namespace arut::rendering {
namespace {
constexpr int kMaxDimension = 8192;
constexpr std::uint64_t kMaxPixels = 16ULL * 1024ULL * 1024ULL;

[[nodiscard]] float srgb_to_linear(float value) noexcept {
    return value <= 0.04045F ? value / 12.92F
                             : std::pow((value + 0.055F) / 1.055F, 2.4F);
}

[[nodiscard]] float linear_to_srgb(float value) noexcept {
    value = std::clamp(value, 0.0F, 1.0F);
    return value <= 0.0031308F ? value * 12.92F
                               : 1.055F * std::pow(value, 1.0F / 2.4F) - 0.055F;
}

[[nodiscard]] std::uint8_t to_byte(float value) noexcept {
    return static_cast<std::uint8_t>(std::lround(std::clamp(value, 0.0F, 1.0F) * 255.0F));
}
}


ImageDecodeResult decode_image_rgba32f(
    std::span<const std::uint8_t> encoded, Rgba32fImage& output) noexcept {
    output = {};
    if (encoded.empty()) return {ImageDecodeError::EmptyInput};
    if (encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {ImageDecodeError::InputTooLarge};

    int width = 0;
    int height = 0;
    int source_channels = 0;
    const auto* bytes = reinterpret_cast<const stbi_uc*>(encoded.data());
    const int input_size = static_cast<int>(encoded.size());
    if (stbi_is_hdr_from_memory(bytes, input_size) == 0 ||
        stbi_info_from_memory(bytes, input_size, &width, &height, &source_channels) == 0)
        return {ImageDecodeError::UnsupportedOrCorrupt};
    if (width <= 0 || height <= 0 || width > kMaxDimension || height > kMaxDimension ||
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) > kMaxPixels)
        return {ImageDecodeError::DimensionsOutOfRange};

    float* decoded = stbi_loadf_from_memory(bytes, input_size, &width, &height, nullptr, STBI_rgb_alpha);
    if (!decoded) return {ImageDecodeError::UnsupportedOrCorrupt};
    const std::size_t value_count = static_cast<std::size_t>(width) *
                                    static_cast<std::size_t>(height) * 4U;
    try {
        output.width = static_cast<std::uint32_t>(width);
        output.height = static_cast<std::uint32_t>(height);
        output.pixels.assign(decoded, decoded + value_count);
    } catch (...) {
        stbi_image_free(decoded);
        output = {};
        return {ImageDecodeError::AllocationFailed};
    }
    stbi_image_free(decoded);
    if (!output.valid()) {
        output = {};
        return {ImageDecodeError::UnsupportedOrCorrupt};
    }
    for (float value : output.pixels) {
        if (!std::isfinite(value) || value < 0.0F) {
            output = {};
            return {ImageDecodeError::UnsupportedOrCorrupt};
        }
    }
    return {ImageDecodeError::None};
}

ImageDecodeResult decode_image_rgba8(
    std::span<const std::uint8_t> encoded, Rgba8Image& output) noexcept {
    output = {};
    if (encoded.empty()) return {ImageDecodeError::EmptyInput};
    if (encoded.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return {ImageDecodeError::InputTooLarge};

    int width = 0;
    int height = 0;
    int source_channels = 0;
    const auto* bytes = reinterpret_cast<const stbi_uc*>(encoded.data());
    const int input_size = static_cast<int>(encoded.size());
    if (stbi_info_from_memory(bytes, input_size, &width, &height, &source_channels) == 0)
        return {ImageDecodeError::UnsupportedOrCorrupt};
    if (width <= 0 || height <= 0 || width > kMaxDimension || height > kMaxDimension ||
        static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height) > kMaxPixels)
        return {ImageDecodeError::DimensionsOutOfRange};

    stbi_uc* decoded = stbi_load_from_memory(bytes, input_size, &width, &height, nullptr, STBI_rgb_alpha);
    if (!decoded) return {ImageDecodeError::UnsupportedOrCorrupt};
    const std::size_t byte_count = static_cast<std::size_t>(width) *
                                   static_cast<std::size_t>(height) * 4U;
    try {
        output.width = static_cast<std::uint32_t>(width);
        output.height = static_cast<std::uint32_t>(height);
        output.pixels.assign(decoded, decoded + byte_count);
    } catch (...) {
        stbi_image_free(decoded);
        output = {};
        return {ImageDecodeError::AllocationFailed};
    }
    stbi_image_free(decoded);
    if (!output.valid()) {
        output = {};
        return {ImageDecodeError::UnsupportedOrCorrupt};
    }
    return {ImageDecodeError::None};
}

bool build_rgba8_mip_chain(
    const Rgba8Image& source, bool source_is_srgb,
    std::vector<Rgba8Mip>& output) noexcept {
    output.clear();
    if (!source.valid() || source.width > kMaxDimension || source.height > kMaxDimension ||
        static_cast<std::uint64_t>(source.width) * source.height > kMaxPixels) return false;
    try {
        Rgba8Mip base{};
        base.width = source.width;
        base.height = source.height;
        base.pixels = source.pixels;
        output.push_back(std::move(base));

        while (output.back().width > 1U || output.back().height > 1U) {
            const Rgba8Mip& previous = output.back();
            Rgba8Mip next{};
            next.width = std::max(1U, previous.width / 2U);
            next.height = std::max(1U, previous.height / 2U);
            next.pixels.resize(static_cast<std::size_t>(next.width) * next.height * 4U);
            for (std::uint32_t y = 0; y < next.height; ++y) {
                for (std::uint32_t x = 0; x < next.width; ++x) {
                    float sums[4]{};
                    std::uint32_t samples = 0;
                    for (std::uint32_t oy = 0; oy < 2U; ++oy) {
                        const std::uint32_t sy = y * 2U + oy;
                        if (sy >= previous.height) continue;
                        for (std::uint32_t ox = 0; ox < 2U; ++ox) {
                            const std::uint32_t sx = x * 2U + ox;
                            if (sx >= previous.width) continue;
                            const std::size_t offset =
                                (static_cast<std::size_t>(sy) * previous.width + sx) * 4U;
                            for (std::uint32_t channel = 0; channel < 4U; ++channel) {
                                float value = static_cast<float>(previous.pixels[offset + channel]) / 255.0F;
                                if (source_is_srgb && channel < 3U) value = srgb_to_linear(value);
                                sums[channel] += value;
                            }
                            ++samples;
                        }
                    }
                    const std::size_t destination =
                        (static_cast<std::size_t>(y) * next.width + x) * 4U;
                    for (std::uint32_t channel = 0; channel < 4U; ++channel) {
                        float average = sums[channel] / static_cast<float>(std::max(1U, samples));
                        if (source_is_srgb && channel < 3U) average = linear_to_srgb(average);
                        next.pixels[destination + channel] = to_byte(average);
                    }
                }
            }
            output.push_back(std::move(next));
        }
    } catch (...) {
        output.clear();
        return false;
    }
    return !output.empty();
}

} // namespace arut::rendering
