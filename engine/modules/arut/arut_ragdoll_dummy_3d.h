#pragma once

#include <vector>

#include "core/math/color.h"
#include "core/math/vector3.h"
#include "scene/3d/node_3d.h"
#include "scene/3d/physics/rigid_body_3d.h"

// Procedural articulated dummy made from connected native rigid bodies.
// Intended as an out-of-the-box ragdoll fallback until a skinned mesh imports
// a custom skeleton/physics rig.
class ArutRagdollDummy3D : public Node3D {
    GDCLASS(ArutRagdollDummy3D, Node3D);

private:
    bool start_simulating = true;
    real_t mass_scale = 1.0;
    real_t angular_damping = 1.6;
    bool generated = false;
    std::vector<RigidBody3D *> ragdoll_bodies;

    RigidBody3D *_create_capsule_part(const StringName &p_name, const Vector3 &p_position, real_t p_radius, real_t p_height, real_t p_mass, const Color &p_color, real_t p_rotation_z = 0.0);
    RigidBody3D *_create_sphere_part(const StringName &p_name, const Vector3 &p_position, real_t p_radius, real_t p_mass, const Color &p_color);
    void _create_joint(const StringName &p_name, const StringName &p_body_a, const StringName &p_body_b, const Vector3 &p_anchor, real_t p_swing_span, real_t p_twist_span);
    void _build_ragdoll();
    void _apply_simulation_state();

protected:
    static void _bind_methods();

public:
    void _ready() override;

    void set_start_simulating(bool p_value);
    bool get_start_simulating() const;
    void set_mass_scale(real_t p_value);
    real_t get_mass_scale() const;
    void set_angular_damping(real_t p_value);
    real_t get_angular_damping() const;

    void set_ragdoll_active(bool p_active);
    bool is_ragdoll_active() const;
    void apply_impulse_to_body(const StringName &p_body_name, const Vector3 &p_impulse);
    void set_initial_velocity(const Vector3 &p_velocity);
};
