#include "arut_ragdoll_dummy_3d.h"

#include "core/math/color.h"
#include "core/math/math_defs.h"
#include "core/object/class_db.h"
#include "core/variant/variant.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/joints/cone_twist_joint_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/3d/standard_material_3d.h"

void ArutRagdollDummy3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_start_simulating", "enabled"), &ArutRagdollDummy3D::set_start_simulating);
    ClassDB::bind_method(D_METHOD("get_start_simulating"), &ArutRagdollDummy3D::get_start_simulating);
    ClassDB::bind_method(D_METHOD("set_mass_scale", "scale"), &ArutRagdollDummy3D::set_mass_scale);
    ClassDB::bind_method(D_METHOD("get_mass_scale"), &ArutRagdollDummy3D::get_mass_scale);
    ClassDB::bind_method(D_METHOD("set_angular_damping", "damping"), &ArutRagdollDummy3D::set_angular_damping);
    ClassDB::bind_method(D_METHOD("get_angular_damping"), &ArutRagdollDummy3D::get_angular_damping);
    ClassDB::bind_method(D_METHOD("set_ragdoll_active", "active"), &ArutRagdollDummy3D::set_ragdoll_active);
    ClassDB::bind_method(D_METHOD("is_ragdoll_active"), &ArutRagdollDummy3D::is_ragdoll_active);
    ClassDB::bind_method(D_METHOD("apply_impulse_to_body", "body_name", "impulse"), &ArutRagdollDummy3D::apply_impulse_to_body);
    ClassDB::bind_method(D_METHOD("set_initial_velocity", "velocity"), &ArutRagdollDummy3D::set_initial_velocity);

    ADD_GROUP("Ragdoll", "ragdoll_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "ragdoll_start_simulating"), "set_start_simulating", "get_start_simulating");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ragdoll_mass_scale", PROPERTY_HINT_RANGE, "0.1,5.0,0.1"), "set_mass_scale", "get_mass_scale");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "ragdoll_angular_damping", PROPERTY_HINT_RANGE, "0.0,10.0,0.1"), "set_angular_damping", "get_angular_damping");
}

void ArutRagdollDummy3D::set_start_simulating(bool p_value) {
    start_simulating = p_value;
    if (generated) {
        _apply_simulation_state();
    }
}

bool ArutRagdollDummy3D::get_start_simulating() const {
    return start_simulating;
}

void ArutRagdollDummy3D::set_mass_scale(real_t p_value) {
    mass_scale = CLAMP(p_value, static_cast<real_t>(0.1), static_cast<real_t>(5.0));
}

real_t ArutRagdollDummy3D::get_mass_scale() const {
    return mass_scale;
}

void ArutRagdollDummy3D::set_angular_damping(real_t p_value) {
    angular_damping = CLAMP(p_value, static_cast<real_t>(0.0), static_cast<real_t>(10.0));
    for (RigidBody3D *body : ragdoll_bodies) {
        if (body != nullptr) {
            body->set_angular_damp(angular_damping);
        }
    }
}

real_t ArutRagdollDummy3D::get_angular_damping() const {
    return angular_damping;
}

void ArutRagdollDummy3D::_ready() {
    if (!generated) {
        _build_ragdoll();
    }
    _apply_simulation_state();
}

