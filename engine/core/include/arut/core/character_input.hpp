#pragma once

#include "arut/core/character_controller.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace arut::core {

enum class CharacterInputKey : std::uint8_t {
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    Jump
};

// Platform-neutral input merger. Window/input backends translate their native
// key and analog events into this class; gameplay consumes one normalized sample.
class CharacterInputRouter final {
public:
    void set_key(CharacterInputKey key, bool pressed) noexcept {
        bool* value = key_state(key);
        if (!value) return;
        if (key == CharacterInputKey::Jump && pressed && !*value)
            jump_pending_ = true;
        *value = pressed;
    }

    void set_analog(float x, float y) noexcept {
        analog_x_ = std::isfinite(x) ? std::clamp(x, -1.0F, 1.0F) : 0.0F;
        analog_y_ = std::isfinite(y) ? std::clamp(y, -1.0F, 1.0F) : 0.0F;
    }

    [[nodiscard]] CharacterInput sample() noexcept {
        float x = analog_x_ + (right_ ? 1.0F : 0.0F) -
                  (left_ ? 1.0F : 0.0F);
        float y = analog_y_ + (forward_ ? 1.0F : 0.0F) -
                  (backward_ ? 1.0F : 0.0F);
        const float length = std::sqrt(x*x + y*y);
        if (length > 1.0F) {
            x /= length;
            y /= length;
        }
        const bool jump = jump_pending_;
        jump_pending_ = false;
        return {x, y, jump};
    }

    void clear() noexcept {
        forward_ = backward_ = left_ = right_ = jump_held_ = false;
        analog_x_ = analog_y_ = 0.0F;
        jump_pending_ = false;
    }

private:
    [[nodiscard]] bool* key_state(CharacterInputKey key) noexcept {
        switch (key) {
            case CharacterInputKey::MoveForward: return &forward_;
            case CharacterInputKey::MoveBackward: return &backward_;
            case CharacterInputKey::MoveLeft: return &left_;
            case CharacterInputKey::MoveRight: return &right_;
            case CharacterInputKey::Jump: return &jump_held_;
        }
        return nullptr;
    }

    bool forward_{false};
    bool backward_{false};
    bool left_{false};
    bool right_{false};
    bool jump_held_{false};
    bool jump_pending_{false};
    float analog_x_{0.0F};
    float analog_y_{0.0F};
};

} // namespace arut::core
