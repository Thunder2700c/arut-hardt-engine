#include "arut_car_3d.h"

#include <algorithm>
#include <cmath>

#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_map.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

ArutCar3D::ArutCar3D() {
}

void ArutCar3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_wheel_track", "value"), &ArutCar3D::set_wheel_track);
    ClassDB::bind_method(D_METHOD("get_wheel_track"), &ArutCar3D::get_wheel_track);
    ClassDB::bind_method(D_METHOD("set_front_axle_offset", "value"), &ArutCar3D::set_front_axle_offset);
    ClassDB::bind_method(D_METHOD("get_front_axle_offset"), &ArutCar3D::get_front_axle_offset);
    ClassDB::bind_method(D_METHOD("set_rear_axle_offset", "value"), &ArutCar3D::set_rear_axle_offset);
    ClassDB::bind_method(D_METHOD("get_rear_axle_offset"), &ArutCar3D::get_rear_axle_offset);
    ClassDB::bind_method(D_METHOD("set_wheel_mount_height", "value"), &ArutCar3D::set_wheel_mount_height);
    ClassDB::bind_method(D_METHOD("get_wheel_mount_height"), &ArutCar3D::get_wheel_mount_height);
    ClassDB::bind_method(D_METHOD("set_wheel_radius", "value"), &ArutCar3D::set_wheel_radius);
    ClassDB::bind_method(D_METHOD("get_wheel_radius"), &ArutCar3D::get_wheel_radius);

    ClassDB::bind_method(D_METHOD("set_suspension_rest_length", "value"), &ArutCar3D::set_suspension_rest_length);
    ClassDB::bind_method(D_METHOD("get_suspension_rest_length"), &ArutCar3D::get_suspension_rest_length);
    ClassDB::bind_method(D_METHOD("set_suspension_stiffness", "value"), &ArutCar3D::set_suspension_stiffness);
    ClassDB::bind_method(D_METHOD("get_suspension_stiffness"), &ArutCar3D::get_suspension_stiffness);
    ClassDB::bind_method(D_METHOD("set_suspension_damping", "value"), &ArutCar3D::set_suspension_damping);
    ClassDB::bind_method(D_METHOD("get_suspension_damping"), &ArutCar3D::get_suspension_damping);

    ClassDB::bind_method(D_METHOD("set_engine_force", "value"), &ArutCar3D::set_engine_force);
    ClassDB::bind_method(D_METHOD("get_engine_force"), &ArutCar3D::get_engine_force);
    ClassDB::bind_method(D_METHOD("set_brake_force", "value"), &ArutCar3D::set_brake_force);
    ClassDB::bind_method(D_METHOD("get_brake_force"), &ArutCar3D::get_brake_force);
    ClassDB::bind_method(D_METHOD("set_handbrake_force", "value"), &ArutCar3D::set_handbrake_force);
    ClassDB::bind_method(D_METHOD("get_handbrake_force"), &ArutCar3D::get_handbrake_force);
    ClassDB::bind_method(D_METHOD("set_max_steering_angle_degrees", "value"), &ArutCar3D::set_max_steering_angle_degrees);
    ClassDB::bind_method(D_METHOD("get_max_steering_angle_degrees"), &ArutCar3D::get_max_steering_angle_degrees);
    ClassDB::bind_method(D_METHOD("set_drive_layout", "value"), &ArutCar3D::set_drive_layout);
    ClassDB::bind_method(D_METHOD("get_drive_layout"), &ArutCar3D::get_drive_layout);

    ClassDB::bind_method(D_METHOD("set_tire_friction", "value"), &ArutCar3D::set_tire_friction);
    ClassDB::bind_method(D_METHOD("get_tire_friction"), &ArutCar3D::get_tire_friction);
    ClassDB::bind_method(D_METHOD("set_lateral_grip", "value"), &ArutCar3D::set_lateral_grip);
    ClassDB::bind_method(D_METHOD("get_lateral_grip"), &ArutCar3D::get_lateral_grip);

    ClassDB::bind_method(D_METHOD("set_use_input_actions", "value"), &ArutCar3D::set_use_input_actions);
    ClassDB::bind_method(D_METHOD("get_use_input_actions"), &ArutCar3D::get_use_input_actions);
    ClassDB::bind_method(D_METHOD("set_throttle_action", "value"), &ArutCar3D::set_throttle_action);
    ClassDB::bind_method(D_METHOD("get_throttle_action"), &ArutCar3D::get_throttle_action);
    ClassDB::bind_method(D_METHOD("set_brake_action", "value"), &ArutCar3D::set_brake_action);
    ClassDB::bind_method(D_METHOD("get_brake_action"), &ArutCar3D::get_brake_action);
    ClassDB::bind_method(D_METHOD("set_steer_left_action", "value"), &ArutCar3D::set_steer_left_action);
    ClassDB::bind_method(D_METHOD("get_steer_left_action"), &ArutCar3D::get_steer_left_action);
    ClassDB::bind_method(D_METHOD("set_steer_right_action", "value"), &ArutCar3D::set_steer_right_action);
    ClassDB::bind_method(D_METHOD("get_steer_right_action"), &ArutCar3D::get_steer_right_action);
    ClassDB::bind_method(D_METHOD("set_handbrake_action", "value"), &ArutCar3D::set_handbrake_action);
    ClassDB::bind_method(D_METHOD("get_handbrake_action"), &ArutCar3D::get_handbrake_action);
    ClassDB::bind_method(D_METHOD("set_wheel_collision_mask", "value"), &ArutCar3D::set_wheel_collision_mask);
    ClassDB::bind_method(D_METHOD("get_wheel_collision_mask"), &ArutCar3D::get_wheel_collision_mask);

    ClassDB::bind_method(D_METHOD("set_throttle", "value"), &ArutCar3D::set_throttle);
    ClassDB::bind_method(D_METHOD("set_brake", "value"), &ArutCar3D::set_brake);
    ClassDB::bind_method(D_METHOD("set_steering", "value"), &ArutCar3D::set_steering);
    ClassDB::bind_method(D_METHOD("set_handbrake", "value"), &ArutCar3D::set_handbrake);
    ClassDB::bind_method(D_METHOD("get_speed_kph"), &ArutCar3D::get_speed_kph);
    ClassDB::bind_method(D_METHOD("get_wheel_contact_count"), &ArutCar3D::get_wheel_contact_count);

    ADD_GROUP("Wheel Layout", "wheel_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wheel_track", PROPERTY_HINT_RANGE, "0.5,3.5,0.01,or_greater,suffix:m"), "set_wheel_track", "get_wheel_track");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wheel_front_axle_offset", PROPERTY_HINT_RANGE, "-4.0,0.0,0.01,suffix:m"), "set_front_axle_offset", "get_front_axle_offset");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wheel_rear_axle_offset", PROPERTY_HINT_RANGE, "0.0,4.0,0.01,suffix:m"), "set_rear_axle_offset", "get_rear_axle_offset");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wheel_mount_height", PROPERTY_HINT_RANGE, "-1.5,1.0,0.01,suffix:m"), "set_wheel_mount_height", "get_wheel_mount_height");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wheel_radius", PROPERTY_HINT_RANGE, "0.1,1.5,0.01,suffix:m"), "set_wheel_radius", "get_wheel_radius");

    ADD_GROUP("Suspension", "suspension_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "suspension_rest_length", PROPERTY_HINT_RANGE, "0.05,1.5,0.01,suffix:m"), "set_suspension_rest_length", "get_suspension_rest_length");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "suspension_stiffness", PROPERTY_HINT_RANGE, "1000,100000,100,suffix:N/m"), "set_suspension_stiffness", "get_suspension_stiffness");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "suspension_damping", PROPERTY_HINT_RANGE, "0,15000,50,suffix:N*s/m"), "set_suspension_damping", "get_suspension_damping");

    ADD_GROUP("Drive", "drive_");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "drive_layout", PROPERTY_HINT_ENUM, "Front Wheel Drive,Rear Wheel Drive,All Wheel Drive"), "set_drive_layout", "get_drive_layout");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drive_engine_force", PROPERTY_HINT_RANGE, "0,40000,100,suffix:N"), "set_engine_force", "get_engine_force");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drive_brake_force", PROPERTY_HINT_RANGE, "0,60000,100,suffix:N"), "set_brake_force", "get_brake_force");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drive_handbrake_force", PROPERTY_HINT_RANGE, "0,60000,100,suffix:N"), "set_handbrake_force", "get_handbrake_force");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "drive_max_steering_angle_degrees", PROPERTY_HINT_RANGE, "0,60,1,suffix:deg"), "set_max_steering_angle_degrees", "get_max_steering_angle_degrees");

    ADD_GROUP("Tires", "tire_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tire_friction", PROPERTY_HINT_RANGE, "0.1,3.0,0.01"), "set_tire_friction", "get_tire_friction");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "tire_lateral_grip", PROPERTY_HINT_RANGE, "0.1,10.0,0.1"), "set_lateral_grip", "get_lateral_grip");

    ADD_GROUP("Controls", "control_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "control_use_input_actions"), "set_use_input_actions", "get_use_input_actions");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_throttle_action"), "set_throttle_action", "get_throttle_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_brake_action"), "set_brake_action", "get_brake_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_steer_left_action"), "set_steer_left_action", "get_steer_left_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_steer_right_action"), "set_steer_right_action", "get_steer_right_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_handbrake_action"), "set_handbrake_action", "get_handbrake_action");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "control_wheel_collision_mask", PROPERTY_HINT_LAYERS_3D_PHYSICS), "set_wheel_collision_mask", "get_wheel_collision_mask");

    BIND_ENUM_CONSTANT(FRONT_WHEEL_DRIVE);
    BIND_ENUM_CONSTANT(REAR_WHEEL_DRIVE);
    BIND_ENUM_CONSTANT(ALL_WHEEL_DRIVE);
}

