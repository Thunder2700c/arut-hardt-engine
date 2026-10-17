#pragma once

#include "core/math/color.h"
#include "scene/resources/material.h"
#include "scene/resources/texture.h"

// ARUT's reusable, mobile-aware anime/PBR hybrid material. The shader source
// lives in native C++ so the material has no script-file dependency.
class ArutAnimeLitMaterial3D : public ShaderMaterial {
    GDCLASS(ArutAnimeLitMaterial3D, ShaderMaterial);

private:
    static Ref<Shader> shared_shader;

    Color base_color = Color(0.92, 0.93, 0.98, 1.0);
    Color shadow_color = Color(0.30, 0.34, 0.44, 1.0);
    Color lit_tint = Color(1.0, 1.0, 1.0, 1.0);
    real_t shadow_threshold = 0.48;
    real_t shadow_softness = 0.08;
    real_t roughness = 0.72;
    real_t specular_strength = 0.12;
    real_t specular_power = 32.0;
    Color rim_color = Color(0.32, 0.62, 1.0, 1.0);
    real_t rim_strength = 0.12;
    real_t rim_power = 3.0;
    real_t normal_map_strength = 1.0;
    Ref<Texture2D> albedo_texture;
    Ref<Texture2D> normal_texture;

    void _apply_all_parameters();
    void _apply_color_parameter(const StringName &p_name, const Color &p_color);
    void _apply_texture_parameters();

protected:
    static void _bind_methods();

public:
    static void initialize_shared_shader();
    static void clear_shared_shader();

    ArutAnimeLitMaterial3D();

    void set_base_color(const Color &p_value);
    Color get_base_color() const { return base_color; }
    void set_shadow_color(const Color &p_value);
    Color get_shadow_color() const { return shadow_color; }
    void set_lit_tint(const Color &p_value);
    Color get_lit_tint() const { return lit_tint; }

    void set_shadow_threshold(real_t p_value);
    real_t get_shadow_threshold() const { return shadow_threshold; }
    void set_shadow_softness(real_t p_value);
    real_t get_shadow_softness() const { return shadow_softness; }
    void set_roughness(real_t p_value);
    real_t get_roughness() const { return roughness; }
    void set_specular_strength(real_t p_value);
    real_t get_specular_strength() const { return specular_strength; }
    void set_specular_power(real_t p_value);
    real_t get_specular_power() const { return specular_power; }

    void set_rim_color(const Color &p_value);
    Color get_rim_color() const { return rim_color; }
    void set_rim_strength(real_t p_value);
    real_t get_rim_strength() const { return rim_strength; }
    void set_rim_power(real_t p_value);
    real_t get_rim_power() const { return rim_power; }

    void set_normal_map_strength(real_t p_value);
    real_t get_normal_map_strength() const { return normal_map_strength; }

    void set_albedo_texture(const Ref<Texture2D> &p_value);
    Ref<Texture2D> get_albedo_texture() const { return albedo_texture; }
    void set_normal_texture(const Ref<Texture2D> &p_value);
    Ref<Texture2D> get_normal_texture() const { return normal_texture; }
};
