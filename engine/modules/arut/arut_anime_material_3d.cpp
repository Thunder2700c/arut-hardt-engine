#include "arut_anime_material_3d.h"

#include "core/object/class_db.h"
#include "core/variant/variant.h"
#include "scene/resources/shader.h"

// This intentionally uses Godot's existing spatial shader pipeline. It adds a
// first-party stylized material resource; it does not replace the renderer.
static const char *ARUT_ANIME_MATERIAL_SHADER = R"ARUT_SHADER(
shader_type spatial;
render_mode diffuse_toon, specular_schlick_ggx, cull_back;

uniform sampler2D albedo_texture : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D normal_texture : hint_normal, filter_linear_mipmap, repeat_enable;
uniform sampler2D ao_texture : hint_default_white, filter_linear_mipmap, repeat_enable;

uniform bool use_albedo_texture = false;
uniform bool use_normal_map = false;
uniform bool use_ao_map = false;
uniform vec4 tint : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform float texture_scale : hint_range(0.01, 64.0, 0.01) = 1.0;
uniform float surface_roughness : hint_range(0.0, 1.0, 0.01) = 0.85;
uniform float specular_strength : hint_range(0.0, 1.0, 0.01) = 0.2;
uniform float rim_strength : hint_range(0.0, 0.3, 0.01) = 0.03;
uniform float rim_tint : hint_range(0.0, 1.0, 0.05) = 0.8;
uniform float normal_depth : hint_range(0.0, 1.0, 0.05) = 1.0;

void fragment() {
    vec2 scaled_uv = UV * texture_scale;
    vec3 base_color = vec3(1.0);

    if (use_albedo_texture) {
        base_color = texture(albedo_texture, scaled_uv).rgb;
    }
    ALBEDO = base_color * tint.rgb;

    AO = 1.0;
    if (use_ao_map) {
        AO = texture(ao_texture, scaled_uv).r;
    }

    if (use_normal_map) {
        NORMAL_MAP = texture(normal_texture, scaled_uv).rgb;
        NORMAL_MAP_DEPTH = normal_depth;
    }

    ROUGHNESS = surface_roughness;
    SPECULAR = specular_strength;
    RIM = rim_strength;
    RIM_TINT = rim_tint;
}
)ARUT_SHADER";

ArutAnimeMaterial3D::ArutAnimeMaterial3D() {
    Ref<Shader> built_in_shader;
    built_in_shader.instantiate();
    built_in_shader->set_code(String::utf8(ARUT_ANIME_MATERIAL_SHADER));
    set_shader(built_in_shader);

    set_shader_parameter("use_albedo_texture", false);
    set_shader_parameter("use_normal_map", false);
    set_shader_parameter("use_ao_map", false);
    set_shader_parameter("tint", tint);
    set_shader_parameter("texture_scale", texture_scale);
    set_shader_parameter("surface_roughness", surface_roughness);
    set_shader_parameter("specular_strength", specular_strength);
    set_shader_parameter("rim_strength", rim_strength);
    set_shader_parameter("rim_tint", rim_tint);
    set_shader_parameter("normal_depth", normal_depth);
}