void ArutCar3D::_ready() {
    static const char *probe_names[4] = {
        "WheelProbeFrontLeft",
        "WheelProbeFrontRight",
        "WheelProbeRearLeft",
        "WheelProbeRearRight"
    };

    for (int32_t i = 0; i < 4; ++i) {
        if (_wheel_probes[i] == nullptr) {
            RayCast3D *probe = memnew(RayCast3D);
            probe->set_name(StringName(probe_names[i]));
            add_child(probe);
            _wheel_probes[i] = probe;
        }
        _wheel_probes[i]->set_enabled(true);
        _wheel_probes[i]->set_collide_with_bodies(true);
        _wheel_probes[i]->set_collide_with_areas(false);
        _wheel_probes[i]->set_exclude_parent_body(true);
        _wheel_probes[i]->set_collision_mask(static_cast<uint32_t>(_wheel_collision_mask));
    }

    _update_wheel_probes();
    set_physics_process(true);
}

void ArutCar3D::_update_wheel_probes() {
    const real_t half_track = _wheel_track * 0.5;
    const real_t ray_length = _suspension_rest_length + _wheel_radius;

    for (int32_t i = 0; i < 4; ++i) {
        RayCast3D *probe = _wheel_probes[i];
        if (probe == nullptr) {
            continue;
        }

        const real_t side = (i % 2 == 0) ? -1.0 : 1.0;
        const real_t axle = (i < 2) ? _front_axle_offset : _rear_axle_offset;
        probe->set_position(Vector3(side * half_track, _wheel_mount_height, axle));
        probe->set_target_position(Vector3(0.0, -ray_length, 0.0));
        probe->set_collision_mask(static_cast<uint32_t>(_wheel_collision_mask));
    }
}

