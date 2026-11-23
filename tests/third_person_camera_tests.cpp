#include "arut/core/third_person_camera.hpp"

#include <cassert>
#include <cmath>

namespace {
arut::core::CharacterGroundSample flat_ground(float, float, void*) noexcept {
    return {0.0F, {0.0F, 1.0F, 0.0F}, true};
}

arut::core::CharacterGroundSample ridge_ground(float, float z, void*) noexcept {
    const float height = (z > 1.8F && z < 3.2F) ? 2.0F : 0.0F;
    return {height, {0.0F, 1.0F, 0.0F}, true};
}
}

int main() {
    using namespace arut::core;
    const std::array<float, 3> target{0.0F, 1.0F, 0.0F};
    constexpr float yaw = -1.570796327F;

    const auto clear = solve_third_person_camera(target, yaw, 0.0F, flat_ground);
    assert(clear.valid);
    assert(!clear.terrain_obstructed);
    assert(std::abs(clear.distance - 4.2F) < 1.0e-5F);
    assert(std::abs(clear.position[2] - 4.2F) < 1.0e-4F);

    const auto obstructed = solve_third_person_camera(
        target, yaw, 0.0F, ridge_ground);
    assert(obstructed.valid);
    assert(obstructed.terrain_obstructed);
    assert(obstructed.distance < clear.distance);
    assert(obstructed.distance >= 0.8F);
    assert(obstructed.position[2] < 1.8F);

    ThirdPersonCameraConfig invalid{};
    invalid.sweep_samples = 0U;
    assert(!solve_third_person_camera(
        target, yaw, 0.0F, flat_ground, nullptr, invalid).valid);

    assert(!solve_third_person_camera(
        target, yaw, 0.0F, nullptr).valid);
    return 0;
}
