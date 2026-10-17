#include "arut_character_3d.h"

#include "arut_ragdoll_dummy_3d.h"
#include "core/input/input.h"
#include "core/input/input_event.h"
#include "core/input/input_map.h"
#include "core/math/color.h"
#include "core/object/class_db.h"
#include "core/os/keyboard.h"
#include "core/variant/variant.h"
#include "scene/2d/canvas_layer.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/gui/color_rect.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/main/scene_tree.h"
#include "scene/main/viewport.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/standard_material_3d.h"

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

    ClassDB::bind_method(D_METHOD("set_create_default_dummy", "value"), &ArutCharacter3D::set_create_default_dummy);
    ClassDB::bind_method(D_METHOD("get_create_default_dummy"), &ArutCharacter3D::get_create_default_dummy);
    ClassDB::bind_method(D_METHOD("set_create_default_collision", "value"), &ArutCharacter3D::set_create_default_collision);
    ClassDB::bind_method(D_METHOD("get_create_default_collision"), &ArutCharacter3D::get_create_default_collision);
    ClassDB::bind_method(D_METHOD("set_create_default_camera", "value"), &ArutCharacter3D::set_create_default_camera);
    ClassDB::bind_method(D_METHOD("get_create_default_camera"), &ArutCharacter3D::get_create_default_camera);
    ClassDB::bind_method(D_METHOD("set_enable_ragdoll", "value"), &ArutCharacter3D::set_enable_ragdoll);
    ClassDB::bind_method(D_METHOD("get_enable_ragdoll"), &ArutCharacter3D::get_enable_ragdoll);
    ClassDB::bind_method(D_METHOD("set_enable_mobile_touch_controls", "value"), &ArutCharacter3D::set_enable_mobile_touch_controls);
    ClassDB::bind_method(D_METHOD("get_enable_mobile_touch_controls"), &ArutCharacter3D::get_enable_mobile_touch_controls);
    ClassDB::bind_method(D_METHOD("set_camera_distance", "value"), &ArutCharacter3D::set_camera_distance);
    ClassDB::bind_method(D_METHOD("get_camera_distance"), &ArutCharacter3D::get_camera_distance);
    ClassDB::bind_method(D_METHOD("set_touch_look_sensitivity", "value"), &ArutCharacter3D::set_touch_look_sensitivity);
    ClassDB::bind_method(D_METHOD("get_touch_look_sensitivity"), &ArutCharacter3D::get_touch_look_sensitivity);

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
    ClassDB::bind_method(D_METHOD("start_ragdoll"), &ArutCharacter3D::start_ragdoll);
    ClassDB::bind_method(D_METHOD("is_ragdoll_started"), &ArutCharacter3D::is_ragdoll_started);

    ADD_GROUP("Movement", "movement_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_walk_speed", PROPERTY_HINT_RANGE, "0,50,0.1,suffix:m/s"), "set_walk_speed", "get_walk_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_sprint_speed", PROPERTY_HINT_RANGE, "0,70,0.1,suffix:m/s"), "set_sprint_speed", "get_sprint_speed");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_ground_acceleration", PROPERTY_HINT_RANGE, "0.1,100,0.1,suffix:m/s^2"), "set_ground_acceleration", "get_ground_acceleration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_air_acceleration", PROPERTY_HINT_RANGE, "0,50,0.1,suffix:m/s^2"), "set_air_acceleration", "get_air_acceleration");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_air_control", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_air_control", "get_air_control");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_ground_friction", PROPERTY_HINT_RANGE, "0,100,0.1"), "set_ground_friction", "get_ground_friction");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_jump_velocity", PROPERTY_HINT_RANGE, "0,30,0.1,suffix:m/s"), "set_jump_velocity", "get_jump_velocity");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "movement_gravity", PROPERTY_HINT_RANGE, "0,100,0.1,suffix:m/s^2"), "set_gravity", "get_gravity");

    ADD_GROUP("Built-in Character", "character_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "character_create_default_dummy"), "set_create_default_dummy", "get_create_default_dummy");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "character_create_default_collision"), "set_create_default_collision", "get_create_default_collision");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "character_create_default_camera"), "set_create_default_camera", "get_create_default_camera");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "character_enable_ragdoll"), "set_enable_ragdoll", "get_enable_ragdoll");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "character_enable_mobile_touch_controls"), "set_enable_mobile_touch_controls", "get_enable_mobile_touch_controls");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "character_camera_distance", PROPERTY_HINT_RANGE, "0,12,0.1,suffix:m"), "set_camera_distance", "get_camera_distance");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "character_touch_look_sensitivity", PROPERTY_HINT_RANGE, "0.001,0.02,0.001"), "set_touch_look_sensitivity", "get_touch_look_sensitivity");

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
    set_process_input(true);

    // The basic character must work as soon as it is added to a scene.
    if (create_default_collision) {
        bool has_direct_collision = false;
        for (int32_t i = 0; i < get_child_count(); ++i) {
            if (Object::cast_to<CollisionShape3D>(get_child(i)) != nullptr) {
                has_direct_collision = true;
                break;
            }
        }
        if (!has_direct_collision) {
            _create_default_collision();
        }
    }

    if (create_default_dummy) {
        bool has_direct_visual = false;
        for (int32_t i = 0; i < get_child_count(); ++i) {
            Node *child = get_child(i);
            if (Object::cast_to<MeshInstance3D>(child) != nullptr || child->get_name() == StringName("Visual") || child->get_name() == StringName("DefaultDummyVisual")) {
                has_direct_visual = true;
                break;
            }
        }
        if (!has_direct_visual) {
            _create_default_dummy();
        }
    }

    if (create_default_camera) {
        bool has_camera = false;
        for (int32_t i = 0; i < get_child_count(); ++i) {
            if (Object::cast_to<Camera3D>(get_child(i)) != nullptr) {
                has_camera = true;
                break;
            }
        }
        if (!has_camera) {
            _create_default_camera();
        }
    }

    if (enable_ragdoll) {
        ragdoll_dummy = memnew(ArutRagdollDummy3D);
        ragdoll_dummy->set_name("ArutRagdollDummy");
        ragdoll_dummy->set_start_simulating(false);
        add_child(ragdoll_dummy);
    }

    if (enable_mobile_touch_controls) {
        _create_touch_hint_overlay();
    }
}