void ArutCar3D::_physics_process(double p_delta) {
    (void)p_delta;

    if (_use_input_actions) {
        Input *input = Input::get_singleton();
        InputMap *input_map = InputMap::get_singleton();
        if (input != nullptr && input_map != nullptr) {
            _throttle = input_map->has_action(_throttle_action) ? input->get_action_strength(_throttle_action) : 0.0;
            _brake = input_map->has_action(_brake_action) ? input->get_action_strength(_brake_action) : 0.0;
            const real_t steer_left = input_map->has_action(_steer_left_action) ? input->get_action_strength(_steer_left_action) : 0.0;
            const real_t steer_right = input_map->has_action(_steer_right_action) ? input->get_action_strength(_steer_right_action) : 0.0;
            _steering = steer_right - steer_left;
            _handbrake = input_map->has_action(_handbrake_action) ? input->get_action_strength(_handbrake_action) : 0.0;
        }
    }

    const Vector3 origin = get_global_position();
    const Vector3 body_up = get_global_basis().get_column(1).normalized();
    const Vector3 body_forward = -get_global_basis().get_column(2).normalized();
    const int32_t driven_wheels = (_drive_layout == ALL_WHEEL_DRIVE) ? 4 : 2;
    _wheel_contact_count = 0;

    for (int32_t i = 0; i < 4; ++i) {
        RayCast3D *probe = _wheel_probes[i];
        if (probe == nullptr || !probe->is_enabled()) {
            continue;
        }

        probe->force_raycast_update();
        if (!probe->is_colliding()) {
            continue;
        }

        ++_wheel_contact_count;

        const Vector3 contact = probe->get_collision_point();
        const Vector3 normal = probe->get_collision_normal().normalized();
        const Vector3 lever = contact - origin;
        const Vector3 point_velocity = get_linear_velocity() + get_angular_velocity().cross(lever);
        const real_t ray_distance = probe->get_global_position().distance_to(contact);
        const real_t suspension_length = std::clamp(ray_distance - _wheel_radius, static_cast<real_t>(0.0), _suspension_rest_length);
        const real_t compression = _suspension_rest_length - suspension_length;

        real_t spring_force = _suspension_stiffness * compression - _suspension_damping * point_velocity.dot(body_up);
        spring_force = std::clamp(spring_force, static_cast<real_t>(0.0), _suspension_stiffness * _suspension_rest_length * 1.5);

        Vector3 wheel_forward = body_forward;
        if (i < 2) {
            const real_t steering_radians = _steering * _max_steering_angle_degrees * static_cast<real_t>(Math_PI / 180.0);
            wheel_forward = Basis(body_up, steering_radians) * wheel_forward;
        }
        wheel_forward -= normal * wheel_forward.dot(normal);
        if (wheel_forward.length_squared() < 0.0001) {
            wheel_forward = body_forward;
        } else {
            wheel_forward = wheel_forward.normalized();
        }
        const Vector3 wheel_right = wheel_forward.cross(normal).normalized();

        const real_t longitudinal_speed = point_velocity.dot(wheel_forward);
        const real_t lateral_speed = point_velocity.dot(wheel_right);
        const real_t grip_limit = spring_force * _tire_friction;
        real_t lateral_force = -lateral_speed * _lateral_grip * get_mass();
        lateral_force = std::clamp(lateral_force, -grip_limit, grip_limit);

        const bool is_driven = (_drive_layout == ALL_WHEEL_DRIVE) ||
            (_drive_layout == FRONT_WHEEL_DRIVE && i < 2) ||
            (_drive_layout == REAR_WHEEL_DRIVE && i >= 2);
        real_t longitudinal_force = is_driven ? (_throttle * _engine_force / driven_wheels) : 0.0;

        real_t braking = _brake * _brake_force / 4.0;
        if (i >= 2) {
            braking += _handbrake * _handbrake_force / 2.0;
        }
        if (std::abs(longitudinal_speed) > 0.05) {
            longitudinal_force -= std::copysign(braking, longitudinal_speed);
        }
        longitudinal_force = std::clamp(longitudinal_force, -grip_limit, grip_limit);

        const real_t combined_force = std::sqrt(longitudinal_force * longitudinal_force + lateral_force * lateral_force);
        if (combined_force > grip_limit && combined_force > 0.001) {
            const real_t scale = grip_limit / combined_force;
            longitudinal_force *= scale;
            lateral_force *= scale;
        }

        const Vector3 total_force = body_up * spring_force + wheel_forward * longitudinal_force + wheel_right * lateral_force;
        apply_force(total_force, lever);
    }
}

