#include "arut_car_3d.h"

#include <cmath>

#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/object/class_db.h"
#include "core/variant/variant.h"

void ArutCar3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_vehicle_preset", "value"), &ArutCar3D::set_vehicle_preset);
    ClassDB::bind_method(D_METHOD("get_vehicle_preset"), &ArutCar3D::get_vehicle_preset);
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

    ADD_GROUP("Vehicle Setup", "vehicle_");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "vehicle_preset", PROPERTY_HINT_ENUM, "Custom,Compact,Sedan,Off-road"), "set_vehicle_preset", "get_vehicle_preset");

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

    BIND_ENUM_CONSTANT(CUSTOM);
    BIND_ENUM_CONSTANT(COMPACT);
    BIND_ENUM_CONSTANT(SEDAN);
    BIND_ENUM_CONSTANT(OFFROAD);
    BIND_ENUM_CONSTANT(FRONT_WHEEL_DRIVE);
    BIND_ENUM_CONSTANT(REAR_WHEEL_DRIVE);
    BIND_ENUM_CONSTANT(ALL_WHEEL_DRIVE);
}

void ArutCar3D::set_vehicle_preset(int32_t p_value) {
    vehicle_preset = std::clamp(p_value, static_cast<int32_t>(CUSTOM), static_cast<int32_t>(OFFROAD));

    switch (vehicle_preset) {
        case COMPACT:
            wheel_track = 1.46;
            front_axle_offset = -1.00;
            rear_axle_offset = 1.00;
            wheel_mount_height = -0.12;
            wheel_radius = 0.29;
            suspension_rest_length = 0.34;
            suspension_stiffness = 22000.0;
            suspension_damping = 2700.0;
            engine_force = 6800.0;
            brake_force = 9000.0;
            handbrake_force = 11500.0;
            max_steering_angle_degrees = 34.0;
            tire_friction = 1.0;
            lateral_grip = 1.65;
            break;
        case SEDAN:
            wheel_track = 1.55;
            front_axle_offset = -1.15;
            rear_axle_offset = 1.15;
            wheel_mount_height = -0.15;
            wheel_radius = 0.34;
            suspension_rest_length = 0.42;
            suspension_stiffness = 26000.0;
            suspension_damping = 3200.0;
            engine_force = 8500.0;
            brake_force = 11000.0;
            handbrake_force = 14000.0;
            max_steering_angle_degrees = 32.0;
            tire_friction = 1.05;
            lateral_grip = 1.8;
            break;
        case OFFROAD:
            wheel_track = 1.72;
            front_axle_offset = -1.35;
            rear_axle_offset = 1.35;
            wheel_mount_height = -0.08;
            wheel_radius = 0.47;
            suspension_rest_length = 0.62;
            suspension_stiffness = 34000.0;
            suspension_damping = 4200.0;
            engine_force = 10500.0;
            brake_force = 13500.0;
            handbrake_force = 16000.0;
            max_steering_angle_degrees = 30.0;
            tire_friction = 1.15;
            lateral_grip = 1.6;
            break;
        case CUSTOM:
        default:
            // Keep the user's custom values unchanged.
            break;
    }

    _update_wheel_probes();
}

void ArutCar3D::_ready() {
    static const char *probe_names[4] = {
        "WheelProbeFrontLeft",
        "WheelProbeFrontRight",
        "WheelProbeRearLeft",
        "WheelProbeRearRight"
    };

    for (int32_t i = 0; i < 4; ++i) {
        if (wheel_probes[i] == nullptr) {
            RayCast3D *probe = memnew(RayCast3D);
            probe->set_name(probe_names[i]);
            add_child(probe);
            wheel_probes[i] = probe;
        }

        wheel_probes[i]->set_enabled(true);
        wheel_probes[i]->set_collide_with_bodies(true);
        wheel_probes[i]->set_collide_with_areas(false);
        wheel_probes[i]->set_exclude_parent_body(true);
        wheel_probes[i]->set_collision_mask(wheel_collision_mask);
    }

    _update_wheel_probes();
    set_physics_process(true);
}

void ArutCar3D::_update_wheel_probes() {
    const real_t half_track = wheel_track * static_cast<real_t>(0.5);
    const real_t ray_length = suspension_rest_length + wheel_radius;

    for (int32_t i = 0; i < 4; ++i) {
        RayCast3D *probe = wheel_probes[i];
        if (probe == nullptr) {
            continue;
        }
        const real_t side = (i % 2 == 0) ? -1.0 : 1.0;
        const real_t axle = (i < 2) ? front_axle_offset : rear_axle_offset;
        probe->set_position(Vector3(side * half_track, wheel_mount_height, axle));
        probe->set_target_position(Vector3(0.0, -ray_length, 0.0));
        probe->set_collision_mask(wheel_collision_mask);
    }
}