void ArutCharacter3D::_create_default_collision() {
    Ref<CapsuleShape3D> capsule;
    capsule.instantiate();
    capsule->set_radius(0.28);
    capsule->set_height(1.78);

    default_collision_shape = memnew(CollisionShape3D);
    default_collision_shape->set_name("ArutDefaultCollision");
    default_collision_shape->set_position(Vector3(0.0, 0.89, 0.0));
    default_collision_shape->set_shape(capsule);
    add_child(default_collision_shape);
}

void ArutCharacter3D::_create_default_dummy() {
    default_visual_root = memnew(Node3D);
    default_visual_root->set_name("DefaultDummyVisual");
    add_child(default_visual_root);

    Ref<StandardMaterial3D> shirt;
    shirt.instantiate();
    shirt->set_albedo(Color(0.06, 0.66, 0.58));
    shirt->set_roughness(0.78);

    Ref<StandardMaterial3D> skin;
    skin.instantiate();
    skin->set_albedo(Color(0.78, 0.53, 0.38));
    skin->set_roughness(0.86);

    Ref<StandardMaterial3D> pants;
    pants.instantiate();
    pants->set_albedo(Color(0.10, 0.15, 0.25));
    pants->set_roughness(0.84);

    Ref<StandardMaterial3D> shoes;
    shoes.instantiate();
    shoes->set_albedo(Color(0.035, 0.045, 0.06));
    shoes->set_roughness(0.7);

    auto add_capsule = [this](const StringName &p_name, real_t p_radius, real_t p_height, const Vector3 &p_position, real_t p_rotation_z, const Ref<StandardMaterial3D> &p_material) {
        Ref<CapsuleMesh> mesh;
        mesh.instantiate();
        mesh->set_radius(p_radius);
        mesh->set_height(MAX(p_height, p_radius * 2.0));
        MeshInstance3D *part = memnew(MeshInstance3D);
        part->set_name(p_name);
        part->set_mesh(mesh);
        part->set_material_override(p_material);
        part->set_position(p_position);
        part->set_rotation(Vector3(0.0, 0.0, p_rotation_z));
        default_visual_root->add_child(part);
    };

    auto add_sphere = [this](const StringName &p_name, real_t p_radius, const Vector3 &p_position, const Ref<StandardMaterial3D> &p_material) {
        Ref<SphereMesh> mesh;
        mesh.instantiate();
        mesh->set_radius(p_radius);
        MeshInstance3D *part = memnew(MeshInstance3D);
        part->set_name(p_name);
        part->set_mesh(mesh);
        part->set_material_override(p_material);
        part->set_position(p_position);
        default_visual_root->add_child(part);
    };

    auto add_box = [this](const StringName &p_name, const Vector3 &p_size, const Vector3 &p_position, const Ref<StandardMaterial3D> &p_material) {
        Ref<BoxMesh> mesh;
        mesh.instantiate();
        mesh->set_size(p_size);
        MeshInstance3D *part = memnew(MeshInstance3D);
        part->set_name(p_name);
        part->set_mesh(mesh);
        part->set_material_override(p_material);
        part->set_position(p_position);
        default_visual_root->add_child(part);
    };

    add_capsule("Torso", 0.225, 0.64, Vector3(0.0, 1.30, 0.0), 0.0, shirt);
    add_box("Pelvis", Vector3(0.38, 0.25, 0.25), Vector3(0.0, 0.91, 0.0), pants);
    add_sphere("Head", 0.165, Vector3(0.0, 1.86, 0.0), skin);
    add_capsule("UpperArm_L", 0.085, 0.47, Vector3(-0.34, 1.39, 0.0), -1.12, shirt);
    add_capsule("UpperArm_R", 0.085, 0.47, Vector3(0.34, 1.39, 0.0), 1.12, shirt);
    add_capsule("Forearm_L", 0.070, 0.43, Vector3(-0.60, 1.17, 0.0), -1.23, skin);
    add_capsule("Forearm_R", 0.070, 0.43, Vector3(0.60, 1.17, 0.0), 1.23, skin);
    add_capsule("Thigh_L", 0.115, 0.49, Vector3(-0.13, 0.62, 0.0), 0.04, pants);
    add_capsule("Thigh_R", 0.115, 0.49, Vector3(0.13, 0.62, 0.0), -0.04, pants);
    add_capsule("Shin_L", 0.092, 0.49, Vector3(-0.13, 0.25, 0.0), 0.02, pants);
    add_capsule("Shin_R", 0.092, 0.49, Vector3(0.13, 0.25, 0.0), -0.02, pants);
    add_box("Shoe_L", Vector3(0.19, 0.10, 0.29), Vector3(-0.13, 0.06, -0.035), shoes);
    add_box("Shoe_R", Vector3(0.19, 0.10, 0.29), Vector3(0.13, 0.06, -0.035), shoes);
}