// Property accessors: geometry setters refresh the generated wheel probes.
void ArutCar3D::set_wheel_track(real_t p_value) { _wheel_track = std::clamp(p_value, static_cast<real_t>(0.5), static_cast<real_t>(3.5)); _update_wheel_probes(); }
real_t ArutCar3D::get_wheel_track() const { return _wheel_track; }
void ArutCar3D::set_front_axle_offset(real_t p_value) { _front_axle_offset = std::clamp(p_value, static_cast<real_t>(-4.0), static_cast<real_t>(0.0)); _update_wheel_probes(); }
real_t ArutCar3D::get_front_axle_offset() const { return _front_axle_offset; }
void ArutCar3D::set_rear_axle_offset(real_t p_value) { _rear_axle_offset = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(4.0)); _update_wheel_probes(); }
real_t ArutCar3D::get_rear_axle_offset() const { return _rear_axle_offset; }
void ArutCar3D::set_wheel_mount_height(real_t p_value) { _wheel_mount_height = std::clamp(p_value, static_cast<real_t>(-1.5), static_cast<real_t>(1.0)); _update_wheel_probes(); }
real_t ArutCar3D::get_wheel_mount_height() const { return _wheel_mount_height; }
void ArutCar3D::set_wheel_radius(real_t p_value) { _wheel_radius = std::clamp(p_value, static_cast<real_t>(0.1), static_cast<real_t>(1.5)); _update_wheel_probes(); }
real_t ArutCar3D::get_wheel_radius() const { return _wheel_radius; }