RigidBody3D *ArutRagdollDummy3D::_create_capsule_part(const StringName &p_name, const Vector3 &p_position, real_t p_radius, real_t p_height, real_t p_mass, const Color &p_color, real_t p_rotation_z) {
    RigidBody3D *body = memnew(RigidBody3D);
    body->set_name(p_name);
    body->set_position(p_position);
    body->set_mass(p_mass * mass_scale);
    body->set_linear_damp(0.15);
    body->set_angular_damp(angular_damping);
    body->set_collision_layer(2);
    body->set_collision_mask(1 | 2);
    body->set_can_sleep(true);
    add_child(body);

    Ref<CapsuleShape3D> shape;
    shape.instantiate();
    shape->set_radius(p_radius);
    shape->set_height(MAX(p_height, p_radius * 2.0));

    CollisionShape3D *collision = memnew(CollisionShape3D);
    collision->set_name("CollisionShape3D");
    collision->set_shape(shape);
    collision->set_rotation(Vector3(0.0, 0.0, p_rotation_z));
    body->add_child(collision);

    Ref<CapsuleMesh> mesh;
    mesh.instantiate();
    mesh->set_radius(p_radius);
    mesh->set_height(MAX(p_height, p_radius * 2.0));

    Ref<StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(p_color);
    material->set_roughness(0.72);

    MeshInstance3D *visual = memnew(MeshInstance3D);
    visual->set_name("Visual");
    visual->set_mesh(mesh);
    visual->set_material_override(material);
    visual->set_rotation(Vector3(0.0, 0.0, p_rotation_z));
    body->add_child(visual);

    ragdoll_bodies.push_back(body);
    return body;
}

RigidBody3D *ArutRagdollDummy3D::_create_sphere_part(const StringName &p_name, const Vector3 &p_position, real_t p_radius, real_t p_mass, const Color &p_color) {
    RigidBody3D *body = memnew(RigidBody3D);
    body->set_name(p_name);
    body->set_position(p_position);
    body->set_mass(p_mass * mass_scale);
    body->set_linear_damp(0.15);
    body->set_angular_damp(angular_damping);
    body->set_collision_layer(2);
    body->set_collision_mask(1 | 2);
    body->set_can_sleep(true);
    add_child(body);

    Ref<SphereShape3D> shape;
    shape.instantiate();
    shape->set_radius(p_radius);

    CollisionShape3D *collision = memnew(CollisionShape3D);
    collision->set_name("CollisionShape3D");
    collision->set_shape(shape);
    body->add_child(collision);

    Ref<SphereMesh> mesh;
    mesh.instantiate();
    mesh->set_radius(p_radius);

    Ref<StandardMaterial3D> material;
    material.instantiate();
    material->set_albedo(p_color);
    material->set_roughness(0.72);

    MeshInstance3D *visual = memnew(MeshInstance3D);
    visual->set_name("Visual");
    visual->set_mesh(mesh);
    visual->set_material_override(material);
    body->add_child(visual);

    ragdoll_bodies.push_back(body);
    return body;
}

void ArutRagdollDummy3D::_create_joint(const StringName &p_name, const StringName &p_body_a, const StringName &p_body_b, const Vector3 &p_anchor, real_t p_swing_span, real_t p_twist_span) {
    ConeTwistJoint3D *joint = memnew(ConeTwistJoint3D);
    joint->set_name(p_name);
    joint->set_position(p_anchor);
    joint->set_param(ConeTwistJoint3D::PARAM_SWING_SPAN, p_swing_span);
    joint->set_param(ConeTwistJoint3D::PARAM_TWIST_SPAN, p_twist_span);
    joint->set_param(ConeTwistJoint3D::PARAM_BIAS, 0.3);
    joint->set_param(ConeTwistJoint3D::PARAM_SOFTNESS, 0.8);
    joint->set_param(ConeTwistJoint3D::PARAM_RELAXATION, 1.0);
    joint->set_exclude_nodes_from_collision(true);
    add_child(joint);
    joint->set_node_a(NodePath(String("../") + String(p_body_a)));
    joint->set_node_b(NodePath(String("../") + String(p_body_b)));
}