void ArutCharacter3D::_create_default_camera() {
    camera_pivot = memnew(Node3D);
    camera_pivot->set_name("ArutCameraPivot");
    camera_pivot->set_position(Vector3(0.0, 1.36, 0.0));
    add_child(camera_pivot);

    default_camera = memnew(Camera3D);
    default_camera->set_name("ArutDefaultCamera");
    default_camera->set_position(Vector3(0.0, 0.12, camera_distance));
    default_camera->set_rotation(Vector3(-0.12, Math_PI, 0.0));
    default_camera->set_current(true);
    camera_pivot->add_child(default_camera);
}

void ArutCharacter3D::_create_touch_hint_overlay() {
    Viewport *viewport = get_viewport();
    if (viewport == nullptr) {
        return;
    }
    const Vector2 screen_size = viewport->get_visible_rect().size;
    CanvasLayer *layer = memnew(CanvasLayer);
    layer->set_name("ArutTouchHints");
    layer->set_layer(10);
    add_child(layer);

    ColorRect *backdrop = memnew(ColorRect);
    backdrop->set_name("TouchHintsBackdrop");
    backdrop->set_position(Vector2(12.0, MAX(12.0, screen_size.y - 106.0)));
    backdrop->set_size(Vector2(MAX(200.0, screen_size.x - 24.0), 94.0));
    backdrop->set_color(Color(0.015, 0.025, 0.04, 0.55));
    backdrop->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    layer->add_child(backdrop);

    Label *hint = memnew(Label);
    hint->set_name("TouchHints");
    hint->set_position(Vector2(8.0, 6.0));
    hint->set_size(Vector2(MAX(180.0, screen_size.x - 40.0), 82.0));
    hint->set_text("LEFT DRAG: MOVE    RIGHT DRAG: LOOK\nTOP-RIGHT: RAGDOLL    BOTTOM-RIGHT: JUMP\nHOLD MID-RIGHT: SPRINT    |    KEYBOARD: WASD + R");
    hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
    hint->add_theme_font_size_override("font_size", 12);
    hint->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    backdrop->add_child(hint);
}

