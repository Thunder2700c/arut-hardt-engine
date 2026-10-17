#include "arut_character_3d.h"

#include "core/input/input.h"
#include "core/input/input_map.h"
#include "core/object/class_db.h"
#include "core/variant/variant.h"

void ArutCharacter3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_walk_speed", "value"), &ArutCharacter3D::set_walk_speed);
    ClassDB::bind_method(D_METHOD("get_walk_speed"), &ArutCharacter3D::get_walk_speed);
    ClassDB::bind_method(D_METHOD("set_sprint_speed", "value"), &ArutCharacter3D::set_sprint_speed);
    ClassDB::bind_method(D_METHOD("get_sprint_speed"), &ArutCharacter3D::get_sprint_speed);
    ClassDB::bind_method(D_METHOD("set_ground_acceleration", "value"), &ArutCharacter3D::set_ground_acceleration);
    ClassDB::bind_method(D_METHOD("get_ground_acceleration"), &ArutCharacter3D::get_ground_acceleration);
    ClassDB::bind_method(D_METHOD("set_air_acceleration", "value"), &ArutCharacter3D::set_air_acceleration);
    ClassDB::bind_method(D_METHOD("get_air_acceleration"), &ArutCharacter3D::get_air_acceleration);
    ClassDB::bind_method(D_METHOD("set_air_control", "value"), &ArutCharacter3D::set_air_control);
    ClassDB::bind_method(D_METHOD("get_air_control"), &ArutCharacter3D::get_air_control);
    ClassDB::bind_method(D_METHOD("set_ground_friction", "value"), &ArutCharacter3D::set_ground_friction);
    ClassDB::bind_method(D_METHOD("get_ground_friction"), &ArutCharacter3D::get_ground_friction);
    ClassDB::bind_method(D_METHOD("set_jump_velocity", "value"), &ArutCharacter3D::set_jump_velocity);
    ClassDB::bind_method(D_METHOD("get_jump_velocity"), &ArutCharacter3D::get_jump_velocity);
    ClassDB::bind_method(D_METHOD("set_gravity", "value"), &ArutCharacter3D::set_gravity);
    ClassDB::bind_method(D_METHOD("get_gravity"), &ArutCharacter3D::get_gravity);

    ClassDB::bind_method(D_METHOD("set_use_input_actions", "value"), &ArutCharacter3D::set_use_input_actions);
    ClassDB::bind_method(D_METHOD("get_use_input_actions"), &ArutCharacter3D::get_use_input_actions);
    ClassDB::bind_method(D_METHOD("set_move_left_action", "value"), &ArutCharacter3D::set_move_left_action);
    ClassDB::bind_method(D_METHOD("get_move_left_action"), &ArutCharacter3D::get_move_left_action);
    ClassDB::bind_method(D_METHOD("set_move_right_action", "value"), &ArutCharacter3D::set_move_right_action);
    ClassDB::bind_method(D_METHOD("get_move_right_action"), &ArutCharacter3D::get_move_right_action);
    ClassDB::bind_method(D_METHOD("set_move_forward_action", "value"), &ArutCharacter3D::set_move_forward_action);
    ClassDB::bind_method(D_METHOD("get_move_forward_action"), &ArutCharacter3D::get_move_forward_action);
    ClassDB::bind_method(D_METHOD("set_move_backward_action", "value"), &ArutCharacter3D::set_move_backward_action);
    ClassDB::bind_method(D_METHOD("get_move_backward_action"), &ArutCharacter3D::get_move_backward_action);
    ClassDB::bind_method(D_METHOD("set_jump_action", "value"), &ArutCharacter3D::set_jump_action);
    ClassDB::bind_method(D_METHOD("get_jump_action"), &ArutCharacter3D::get_jump_action);
    ClassDB::bind_method(D_METHOD("set_sprint_action", "value"), &ArutCharacter3D::set_sprint_action);
    ClassDB::bind_method(D_METHOD("get_sprint_action"), &ArutCharacter3D::get_sprint_action);

    ClassDB::bind_method(D_METHOD("set_move_input", "value"), &ArutCharacter3D::set_move_input);
    ClassDB::bind_method(D_METHOD("get_move_input"), &ArutCharacter3D::get_move_input);
    ClassDB::bind_method(D_METHOD("request_jump"), &ArutCharacter3D::request_jump);
    ClassDB::bind_method(D_METHOD("set_sprint_active", "active"), &ArutCharacter3D::set_sprint_active);

    ADD_GROUP("Movement", "movement_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_walk_speed", PROPERTY_HINT_RANGE, "0,50,0.1,suffix:m/s"), "set_walk_speed", "get_walk_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_sprint_speed", PROPERTY_HINT_RANGE, "0,70,0.1,suffix:m/s"), "set_sprint_speed", "get_sprint_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_ground_acceleration", PROPERTY_HINT_RANGE, "0.1,100,0.1,suffix:m/s²"), "set_ground_acceleration", "get_ground_acceleration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_air_acceleration", PROPERTY_HINT_RANGE, "0,50,0.1,suffix:m/s²"), "set_air_acceleration", "get_air_acceleration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_air_control", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_air_control", "get_air_control");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_ground_friction", PROPERTY_HINT_RANGE, "0,100,0.1"), "set_ground_friction", "get_ground_friction");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_jump_velocity", PROPERTY_HINT_RANGE, "0,30,0.1,suffix:m/s"), "set_jump_velocity", "get_jump_velocity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_gravity", PROPERTY_HINT_RANGE, "0,100,0.1,suffix:m/s²"), "set_gravity", "get_gravity");

    ADD_GROUP("Controls", "control_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "control_use_input_actions"), "set_use_input_actions", "get_use_input_actions");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_move_left_action"), "set_move_left_action", "get_move_left_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_move_right_action"), "set_move_right_action", "get_move_right_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_move_forward_action"), "set_move_forward_action", "get_move_forward_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_move_backward_action"), "set_move_backward_action", "get_move_backward_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_jump_action"), "set_jump_action", "get_jump_action");
    ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "control_sprint_action"), "set_sprint_action", "get_sprint_action");
}