void ArutCar3D::set_suspension_rest_length(real_t p_value) { _suspension_rest_length = std::clamp(p_value, static_cast<real_t>(0.05), static_cast<real_t>(1.5)); _update_wheel_probes(); }
real_t ArutCar3D::get_suspension_rest_length() const { return _suspension_rest_length; }
void ArutCar3D::set_suspension_stiffness(real_t p_value) { _suspension_stiffness = std::clamp(p_value, static_cast<real_t>(1000.0), static_cast<real_t>(100000.0)); }
real_t ArutCar3D::get_suspension_stiffness() const { return _suspension_stiffness; }
void ArutCar3D::set_suspension_damping(real_t p_value) { _suspension_damping = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(15000.0)); }
real_t ArutCar3D::get_suspension_damping() const { return _suspension_damping; }

void ArutCar3D::set_engine_force(real_t p_value) { _engine_force = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(40000.0)); }
real_t ArutCar3D::get_engine_force() const { return _engine_force; }
void ArutCar3D::set_brake_force(real_t p_value) { _brake_force = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(60000.0)); }
real_t ArutCar3D::get_brake_force() const { return _brake_force; }
void ArutCar3D::set_handbrake_force(real_t p_value) { _handbrake_force = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(60000.0)); }
real_t ArutCar3D::get_handbrake_force() const { return _handbrake_force; }
void ArutCar3D::set_max_steering_angle_degrees(real_t p_value) { _max_steering_angle_degrees = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(60.0)); }
real_t ArutCar3D::get_max_steering_angle_degrees() const { return _max_steering_angle_degrees; }
void ArutCar3D::set_drive_layout(int32_t p_value) { _drive_layout = std::clamp(p_value, static_cast<int32_t>(FRONT_WHEEL_DRIVE), static_cast<int32_t>(ALL_WHEEL_DRIVE)); }
int32_t ArutCar3D::get_drive_layout() const { return _drive_layout; }

void ArutCar3D::set_tire_friction(real_t p_value) { _tire_friction = std::clamp(p_value, static_cast<real_t>(0.1), static_cast<real_t>(3.0)); }
real_t ArutCar3D::get_tire_friction() const { return _tire_friction; }
void ArutCar3D::set_lateral_grip(real_t p_value) { _lateral_grip = std::clamp(p_value, static_cast<real_t>(0.1), static_cast<real_t>(10.0)); }
real_t ArutCar3D::get_lateral_grip() const { return _lateral_grip; }

void ArutCar3D::set_use_input_actions(bool p_value) { _use_input_actions = p_value; }
bool ArutCar3D::get_use_input_actions() const { return _use_input_actions; }
void ArutCar3D::set_throttle_action(const StringName &p_value) { _throttle_action = p_value; }
StringName ArutCar3D::get_throttle_action() const { return _throttle_action; }
void ArutCar3D::set_brake_action(const StringName &p_value) { _brake_action = p_value; }
StringName ArutCar3D::get_brake_action() const { return _brake_action; }
void ArutCar3D::set_steer_left_action(const StringName &p_value) { _steer_left_action = p_value; }
StringName ArutCar3D::get_steer_left_action() const { return _steer_left_action; }
void ArutCar3D::set_steer_right_action(const StringName &p_value) { _steer_right_action = p_value; }
StringName ArutCar3D::get_steer_right_action() const { return _steer_right_action; }
void ArutCar3D::set_handbrake_action(const StringName &p_value) { _handbrake_action = p_value; }
StringName ArutCar3D::get_handbrake_action() const { return _handbrake_action; }
void ArutCar3D::set_wheel_collision_mask(int64_t p_value) {
    _wheel_collision_mask = std::clamp(p_value, static_cast<int64_t>(1), static_cast<int64_t>(UINT32_MAX));
    _update_wheel_probes();
}
int64_t ArutCar3D::get_wheel_collision_mask() const { return _wheel_collision_mask; }

void ArutCar3D::set_throttle(real_t p_value) { _throttle = std::clamp(p_value, static_cast<real_t>(-1.0), static_cast<real_t>(1.0)); }
void ArutCar3D::set_brake(real_t p_value) { _brake = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(1.0)); }
void ArutCar3D::set_steering(real_t p_value) { _steering = std::clamp(p_value, static_cast<real_t>(-1.0), static_cast<real_t>(1.0)); }
void ArutCar3D::set_handbrake(real_t p_value) { _handbrake = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(1.0)); }
real_t ArutCar3D::get_speed_kph() const { return get_linear_velocity().length() * static_cast<real_t>(3.6); }
int32_t ArutCar3D::get_wheel_contact_count() const { return _wheel_contact_count; }

} // namespace godot
