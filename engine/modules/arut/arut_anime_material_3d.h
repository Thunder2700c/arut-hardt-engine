#pragma once

#include <algorithm>

#include "core/math/color.h"
#include "core/math/math_defs.h"
#include "scene/resources/material.h"
#include "scene/resources/texture.h"

// First-party anime/PBR hybrid material. The shader is embedded so projects
// can create this Resource without copying a shader file or using GDScript.
class ArutAnimeMaterial3D : public ShaderMaterial {
    GDCLASS(ArutAnimeMaterial3D, ShaderMaterial);

private:
    Ref<Texture2D> albedo_texture;
    Ref<Texture2D> normal_texture;
    Ref<Texture2D> ao_texture;
    Color tint = Color(1.0, 1.0, 1.0, 1.0);
    real_t texture_scale = 1.0;
    real_t surface_roughness = 0.85;
    real_t specular_strength = 0.2;
    real_t rim_strength = 0.03;
    real_t rim_tint = 0.8;
    real_t normal_depth = 1.0;

protected:
    static void _bind_methods();

public:
    ArutAnimeMaterial3D();

    void set_albedo_texture(const Ref<Texture2D> &p_texture);
    Ref<Texture2D> get_albedo_texture() const { return albedo_texture; }
    void set_normal_texture(const Ref<Texture2D> &p_texture);
    Ref<Texture2D> get_normal_texture() const { return normal_texture; }
    void set_ao_texture(const Ref<Texture2D> &p_texture);
    Ref<Texture2D> get_ao_texture() const { return ao_texture; }

    void set_tint(const Color &p_tint);
    Color get_tint() const { return tint; }
    void set_texture_scale(real_t p_scale);
    real_t get_texture_scale() const { return texture_scale; }
    void set_surface_roughness(real_t p_roughness);
    real_t get_surface_roughness() const { return surface_roughness; }
    void set_specular_strength(real_t p_strength);
    real_t get_specular_strength() const { return specular_strength; }
    void set_rim_strength(real_t p_strength);
    real_t get_rim_strength() const { return rim_strength; }
    void set_rim_tint(real_t p_tint);
    real_t get_rim_tint() const { return rim_tint; }
    void set_normal_depth(real_t p_depth);
    real_t get_normal_depth() const { return normal_depth; }
};
