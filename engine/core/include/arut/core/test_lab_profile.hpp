#pragma once

#include <cstdint>
#include <string_view>

namespace arut::core {

// Stable IDs shared with the Android Test Laboratory picker.
enum class TestLabProfile : std::uint8_t {
    EcsStress = 0,
    Humanoid = 1,
    Combined = 2
};

[[nodiscard]] constexpr bool valid_test_lab_profile(std::int32_t value) noexcept {
    return value >= static_cast<std::int32_t>(TestLabProfile::EcsStress) &&
           value <= static_cast<std::int32_t>(TestLabProfile::Combined);
}

[[nodiscard]] constexpr bool test_lab_includes_stress(TestLabProfile profile) noexcept {
    return profile == TestLabProfile::EcsStress || profile == TestLabProfile::Combined;
}

[[nodiscard]] constexpr bool test_lab_includes_humanoid(TestLabProfile profile) noexcept {
    return profile == TestLabProfile::Humanoid || profile == TestLabProfile::Combined;
}

[[nodiscard]] constexpr std::string_view test_lab_profile_name(TestLabProfile profile) noexcept {
    switch (profile) {
        case TestLabProfile::EcsStress: return "ecs_stress";
        case TestLabProfile::Humanoid: return "humanoid";
        case TestLabProfile::Combined: return "combined";
    }
    return "invalid";
}

} // namespace arut::core
