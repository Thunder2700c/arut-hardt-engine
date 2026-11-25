#include "arut/core/character_controller.hpp"
#include "arut/core/character_collision.hpp"

#include <cassert>
#include <algorithm>

#include <cmath>
#include <cstdint>

namespace {
bool wall_collision(const arut::core::Vec3& feet, float radius,
                    float height, void* raw) noexcept {
    const auto& wall = *static_cast<const arut::core::CharacterAabbObstacle*>(raw);
    return arut::core::character_capsule_overlaps_aabb(
        feet, radius, height, wall);
}

struct GroundMode final {
    enum class Kind { Flat, SteepBeyondOrigin, StepBeyondOrigin } kind{Kind::Flat};
};

arut::core::CharacterGroundSample ground_query(
    float x, float, void* raw) noexcept {
    const auto* mode = static_cast<const GroundMode*>(raw);
    arut::core::CharacterGroundSample sample{};
    sample.valid = true;
    sample.normal = {0.0F, 1.0F, 0.0F};
    if (mode->kind == GroundMode::Kind::SteepBeyondOrigin && x > 0.001F) {
        sample.normal = {0.95F, 0.30F, 0.0F};
    }
    if (mode->kind == GroundMode::Kind::StepBeyondOrigin && x > 0.001F) {
        sample.height = 0.5F;
    }
    return sample;
}

} // namespace

int main() {
    using namespace arut::core;
    constexpr float dt = 1.0F / 60.0F;
    GroundMode flat{};
    CharacterController controller;
    controller.reset();

    CharacterInput forward{};
    forward.move_y = 1.0F;
    for (int i = 0; i < 60; ++i)
        assert(controller.fixed_step(forward, ground_query, &flat));
    assert(controller.state().grounded);
    assert(controller.state().position.z < -2.0F);
    assert(std::abs(controller.state().position.y) < 1.0e-5F);
    assert(controller.state().velocity.z < -4.0F);

    // Pulling the virtual stick downward supplies negative local Y. That is
    // backward locomotion, not jump/fall input, and must remain grounded.
    controller.reset();
    CharacterInput backward{};
    backward.move_y = -1.0F;
    for (int i = 0; i < 60; ++i)
        assert(controller.fixed_step(backward, ground_query, &flat));
    assert(controller.state().grounded);
    assert(controller.state().position.z > 2.0F);
    assert(controller.state().velocity.z > 4.0F);

    CharacterInput release{};
    for (int i = 0; i < 60; ++i)
        assert(controller.fixed_step(release, ground_query, &flat));
    assert(std::abs(controller.state().velocity.z) < 1.0e-4F);

    CharacterInput jump{};
    jump.jump_pressed = true;
    assert(controller.fixed_step(jump, ground_query, &flat));
    assert(!controller.state().grounded);
    assert(controller.state().velocity.y > 4.0F);
    assert(controller.state().position.y > 0.0F);
    for (int i = 0; i < 120; ++i)
        assert(controller.fixed_step(release, ground_query, &flat));
    assert(controller.state().grounded);
    assert(std::abs(controller.state().position.y) < 1.0e-5F);

    GroundMode steep{GroundMode::Kind::SteepBeyondOrigin};
    controller.reset();
    CharacterInput right{};
    right.move_x = 1.0F;
    for (int i = 0; i < 10; ++i)
        assert(controller.fixed_step(right, ground_query, &steep));
    assert(std::abs(controller.state().position.x) < 1.0e-5F);

    GroundMode step{GroundMode::Kind::StepBeyondOrigin};
    controller.reset();
    for (int i = 0; i < 10; ++i)
        assert(controller.fixed_step(right, ground_query, &step));
    assert(std::abs(controller.state().position.x) < 1.0e-5F);

    CharacterAabbObstacle wall{{1.0F, 0.0F, -20.0F},
                                {1.4F, 3.0F, 20.0F}};
    controller.reset();
    CharacterInput diagonal{};
    diagonal.move_x = 1.0F;
    diagonal.move_y = 1.0F;
    for (int i = 0; i < 120; ++i)
        assert(controller.fixed_step(diagonal, ground_query, &flat,
                                     wall_collision, &wall));
    assert(controller.state().position.x < wall.minimum.x);
    assert(controller.state().position.x > wall.minimum.x - 0.45F);
    assert(controller.state().position.z < -2.0F);

    controller.reset();
    const auto steps = controller.update(forward, 0.16F, ground_query, &flat);
    assert(steps == controller.config().maximum_steps_per_update);
    assert(controller.interpolation_alpha() >= 0.0F &&
           controller.interpolation_alpha() < 1.0F);

    controller.reset();
    CharacterInput queued_jump{};
    queued_jump.jump_pressed = true;
    assert(controller.update(queued_jump, 0.001F, ground_query, &flat) == 0U);
    assert(controller.update(release, 0.02F, ground_query, &flat) == 1U);
    assert(!controller.state().grounded);
    assert(controller.state().position.y > 0.0F);

    // Regression: a grounded idle character must remain constrained to
    // the support plane for a long run, with no gravity-induced fall drift.
    controller.reset();
    for (int i = 0; i < 600; ++i)
        assert(controller.fixed_step(release, ground_query, &flat));
    assert(controller.state().grounded);
    assert(std::abs(controller.state().position.y) < 1.0e-6F);
    assert(std::abs(controller.state().velocity.y) < 1.0e-6F);

    const CharacterAabbObstacle nearby_wall{
        {0.9F, 0.0F, -1.0F}, {1.2F, 2.0F, 1.0F}};
    assert(!character_capsule_overlaps_aabb(
        {0.0F, 0.0F, 0.0F}, 0.32F, 1.8F, nearby_wall));
    assert(character_capsule_overlaps_aabb(
        {0.62F, 0.0F, 0.0F}, 0.32F, 1.8F, nearby_wall));
    const CharacterAabbObstacle ceiling{
        {-1.0F, 1.6F, -1.0F}, {1.0F, 2.0F, 1.0F}};
    assert(character_capsule_overlaps_aabb(
        {0.0F, 0.0F, 0.0F}, 0.32F, 1.8F, ceiling));
    const CharacterAabbObstacle high_ceiling{
        {-1.0F, 3.0F, -1.0F}, {1.0F, 4.0F, 1.0F}};
    assert(!character_capsule_overlaps_aabb(
        {0.0F, 0.0F, 0.0F}, 0.32F, 1.8F, high_ceiling));

    CharacterControllerConfig invalid{};
    invalid.capsule_height = 0.1F;
    CharacterController fallback{invalid};
    assert(fallback.config().valid());
    return 0;
}