void ArutCar3D::_physics_process(double p_delta) {
    (void)p_delta;

    if (use_input_actions) {
        Input *input = Input::get_singleton();
        InputMap *input_map = InputMap::get_singleton();
        if (input != nullptr && input_map != nullptr) {
            throttle_input = input_map->has_action(throttle_action) ? input->get_action_strength(throttle_action) : 0.0;
            brake_input = input_map->has_action(brake_action) ? input->get_action_strength(brake_action) : 0.0;
            const real_t left = input_map->has_action(steer_left_action) ? input->get_action_strength(steer_left_action) : 0.0;
            const real_t right = input_map->has_action(steer_right_action) ? input->get_action_strength(steer_right_action) : 0.0;
            steering_input = right - left;
            handbrake_input = input_map->has_action(handbrake_action) ? input->get_action_strength(handbrake_action) : 0.0;
        }
    }

    const Vector3 body_origin = get_global_position();
    const Vector3 body_up = get_global_basis().get_column(1).normalized();
    const Vector3 body_forward = -get_global_basis().get_column(2).normalized();
    const int32_t driven_wheels = (drive_layout == ALL_WHEEL_DRIVE) ? 4 : 2;
    wheel_contact_count = 0;

    for (int32_t i = 0; i < 4; ++i) {
        RayCast3D *probe = wheel_probes[i];
        if (probe == nullptr || !probe->is_enabled()) {
            continue;
        }

        probe->force_raycast_update();
        if (!probe->is_colliding()) {
            continue;
        }
        ++wheel_contact_count;

        const Vector3 contact = probe->get_collision_point();
        const Vector3 normal = probe->get_collision_normal().normalized();
        const Vector3 force_offset = contact - body_origin;
        const Vector3 point_velocity = get_linear_velocity() + get_angular_velocity().cross(force_offset);
        const real_t hit_distance = probe->get_global_position().distance_to(contact);
        const real_t suspension_length = std::clamp(hit_distance - wheel_radius, static_cast<real_t>(0.0), suspension_rest_length);
        const real_t compression = suspension_rest_length - suspension_length;

        real_t spring_force = suspension_stiffness * compression - suspension_damping * point_velocity.dot(body_up);
        spring_force = std::clamp(spring_force, static_cast<real_t>(0.0), suspension_stiffness * suspension_rest_length * static_cast<real_t>(1.5));

        Vector3 wheel_forward = body_forward;
        if (i < 2) {
            const real_t steering_radians = steering_input * max_steering_angle_degrees * static_cast<real_t>(0.017453292519943295);
            wheel_forward = Basis(body_up, steering_radians) * wheel_forward;
        }
        wheel_forward -= normal * wheel_forward.dot(normal);
        if (wheel_forward.length_squared() < static_cast<real_t>(0.0001)) {
            wheel_forward = body_forward;
        } else {
            wheel_forward.normalize();
        }
        Vector3 wheel_right = wheel_forward.cross(normal);
        if (wheel_right.length_squared() > static_cast<real_t>(0.0001)) {
            wheel_right.normalize();
        }

        const real_t longitudinal_speed = point_velocity.dot(wheel_forward);
        const real_t lateral_speed = point_velocity.dot(wheel_right);
        const real_t grip_limit = spring_force * tire_friction;

        real_t lateral_force = -lateral_speed * lateral_grip * get_mass();
        lateral_force = std::clamp(lateral_force, -grip_limit, grip_limit);

        const bool is_driven = drive_layout == ALL_WHEEL_DRIVE ||
            (drive_layout == FRONT_WHEEL_DRIVE && i < 2) ||
            (drive_layout == REAR_WHEEL_DRIVE && i >= 2);
        real_t longitudinal_force = is_driven ? throttle_input * engine_force / driven_wheels : 0.0;

        real_t braking_force = brake_input * brake_force / 4.0;
        if (i >= 2) {
            braking_force += handbrake_input * handbrake_force / 2.0;
        }
        if (std::abs(longitudinal_speed) > static_cast<real_t>(0.05)) {
            longitudinal_force -= std::copysign(braking_force, longitudinal_speed);
        }
        longitudinal_force = std::clamp(longitudinal_force, -grip_limit, grip_limit);

        // Friction ellipse approximation: combined drive/braking and cornering
        // forces are prevented from exceeding the available contact grip.
        const real_t combined_force = std::sqrt(longitudinal_force * longitudinal_force + lateral_force * lateral_force);
        if (combined_force > grip_limit && combined_force > static_cast<real_t>(0.001)) {
            const real_t scale = grip_limit / combined_force;
            longitudinal_force *= scale;
            lateral_force *= scale;
        }

        const Vector3 total_force = body_up * spring_force + wheel_forward * longitudinal_force + wheel_right * lateral_force;
        apply_force(total_force, force_offset);
    }
}
