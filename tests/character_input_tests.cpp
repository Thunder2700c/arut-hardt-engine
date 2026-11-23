#include "arut/core/character_input.hpp"

#include <cassert>
#include <cmath>
#include <limits>

int main() {
    using namespace arut::core;
    CharacterInputRouter input;

    input.set_key(CharacterInputKey::MoveForward, true);
    auto sample = input.sample();
    assert(sample.move_y == 1.0F);
    assert(sample.move_x == 0.0F);
    assert(!sample.jump_pressed);

    input.set_key(CharacterInputKey::MoveRight, true);
    sample = input.sample();
    assert(std::abs(sample.move_x - 0.7071067F) < 1.0e-5F);
    assert(std::abs(sample.move_y - 0.7071067F) < 1.0e-5F);

    input.set_key(CharacterInputKey::MoveRight, false);
    input.set_analog(-0.5F, 0.0F);
    sample = input.sample();
    assert(sample.move_x < 0.0F);
    assert(sample.move_y > 0.0F);

    input.set_key(CharacterInputKey::Jump, true);
    input.set_key(CharacterInputKey::Jump, true);
    assert(input.sample().jump_pressed);
    assert(!input.sample().jump_pressed);
    input.set_key(CharacterInputKey::Jump, false);
    input.set_key(CharacterInputKey::Jump, true);
    assert(input.sample().jump_pressed);

    input.set_analog(std::numeric_limits<float>::quiet_NaN(),
                     std::numeric_limits<float>::infinity());
    sample = input.sample();
    assert(std::isfinite(sample.move_x) && std::isfinite(sample.move_y));

    input.clear();
    sample = input.sample();
    assert(sample.move_x == 0.0F && sample.move_y == 0.0F);
    assert(!sample.jump_pressed);
    return 0;
}