void ArutCharacter3D::_start_ragdoll() {
    if (ragdoll_started || !enable_ragdoll || ragdoll_dummy == nullptr || get_tree() == nullptr) {
        return;
    }

    const Vector3 inherited_velocity = get_velocity();
    const Transform3D world_transform = get_global_transform();
    Node *scene = get_tree()->get_current_scene();

    ragdoll_dummy->set_global_transform(world_transform);
    if (scene != nullptr && ragdoll_dummy->get_parent() != scene) {
        ragdoll_dummy->reparent(scene, true);
    }
    ragdoll_dummy->set_initial_velocity(inherited_velocity);
    ragdoll_dummy->set_ragdoll_active(true);

    if (default_visual_root != nullptr) {
        default_visual_root->set_visible(false);
    }
    if (default_collision_shape != nullptr) {
        default_collision_shape->set_disabled(true);
    }
    for (int32_t i = 0; i < get_child_count(); ++i) {
        Node *child = get_child(i);
        if (child == ragdoll_dummy) {
            continue;
        }
        CollisionShape3D *collision = Object::cast_to<CollisionShape3D>(child);
        if (collision != nullptr) {
            collision->set_disabled(true);
        }
    }

    set_collision_layer(0);
    set_collision_mask(0);
    set_physics_process(false);
    ragdoll_started = true;
}