void ArutRagdollDummy3D::_build_ragdoll() {
    // Simple stylized body colours keep the dummy readable without external assets.
    const Color skin(0.82, 0.58, 0.42);
    const Color shirt(0.08, 0.72, 0.63);
    const Color trousers(0.10, 0.16, 0.27);
    const Color shoes(0.04, 0.05, 0.07);

    _create_capsule_part("Pelvis", Vector3(0.0, 0.96, 0.0), 0.18, 0.34, 8.0, trousers);
    _create_capsule_part("Torso", Vector3(0.0, 1.38, 0.0), 0.23, 0.62, 15.0, shirt);
    _create_sphere_part("Head", Vector3(0.0, 1.91, 0.0), 0.17, 4.0, skin);
    _create_capsule_part("UpperArm_L", Vector3(-0.34, 1.40, 0.0), 0.085, 0.48, 2.4, shirt, -Math_PI * 0.5);
    _create_capsule_part("UpperArm_R", Vector3(0.34, 1.40, 0.0), 0.085, 0.48, 2.4, shirt, Math_PI * 0.5);
    _create_capsule_part("Forearm_L", Vector3(-0.61, 1.20, 0.0), 0.072, 0.43, 1.6, skin, -Math_PI * 0.5);
    _create_capsule_part("Forearm_R", Vector3(0.61, 1.20, 0.0), 0.072, 0.43, 1.6, skin, Math_PI * 0.5);
    _create_capsule_part("Thigh_L", Vector3(-0.13, 0.68, 0.0), 0.12, 0.56, 5.5, trousers);
    _create_capsule_part("Thigh_R", Vector3(0.13, 0.68, 0.0), 0.12, 0.56, 5.5, trousers);
    _create_capsule_part("Shin_L", Vector3(-0.13, 0.27, 0.0), 0.095, 0.48, 3.6, shoes);
    _create_capsule_part("Shin_R", Vector3(0.13, 0.27, 0.0), 0.095, 0.48, 3.6, shoes);

    _create_joint("Joint_Pelvis_Torso", "Pelvis", "Torso", Vector3(0.0, 1.15, 0.0), 0.45, 0.35);
    _create_joint("Joint_Neck", "Torso", "Head", Vector3(0.0, 1.73, 0.0), 0.30, 0.25);
    _create_joint("Joint_Shoulder_L", "Torso", "UpperArm_L", Vector3(-0.25, 1.55, 0.0), 0.85, 0.65);
    _create_joint("Joint_Shoulder_R", "Torso", "UpperArm_R", Vector3(0.25, 1.55, 0.0), 0.85, 0.65);
    _create_joint("Joint_Elbow_L", "UpperArm_L", "Forearm_L", Vector3(-0.49, 1.29, 0.0), 0.45, 0.20);
    _create_joint("Joint_Elbow_R", "UpperArm_R", "Forearm_R", Vector3(0.49, 1.29, 0.0), 0.45, 0.20);
    _create_joint("Joint_Hip_L", "Pelvis", "Thigh_L", Vector3(-0.13, 0.81, 0.0), 0.65, 0.45);
    _create_joint("Joint_Hip_R", "Pelvis", "Thigh_R", Vector3(0.13, 0.81, 0.0), 0.65, 0.45);
    _create_joint("Joint_Knee_L", "Thigh_L", "Shin_L", Vector3(-0.13, 0.43, 0.0), 0.28, 0.16);
    _create_joint("Joint_Knee_R", "Thigh_R", "Shin_R", Vector3(0.13, 0.43, 0.0), 0.28, 0.16);

    generated = true;
}

void ArutRagdollDummy3D::_apply_simulation_state() {
    set_visible(start_simulating);
    for (RigidBody3D *body : ragdoll_bodies) {
        if (body == nullptr) {
            continue;
        }
        body->set_freeze_enabled(!start_simulating);
        if (start_simulating) {
            body->set_sleeping(false);
        }
    }
}

void ArutRagdollDummy3D::set_ragdoll_active(bool p_active) {
    start_simulating = p_active;
    if (generated) {
        _apply_simulation_state();
    }
}

bool ArutRagdollDummy3D::is_ragdoll_active() const {
    return start_simulating;
}

void ArutRagdollDummy3D::apply_impulse_to_body(const StringName &p_body_name, const Vector3 &p_impulse) {
    Node *node = get_node_or_null(NodePath(String(".") + String(p_body_name)));
    RigidBody3D *body = Object::cast_to<RigidBody3D>(node);
    if (body != nullptr && start_simulating) {
        body->apply_central_impulse(p_impulse);
    }
}

void ArutRagdollDummy3D::set_initial_velocity(const Vector3 &p_velocity) {
    for (RigidBody3D *body : ragdoll_bodies) {
        if (body != nullptr) {
            body->set_linear_velocity(p_velocity);
        }
    }
}
