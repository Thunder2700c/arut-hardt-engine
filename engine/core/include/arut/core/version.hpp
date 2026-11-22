#pragma once

#include <cstdint>
#include <string_view>

namespace arut::core {
struct Version final {
    std::uint32_t major;
    std::uint32_t minor;
    std::uint32_t patch;
};

[[nodiscard]] constexpr Version version() noexcept { return {0, 1, 0}; }
[[nodiscard]] constexpr std::string_view name() noexcept { return "ARUT-HARDT Engine"; }
} // namespace arut::core
