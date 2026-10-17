#pragma once

#include <algorithm>

#include "core/string/string_name.h"
#include "core/math/vector2.h"
#include "scene/3d/physics/character_body_3d.h"

// C++-native first-person/third-person movement baseline. Camera and visual
// rig remain user-supplied children; movement does not require a script.
class ArutCharacter3D : public CharacterBody3D {
    GDCLASS(ArutCharacter3D, CharacterBody3D);

private:
    real_t walk_speed = 5.0;
    real_t sprint_speed = 8.0;
    real_t ground_acceleration = 18.0;
    real_t air_acceleration = 4.0;
    real_t air_control = 0.35;
    real_t ground_friction = 16.0;
    real_t jump_velocity = 4.5;
    real_t gravity = 9.8;

    bool use_input_actions = true;
    StringName move_left_action = "ui_left";
    StringName move_right_action = "ui_right";
    StringName move_forward_action = "ui_up";
    StringName move_backward_action = "ui_down";
    StringName jump_action = "ui_accept";
    StringName sprint_action = "sprint";

    Vector2 move_input;
    bool jump_requested = false;
    bool sprint_active = false;

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _physics_process(double p_delta) override;

    void set_walk_speed(real_t value) { walk_speed = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(50.0)); }
    real_t get_walk_speed() const { return walk_speed; }
    void set_sprint_speed(real_t value) { sprint_speed = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(70.0)); }
    real_t get_sprint_speed() const { return sprint_speed; }
    void set_ground_acceleration(real_t value) { ground_acceleration = std::clamp(value, static_cast<real_t>(0.1), static_cast<real_t>(100.0)); }
    real_t get_ground_acceleration() const { return ground_acceleration; }
    void set_air_acceleration(real_t value) { air_acceleration = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(50.0)); }
    real_t get_air_acceleration() const { return air_acceleration; }
    void set_air_control(real_t value) { air_control = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(1.0)); }
    real_t get_air_control() const { return air_control; }
    void set_ground_friction(real_t value) { ground_friction = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(100.0)); }
    real_t get_ground_friction() const { return ground_friction; }
    void set_jump_velocity(real_t value) { jump_velocity = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(30.0)); }
    real_t get_jump_velocity() const { return jump_velocity; }
    void set_gravity(real_t value) { gravity = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(100.0)); }
    real_t get_gravity() const { return gravity; }

    void set_use_input_actions(bool value) { use_input_actions = value; }
    bool get_use_input_actions() const { return use_input_actions; }
    void set_move_left_action(const StringName &value) { move_left_action = value; }
    StringName get_move_left_action() const { return move_left_action; }
    void set_move_right_action(const StringName &value) { move_right_action = value; }
    StringName get_move_right_action() const { return move_right_action; }
    void set_move_forward_action(const StringName &value) { move_forward_action = value; }
    StringName get_move_forward_action() const { return move_forward_action; }
    void set_move_backward_action(const StringName &value) { move_backward_action = value; }
    StringName get_move_backward_action() const { return move_backward_action; }
    void set_jump_action(const StringName &value) { jump_action = value; }
    StringName get_jump_action() const { return jump_action; }
    void set_sprint_action(const StringName &value) { sprint_action = value; }
    StringName get_sprint_action() const { return sprint_action; }

    // Native entry points for mobile touch controls, AI and future input layers.
    void set_move_input(const Vector2 &value) { move_input = value.length() > 1.0 ? value.normalized() : value; }
    Vector2 get_move_input() const { return move_input; }
    void request_jump() { jump_requested = true; }
    void set_sprint_active(bool value) { sprint_active = value; }
};