void ArutCharacter3D::_ready() {
    set_physics_process(true);
}

void ArutCharacter3D::_physics_process(double p_delta) {
    const real_t delta = static_cast<real_t>(p_delta);

    if (use_input_actions) {
        Input *input = Input::get_singleton();
        InputMap *input_map = InputMap::get_singleton();
        if (input != nullptr && input_map != nullptr) {
            const real_t left = input_map->has_action(move_left_action) ? input->get_action_strength(move_left_action) : 0.0;
            const real_t right = input_map->has_action(move_right_action) ? input->get_action_strength(move_right_action) : 0.0;
            const real_t forward = input_map->has_action(move_forward_action) ? input->get_action_strength(move_forward_action) : 0.0;
            const real_t backward = input_map->has_action(move_backward_action) ? input->get_action_strength(move_backward_action) : 0.0;

            move_input = Vector2(right - left, forward - backward);
            if (move_input.length() > 1.0) {
                move_input.normalize();
            }
            if (input_map->has_action(jump_action) && input->is_action_just_pressed(jump_action)) {
                jump_requested = true;
            }
            sprint_active = input_map->has_action(sprint_action) && input->is_action_pressed(sprint_action);
        }
    }

    Vector3 desired_direction;
    if (move_input.length_squared() > 0.0001) {
        const Basis body_basis = get_global_basis();
        const Vector3 right = body_basis.get_column(0);
        const Vector3 forward = -body_basis.get_column(2);
        desired_direction = right * move_input.x + forward * move_input.y;
        desired_direction.y = 0.0;
        if (desired_direction.length_squared() > 0.0001) {
            desired_direction.normalize();
        }
    }

    Vector3 velocity = get_velocity();
    Vector3 horizontal_velocity(velocity.x, 0.0, velocity.z);
    const real_t target_speed = sprint_active ? sprint_speed : walk_speed;
    const Vector3 target_horizontal_velocity = desired_direction * target_speed;

    if (is_on_floor()) {
        if (desired_direction.length_squared() > 0.0001) {
            horizontal_velocity = horizontal_velocity.move_toward(target_horizontal_velocity, ground_acceleration * delta);
        } else {
            horizontal_velocity = horizontal_velocity.move_toward(Vector3(), ground_friction * delta);
        }
        if (velocity.y < 0.0) {
            velocity.y = 0.0;
        }
        if (jump_requested) {
            velocity.y = jump_velocity;
        }
        jump_requested = false;
    } else if (desired_direction.length_squared() > 0.0001) {
        horizontal_velocity = horizontal_velocity.move_toward(target_horizontal_velocity, air_acceleration * air_control * delta);
        // A jump request is consumed once airborne; there is no hidden buffered jump yet.
        jump_requested = false;
    } else {
        jump_requested = false;
    }

    if (!is_on_floor()) {
        velocity.y -= gravity * delta;
    }

    velocity.x = horizontal_velocity.x;
    velocity.z = horizontal_velocity.z;
    set_velocity(velocity);
    move_and_slide();
}
