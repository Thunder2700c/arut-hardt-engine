#pragma once

#include <algorithm>

#include "core/math/vector2.h"
#include "core/string/string_name.h"
#include "scene/3d/physics/character_body_3d.h"

class Camera3D;
class CollisionShape3D;
class Node3D;
class ArutRagdollDummy3D;

// C++-native player controller with a generated visible dummy, default capsule
// collision and camera. A skinned model can replace the dummy later.
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

    bool create_default_dummy = true;
    bool create_default_collision = true;
    bool create_default_camera = true;
    bool enable_ragdoll = true;
    bool enable_mobile_touch_controls = true;
    real_t camera_distance = 4.0;
    real_t touch_look_sensitivity = 0.005;

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

    Node3D *default_visual_root = nullptr;
    CollisionShape3D *default_collision_shape = nullptr;
    Node3D *camera_pivot = nullptr;
    Camera3D *default_camera = nullptr;
    ArutRagdollDummy3D *ragdoll_dummy = nullptr;
    int32_t joystick_touch_index = -1;
    int32_t look_touch_index = -1;
    Vector2 joystick_origin;
    Vector2 previous_look_position;
    bool ragdoll_started = false;

    void _create_default_dummy();
    void _create_default_collision();
    void _create_default_camera();
    void _create_touch_hint_overlay();
    void _start_ragdoll();

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _physics_process(double p_delta) override;
    void input(const Ref<InputEvent> &p_event) override;

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

    void set_create_default_dummy(bool value) { create_default_dummy = value; }
    bool get_create_default_dummy() const { return create_default_dummy; }
    void set_create_default_collision(bool value) { create_default_collision = value; }
    bool get_create_default_collision() const { return create_default_collision; }
    void set_create_default_camera(bool value) { create_default_camera = value; }
    bool get_create_default_camera() const { return create_default_camera; }
    void set_enable_ragdoll(bool value) { enable_ragdoll = value; }
    bool get_enable_ragdoll() const { return enable_ragdoll; }
    void set_enable_mobile_touch_controls(bool value) { enable_mobile_touch_controls = value; }
    bool get_enable_mobile_touch_controls() const { return enable_mobile_touch_controls; }
    void set_camera_distance(real_t value) { camera_distance = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(12.0)); }
    real_t get_camera_distance() const { return camera_distance; }
    void set_touch_look_sensitivity(real_t value) { touch_look_sensitivity = std::clamp(value, static_cast<real_t>(0.001), static_cast<real_t>(0.02)); }
    real_t get_touch_look_sensitivity() const { return touch_look_sensitivity; }

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

    void set_move_input(const Vector2 &value) { move_input = value.length() > 1.0 ? value.normalized() : value; }
    Vector2 get_move_input() const { return move_input; }
    void request_jump() { jump_requested = true; }
    void set_sprint_active(bool value) { sprint_active = value; }
    void start_ragdoll() { _start_ragdoll(); }
    bool is_ragdoll_started() const { return ragdoll_started; }
};