void ArutCharacter3D::input(const Ref<InputEvent> &p_event) {
    if (ragdoll_started) {
        return;
    }

    InputEventKey *key_event = Object::cast_to<InputEventKey>(p_event.ptr());
    if (key_event != nullptr && key_event->is_pressed() && !key_event->is_echo() && key_event->get_keycode() == Key::KEY_R) {
        _start_ragdoll();
        return;
    }

    if (!enable_mobile_touch_controls || get_viewport() == nullptr) {
        return;
    }

    const Vector2 screen_size = get_viewport()->get_visible_rect().size;
    if (screen_size.x <= 0.0 || screen_size.y <= 0.0) {
        return;
    }

    InputEventScreenTouch *touch = Object::cast_to<InputEventScreenTouch>(p_event.ptr());
    if (touch != nullptr) {
        const Vector2 position = touch->get_position();
        if (touch->is_pressed()) {
            if (enable_ragdoll && position.x > screen_size.x * 0.82 && position.y < screen_size.y * 0.18) {
                _start_ragdoll();
                return;
            }
            if (position.x < screen_size.x * 0.45 && position.y > screen_size.y * 0.28 && joystick_touch_index < 0) {
                joystick_touch_index = touch->get_index();
                joystick_origin = position;
                move_input = Vector2();
                return;
            }
            if (position.x > screen_size.x * 0.76 && position.y > screen_size.y * 0.80) {
                jump_requested = true;
                return;
            }
            if (position.x > screen_size.x * 0.76 && position.y > screen_size.y * 0.60) {
                sprint_touch_index = touch->get_index();
                sprint_active = true;
                return;
            }
            if (position.x >= screen_size.x * 0.45 && look_touch_index < 0) {
                look_touch_index = touch->get_index();
                previous_look_position = position;
                return;
            }
        } else {
            if (touch->get_index() == joystick_touch_index) {
                joystick_touch_index = -1;
                move_input = Vector2();
            }
            if (touch->get_index() == look_touch_index) {
                look_touch_index = -1;
            }
            if (touch->get_index() == sprint_touch_index) {
                sprint_touch_index = -1;
                sprint_active = false;
            }
        }
        return;
    }

    InputEventScreenDrag *drag = Object::cast_to<InputEventScreenDrag>(p_event.ptr());
    if (drag == nullptr) {
        return;
    }

    if (drag->get_index() == joystick_touch_index) {
        const real_t radius = MAX(static_cast<real_t>(60.0), screen_size.x * static_cast<real_t>(0.12));
        const Vector2 delta = drag->get_position() - joystick_origin;
        move_input = Vector2(CLAMP(delta.x / radius, -1.0, 1.0), CLAMP(-delta.y / radius, -1.0, 1.0));
        if (move_input.length() > 1.0) {
            move_input.normalize();
        }
    } else if (drag->get_index() == look_touch_index) {
        const Vector2 delta = drag->get_relative();
        rotate_y(-delta.x * touch_look_sensitivity);
        if (camera_pivot != nullptr) {
            Vector3 camera_rotation = camera_pivot->get_rotation();
            camera_rotation.x = CLAMP(camera_rotation.x - delta.y * touch_look_sensitivity, -0.95, 0.65);
            camera_pivot->set_rotation(camera_rotation);
        }
        previous_look_position = drag->get_position();
    }
}

void ArutCharacter3D::_physics_process(double p_delta) {
    if (ragdoll_started) {
        return;
    }

    const real_t delta = static_cast<real_t>(p_delta);
    if (use_input_actions && joystick_touch_index < 0) {
        Input *input = Input::get_singleton();
        InputMap *input_map = InputMap::get_singleton();
        if (input != nullptr && input_map != nullptr) {
            const real_t left = input_map->has_action(move_left_action) ? input->get_action_strength(move_left_action) : 0.0;
            const real_t right = input_map->has_action(move_right_action) ? input->get_action_strength(move_right_action) : 0.0;
            const real_t forward = input_map->has_action(move_forward_action) ? input->get_action_strength(move_forward_action) : 0.0;
            const real_t backward = input_map->has_action(move_backward_action) ? input->get_action_strength(move_backward_action) : 0.0;

            Vector2 action_input(right - left, forward - backward);
            if (action_input.length() > 1.0) {
                action_input.normalize();
            }
            if (action_input.length_squared() > 0.0 || look_touch_index < 0) {
                move_input = action_input;
            }
            if (input_map->has_action(jump_action) && input->is_action_just_pressed(jump_action)) {
                jump_requested = true;
            }
            if (sprint_touch_index < 0) {
                sprint_active = input_map->has_action(sprint_action) && input->is_action_pressed(sprint_action);
            }
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
    } else {
        if (desired_direction.length_squared() > 0.0001) {
            horizontal_velocity = horizontal_velocity.move_toward(target_horizontal_velocity, air_acceleration * air_control * delta);
        }
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
