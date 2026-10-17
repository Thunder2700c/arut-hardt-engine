#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

#include "core/string/string_name.h"
#include "scene/3d/physics/ray_cast_3d.h"
#include "scene/3d/physics/rigid_body_3d.h"

// High-level vehicle node. The chassis visual and collision shape are children
// supplied by the user; wheel probes are managed internally by this component.
class ArutCar3D : public RigidBody3D {
    GDCLASS(ArutCar3D, RigidBody3D);

public:
    enum VehiclePreset {
        CUSTOM = 0,
        COMPACT = 1,
        SEDAN = 2,
        OFFROAD = 3
    };

    enum DriveLayout {
        FRONT_WHEEL_DRIVE = 0,
        REAR_WHEEL_DRIVE = 1,
        ALL_WHEEL_DRIVE = 2
    };

private:
    int32_t vehicle_preset = SEDAN;

    real_t wheel_track = 1.55;
    real_t front_axle_offset = -1.15;
    real_t rear_axle_offset = 1.15;
    real_t wheel_mount_height = -0.15;
    real_t wheel_radius = 0.34;

    real_t suspension_rest_length = 0.42;
    real_t suspension_stiffness = 26000.0;
    real_t suspension_damping = 3200.0;

    real_t engine_force = 8500.0;
    real_t brake_force = 11000.0;
    real_t handbrake_force = 14000.0;
    real_t max_steering_angle_degrees = 32.0;
    int32_t drive_layout = REAR_WHEEL_DRIVE;

    real_t tire_friction = 1.05;
    real_t lateral_grip = 1.8;

    bool use_input_actions = true;
    StringName throttle_action = "ui_up";
    StringName brake_action = "ui_down";
    StringName steer_left_action = "ui_left";
    StringName steer_right_action = "ui_right";
    StringName handbrake_action = "ui_select";
    uint32_t wheel_collision_mask = 1;

    real_t throttle_input = 0.0;
    real_t brake_input = 0.0;
    real_t steering_input = 0.0;
    real_t handbrake_input = 0.0;
    int32_t wheel_contact_count = 0;

    RayCast3D *wheel_probes[4] = { nullptr, nullptr, nullptr, nullptr };

    void _update_wheel_probes();

protected:
    static void _bind_methods();

public:
    void _ready() override;
    void _physics_process(double p_delta) override;

    void set_vehicle_preset(int32_t value);
    int32_t get_vehicle_preset() const { return vehicle_preset; }

    void set_wheel_track(real_t value) { wheel_track = std::clamp(value, static_cast<real_t>(0.5), static_cast<real_t>(3.5)); _update_wheel_probes(); }
    real_t get_wheel_track() const { return wheel_track; }
    void set_front_axle_offset(real_t value) { front_axle_offset = std::clamp(value, static_cast<real_t>(-4.0), static_cast<real_t>(0.0)); _update_wheel_probes(); }
    real_t get_front_axle_offset() const { return front_axle_offset; }
    void set_rear_axle_offset(real_t value) { rear_axle_offset = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(4.0)); _update_wheel_probes(); }
    real_t get_rear_axle_offset() const { return rear_axle_offset; }
    void set_wheel_mount_height(real_t value) { wheel_mount_height = std::clamp(value, static_cast<real_t>(-1.5), static_cast<real_t>(1.0)); _update_wheel_probes(); }
    real_t get_wheel_mount_height() const { return wheel_mount_height; }
    void set_wheel_radius(real_t value) { wheel_radius = std::clamp(value, static_cast<real_t>(0.1), static_cast<real_t>(1.5)); _update_wheel_probes(); }
    real_t get_wheel_radius() const { return wheel_radius; }

    void set_suspension_rest_length(real_t value) { suspension_rest_length = std::clamp(value, static_cast<real_t>(0.05), static_cast<real_t>(1.5)); _update_wheel_probes(); }
    real_t get_suspension_rest_length() const { return suspension_rest_length; }
    void set_suspension_stiffness(real_t value) { suspension_stiffness = std::clamp(value, static_cast<real_t>(1000.0), static_cast<real_t>(100000.0)); }
    real_t get_suspension_stiffness() const { return suspension_stiffness; }
    void set_suspension_damping(real_t value) { suspension_damping = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(15000.0)); }
    real_t get_suspension_damping() const { return suspension_damping; }

    void set_engine_force(real_t value) { engine_force = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(40000.0)); }
    real_t get_engine_force() const { return engine_force; }
    void set_brake_force(real_t value) { brake_force = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(60000.0)); }
    real_t get_brake_force() const { return brake_force; }
    void set_handbrake_force(real_t value) { handbrake_force = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(60000.0)); }
    real_t get_handbrake_force() const { return handbrake_force; }
    void set_max_steering_angle_degrees(real_t value) { max_steering_angle_degrees = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(60.0)); }
    real_t get_max_steering_angle_degrees() const { return max_steering_angle_degrees; }
    void set_drive_layout(int32_t value) { drive_layout = std::clamp(value, static_cast<int32_t>(FRONT_WHEEL_DRIVE), static_cast<int32_t>(ALL_WHEEL_DRIVE)); }
    int32_t get_drive_layout() const { return drive_layout; }

    void set_tire_friction(real_t value) { tire_friction = std::clamp(value, static_cast<real_t>(0.1), static_cast<real_t>(3.0)); }
    real_t get_tire_friction() const { return tire_friction; }
    void set_lateral_grip(real_t value) { lateral_grip = std::clamp(value, static_cast<real_t>(0.1), static_cast<real_t>(10.0)); }
    real_t get_lateral_grip() const { return lateral_grip; }

    void set_use_input_actions(bool value) { use_input_actions = value; }
    bool get_use_input_actions() const { return use_input_actions; }
    void set_throttle_action(const StringName &value) { throttle_action = value; }
    StringName get_throttle_action() const { return throttle_action; }
    void set_brake_action(const StringName &value) { brake_action = value; }
    StringName get_brake_action() const { return brake_action; }
    void set_steer_left_action(const StringName &value) { steer_left_action = value; }
    StringName get_steer_left_action() const { return steer_left_action; }
    void set_steer_right_action(const StringName &value) { steer_right_action = value; }
    StringName get_steer_right_action() const { return steer_right_action; }
    void set_handbrake_action(const StringName &value) { handbrake_action = value; }
    StringName get_handbrake_action() const { return handbrake_action; }
    void set_wheel_collision_mask(uint32_t value) { wheel_collision_mask = value; _update_wheel_probes(); }
    uint32_t get_wheel_collision_mask() const { return wheel_collision_mask; }

    // Native control entry points for touch controls, AI and future C++ controllers.
    void set_throttle(real_t value) { throttle_input = std::clamp(value, static_cast<real_t>(-1.0), static_cast<real_t>(1.0)); }
    void set_brake(real_t value) { brake_input = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(1.0)); }
    void set_steering(real_t value) { steering_input = std::clamp(value, static_cast<real_t>(-1.0), static_cast<real_t>(1.0)); }
    void set_handbrake(real_t value) { handbrake_input = std::clamp(value, static_cast<real_t>(0.0), static_cast<real_t>(1.0)); }
    real_t get_speed_kph() const { return get_linear_velocity().length() * static_cast<real_t>(3.6); }
    int32_t get_wheel_contact_count() const { return wheel_contact_count; }
};

VARIANT_ENUM_CAST(ArutCar3D::VehiclePreset);
VARIANT_ENUM_CAST(ArutCar3D::DriveLayout);
