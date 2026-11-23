#include "arut/core/third_person_camera.hpp"

#include <cassert>
#include <cmath>
#include <span>

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

    const CharacterAabbObstacle camera_wall{
        {-1.0F, 0.0F, 2.0F}, {1.0F, 2.5F, 3.0F}};
    const std::array<CharacterAabbObstacle, 1> obstacles{camera_wall};
    const auto geometry_blocked = solve_third_person_camera(
        target, yaw, 0.0F, flat_ground, nullptr, {},
        std::span<const CharacterAabbObstacle>{obstacles});
    assert(geometry_blocked.valid);
    assert(geometry_blocked.geometry_obstructed);
    assert(geometry_blocked.distance < clear.distance);
    assert(geometry_blocked.distance >= 0.8F);
    assert(geometry_blocked.position[2] < 2.0F);

    const CharacterAabbObstacle off_path_wall{
        {2.0F, 0.0F, 2.0F}, {3.0F, 2.5F, 3.0F}};
    const std::array<CharacterAabbObstacle, 1> off_path{off_path_wall};
    const auto geometry_clear = solve_third_person_camera(
        target, yaw, 0.0F, flat_ground, nullptr, {},
        std::span<const CharacterAabbObstacle>{off_path});
    assert(geometry_clear.valid);
    assert(!geometry_clear.geometry_obstructed);
    assert(std::abs(geometry_clear.distance - clear.distance) < 1.0e-5F);

    ThirdPersonCameraConfig invalid{};
    invalid.sweep_samples = 0U;
    assert(!solve_third_person_camera(
        target, yaw, 0.0F, flat_ground, nullptr, invalid).valid);

    assert(!solve_third_person_camera(
        target, yaw, 0.0F, nullptr).valid);
    return 0;
}