void ArutAnimeMaterial3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_albedo_texture", "texture"), &ArutAnimeMaterial3D::set_albedo_texture);
    ClassDB::bind_method(D_METHOD("get_albedo_texture"), &ArutAnimeMaterial3D::get_albedo_texture);
    ClassDB::bind_method(D_METHOD("set_normal_texture", "texture"), &ArutAnimeMaterial3D::set_normal_texture);
    ClassDB::bind_method(D_METHOD("get_normal_texture"), &ArutAnimeMaterial3D::get_normal_texture);
    ClassDB::bind_method(D_METHOD("set_ao_texture", "texture"), &ArutAnimeMaterial3D::set_ao_texture);
    ClassDB::bind_method(D_METHOD("get_ao_texture"), &ArutAnimeMaterial3D::get_ao_texture);
    ClassDB::bind_method(D_METHOD("set_tint", "tint"), &ArutAnimeMaterial3D::set_tint);
    ClassDB::bind_method(D_METHOD("get_tint"), &ArutAnimeMaterial3D::get_tint);
    ClassDB::bind_method(D_METHOD("set_texture_scale", "scale"), &ArutAnimeMaterial3D::set_texture_scale);
    ClassDB::bind_method(D_METHOD("get_texture_scale"), &ArutAnimeMaterial3D::get_texture_scale);
    ClassDB::bind_method(D_METHOD("set_surface_roughness", "roughness"), &ArutAnimeMaterial3D::set_surface_roughness);
    ClassDB::bind_method(D_METHOD("get_surface_roughness"), &ArutAnimeMaterial3D::get_surface_roughness);
    ClassDB::bind_method(D_METHOD("set_specular_strength", "strength"), &ArutAnimeMaterial3D::set_specular_strength);
    ClassDB::bind_method(D_METHOD("get_specular_strength"), &ArutAnimeMaterial3D::get_specular_strength);
    ClassDB::bind_method(D_METHOD("set_rim_strength", "strength"), &ArutAnimeMaterial3D::set_rim_strength);
    ClassDB::bind_method(D_METHOD("get_rim_strength"), &ArutAnimeMaterial3D::get_rim_strength);
    ClassDB::bind_method(D_METHOD("set_rim_tint", "tint"), &ArutAnimeMaterial3D::set_rim_tint);
    ClassDB::bind_method(D_METHOD("get_rim_tint"), &ArutAnimeMaterial3D::get_rim_tint);
    ClassDB::bind_method(D_METHOD("set_normal_depth", "depth"), &ArutAnimeMaterial3D::set_normal_depth);
    ClassDB::bind_method(D_METHOD("get_normal_depth"), &ArutAnimeMaterial3D::get_normal_depth);

    ADD_GROUP("Anime Surface", "surface_");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "surface_albedo_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_albedo_texture", "get_albedo_texture");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "surface_normal_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_normal_texture", "get_normal_texture");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "surface_ao_texture", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "set_ao_texture", "get_ao_texture");
    ADD_PROPERTY(PropertyInfo(Variant::COLOR, "surface_tint"), "set_tint", "get_tint");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_texture_scale", PROPERTY_HINT_RANGE, "0.01,64,0.01"), "set_texture_scale", "get_texture_scale");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_roughness", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_surface_roughness", "get_surface_roughness");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_specular_strength", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_specular_strength", "get_specular_strength");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_rim_strength", PROPERTY_HINT_RANGE, "0,0.3,0.01"), "set_rim_strength", "get_rim_strength");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_rim_tint", PROPERTY_HINT_RANGE, "0,1,0.05"), "set_rim_tint", "get_rim_tint");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "surface_normal_depth", PROPERTY_HINT_RANGE, "0,1,0.05"), "set_normal_depth", "get_normal_depth");
}

void ArutAnimeMaterial3D::set_albedo_texture(const Ref<Texture2D> &p_texture) {
    albedo_texture = p_texture;
    set_shader_parameter("albedo_texture", albedo_texture);
    set_shader_parameter("use_albedo_texture", albedo_texture.is_valid());
}

void ArutAnimeMaterial3D::set_normal_texture(const Ref<Texture2D> &p_texture) {
    normal_texture = p_texture;
    set_shader_parameter("normal_texture", normal_texture);
    set_shader_parameter("use_normal_map", normal_texture.is_valid());
}

void ArutAnimeMaterial3D::set_ao_texture(const Ref<Texture2D> &p_texture) {
    ao_texture = p_texture;
    set_shader_parameter("ao_texture", ao_texture);
    set_shader_parameter("use_ao_map", ao_texture.is_valid());
}

void ArutAnimeMaterial3D::set_tint(const Color &p_tint) {
    tint = p_tint;
    set_shader_parameter("tint", tint);
}

void ArutAnimeMaterial3D::set_texture_scale(real_t p_scale) {
    texture_scale = std::clamp(p_scale, static_cast<real_t>(0.01), static_cast<real_t>(64.0));
    set_shader_parameter("texture_scale", texture_scale);
}

void ArutAnimeMaterial3D::set_surface_roughness(real_t p_roughness) {
    surface_roughness = std::clamp(p_roughness, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    set_shader_parameter("surface_roughness", surface_roughness);
}

void ArutAnimeMaterial3D::set_specular_strength(real_t p_strength) {
    specular_strength = std::clamp(p_strength, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    set_shader_parameter("specular_strength", specular_strength);
}

void ArutAnimeMaterial3D::set_rim_strength(real_t p_strength) {
    rim_strength = std::clamp(p_strength, static_cast<real_t>(0.0), static_cast<real_t>(0.3));
    set_shader_parameter("rim_strength", rim_strength);
}

void ArutAnimeMaterial3D::set_rim_tint(real_t p_tint) {
    rim_tint = std::clamp(p_tint, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    set_shader_parameter("rim_tint", rim_tint);
}

void ArutAnimeMaterial3D::set_normal_depth(real_t p_depth) {
    normal_depth = std::clamp(p_depth, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    set_shader_parameter("normal_depth", normal_depth);
}
