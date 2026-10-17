#ifndef ARUT_CAR_3D_H
#define ARUT_CAR_3D_H

#include <godot_cpp/classes/ray_cast3d.hpp>
#include <godot_cpp/classes/rigid_body3d.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {

class ArutCar3D : public RigidBody3D {
    GDCLASS(ArutCar3D, RigidBody3D)

public:
    enum DriveLayout {
        FRONT_WHEEL_DRIVE = 0,
        REAR_WHEEL_DRIVE = 1,
        ALL_WHEEL_DRIVE = 2
    };

private:
    real_t _wheel_track = 1.55;
    real_t _front_axle_offset = -1.15;
    real_t _rear_axle_offset = 1.15;
    real_t _wheel_mount_height = -0.15;
    real_t _wheel_radius = 0.34;
    real_t _suspension_rest_length = 0.42;
    real_t _suspension_stiffness = 26000.0;
    real_t _suspension_damping = 3200.0;

    real_t _engine_force = 8500.0;
    real_t _brake_force = 11000.0;
    real_t _handbrake_force = 14000.0;
    real_t _max_steering_angle_degrees = 32.0;
    int32_t _drive_layout = REAR_WHEEL_DRIVE;

    real_t _tire_friction = 1.05;
    real_t _lateral_grip = 1.8;

    bool _use_input_actions = true;
    StringName _throttle_action = "ui_up";
    StringName _brake_action = "ui_down";
    StringName _steer_left_action = "ui_left";
    StringName _steer_right_action = "ui_right";
    StringName _handbrake_action = "ui_select";
    int64_t _wheel_collision_mask = 1;

    real_t _throttle = 0.0;
    real_t _brake = 0.0;
    real_t _steering = 0.0;
    real_t _handbrake = 0.0;
    int32_t _wheel_contact_count = 0;
    RayCast3D *_wheel_probes[4] = { nullptr, nullptr, nullptr, nullptr };

    void _update_wheel_probes();

protected:
    static void _bind_methods();

public:
    ArutCar3D();

    void _ready() override;
    void _physics_process(double p_delta) override;

    void set_wheel_track(real_t p_value);
    real_t get_wheel_track() const;
    void set_front_axle_offset(real_t p_value);
    real_t get_front_axle_offset() const;
    void set_rear_axle_offset(real_t p_value);
    real_t get_rear_axle_offset() const;
    void set_wheel_mount_height(real_t p_value);
    real_t get_wheel_mount_height() const;
    void set_wheel_radius(real_t p_value);
    real_t get_wheel_radius() const;
    void set_suspension_rest_length(real_t p_value);
    real_t get_suspension_rest_length() const;
    void set_suspension_stiffness(real_t p_value);
    real_t get_suspension_stiffness() const;
    void set_suspension_damping(real_t p_value);
    real_t get_suspension_damping() const;

    void set_engine_force(real_t p_value);
    real_t get_engine_force() const;
    void set_brake_force(real_t p_value);
    real_t get_brake_force() const;
    void set_handbrake_force(real_t p_value);
    real_t get_handbrake_force() const;
    void set_max_steering_angle_degrees(real_t p_value);
    real_t get_max_steering_angle_degrees() const;
    void set_drive_layout(int32_t p_value);
    int32_t get_drive_layout() const;

    void set_tire_friction(real_t p_value);
    real_t get_tire_friction() const;
    void set_lateral_grip(real_t p_value);
    real_t get_lateral_grip() const;

    void set_use_input_actions(bool p_value);
    bool get_use_input_actions() const;
    void set_throttle_action(const StringName &p_value);
    StringName get_throttle_action() const;
    void set_brake_action(const StringName &p_value);
    StringName get_brake_action() const;
    void set_steer_left_action(const StringName &p_value);
    StringName get_steer_left_action() const;
    void set_steer_right_action(const StringName &p_value);
    StringName get_steer_right_action() const;
    void set_handbrake_action(const StringName &p_value);
    StringName get_handbrake_action() const;
    void set_wheel_collision_mask(int64_t p_value);
    int64_t get_wheel_collision_mask() const;

    // Native entry points for touch controls, AI and future C++ controller nodes.
    void set_throttle(real_t p_value);
    void set_brake(real_t p_value);
    void set_steering(real_t p_value);
    void set_handbrake(real_t p_value);
    real_t get_speed_kph() const;
    int32_t get_wheel_contact_count() const;
};

} // namespace godot

#endif // ARUT_CAR_3D_H
