#include "arut_render_profile_3d.h"

#include <cstdlib>

#include "core/config/engine.h"
#include "core/object/class_db.h"
#include "core/os/os.h"
#include "core/variant/variant.h"

void ArutRenderProfile3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_preset", "value"), &ArutRenderProfile3D::set_preset);
    ClassDB::bind_method(D_METHOD("get_preset"), &ArutRenderProfile3D::get_preset);
    ClassDB::bind_method(D_METHOD("set_render_scale", "value"), &ArutRenderProfile3D::set_render_scale);
    ClassDB::bind_method(D_METHOD("get_render_scale"), &ArutRenderProfile3D::get_render_scale);
    ClassDB::bind_method(D_METHOD("set_scaling_mode", "value"), &ArutRenderProfile3D::set_scaling_mode);
    ClassDB::bind_method(D_METHOD("get_scaling_mode"), &ArutRenderProfile3D::get_scaling_mode);
    ClassDB::bind_method(D_METHOD("get_effective_scaling_mode"), &ArutRenderProfile3D::get_effective_scaling_mode);
    ClassDB::bind_method(D_METHOD("get_scaling_mode_status"), &ArutRenderProfile3D::get_scaling_mode_status);
    ClassDB::bind_method(D_METHOD("set_msaa_3d", "value"), &ArutRenderProfile3D::set_msaa_3d);
    ClassDB::bind_method(D_METHOD("get_msaa_3d"), &ArutRenderProfile3D::get_msaa_3d);
    ClassDB::bind_method(D_METHOD("set_screen_space_aa", "value"), &ArutRenderProfile3D::set_screen_space_aa);
    ClassDB::bind_method(D_METHOD("get_screen_space_aa"), &ArutRenderProfile3D::get_screen_space_aa);
    ClassDB::bind_method(D_METHOD("set_anisotropic_filtering", "value"), &ArutRenderProfile3D::set_anisotropic_filtering);
    ClassDB::bind_method(D_METHOD("get_anisotropic_filtering"), &ArutRenderProfile3D::get_anisotropic_filtering);
    ClassDB::bind_method(D_METHOD("set_use_taa", "value"), &ArutRenderProfile3D::set_use_taa);
    ClassDB::bind_method(D_METHOD("get_use_taa"), &ArutRenderProfile3D::get_use_taa);
    ClassDB::bind_method(D_METHOD("set_use_occlusion_culling", "value"), &ArutRenderProfile3D::set_use_occlusion_culling);
    ClassDB::bind_method(D_METHOD("get_use_occlusion_culling"), &ArutRenderProfile3D::get_use_occlusion_culling);
    ClassDB::bind_method(D_METHOD("set_mesh_lod_threshold", "value"), &ArutRenderProfile3D::set_mesh_lod_threshold);
    ClassDB::bind_method(D_METHOD("get_mesh_lod_threshold"), &ArutRenderProfile3D::get_mesh_lod_threshold);
    ClassDB::bind_method(D_METHOD("set_use_debanding", "value"), &ArutRenderProfile3D::set_use_debanding);
    ClassDB::bind_method(D_METHOD("get_use_debanding"), &ArutRenderProfile3D::get_use_debanding);
    ClassDB::bind_method(D_METHOD("set_fsr_sharpness", "value"), &ArutRenderProfile3D::set_fsr_sharpness);
    ClassDB::bind_method(D_METHOD("get_fsr_sharpness"), &ArutRenderProfile3D::get_fsr_sharpness);
    ClassDB::bind_method(D_METHOD("set_texture_mipmap_bias", "value"), &ArutRenderProfile3D::set_texture_mipmap_bias);
    ClassDB::bind_method(D_METHOD("get_texture_mipmap_bias"), &ArutRenderProfile3D::get_texture_mipmap_bias);
    ClassDB::bind_method(D_METHOD("set_positional_shadow_atlas_size", "value"), &ArutRenderProfile3D::set_positional_shadow_atlas_size);
    ClassDB::bind_method(D_METHOD("get_positional_shadow_atlas_size"), &ArutRenderProfile3D::get_positional_shadow_atlas_size);
    ClassDB::bind_method(D_METHOD("set_apply_on_ready", "value"), &ArutRenderProfile3D::set_apply_on_ready);
    ClassDB::bind_method(D_METHOD("get_apply_on_ready"), &ArutRenderProfile3D::get_apply_on_ready);
    ClassDB::bind_method(D_METHOD("set_apply_on_change", "value"), &ArutRenderProfile3D::set_apply_on_change);
    ClassDB::bind_method(D_METHOD("get_apply_on_change"), &ArutRenderProfile3D::get_apply_on_change);
    ClassDB::bind_method(D_METHOD("apply_to_current_viewport"), &ArutRenderProfile3D::apply_to_current_viewport);
    ClassDB::bind_method(D_METHOD("apply_now"), &ArutRenderProfile3D::apply_now);
    ClassDB::bind_method(D_METHOD("apply_to_viewport", "viewport"), &ArutRenderProfile3D::apply_to_viewport);

    ADD_GROUP("Quality Profile", "quality_");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "quality_preset", PROPERTY_HINT_ENUM, "Mobile,Balanced,High,Custom"), "set_preset", "get_preset");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "quality_apply_on_ready"), "set_apply_on_ready", "get_apply_on_ready");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "quality_apply_on_change"), "set_apply_on_change", "get_apply_on_change");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "quality_render_scale", PROPERTY_HINT_RANGE, "0.25,1.5,0.05"), "set_render_scale", "get_render_scale");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "quality_scaling_mode", PROPERTY_HINT_ENUM, "Bilinear,FSR,FSR2,MetalFX Spatial,MetalFX Temporal,Nearest"), "set_scaling_mode", "get_scaling_mode");

    ADD_GROUP("Anti-Aliasing", "aa_");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "aa_msaa_3d", PROPERTY_HINT_ENUM, "Disabled,2x,4x,8x"), "set_msaa_3d", "get_msaa_3d");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "aa_screen_space", PROPERTY_HINT_ENUM, "Disabled,FXAA,SMAA"), "set_screen_space_aa", "get_screen_space_aa");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "aa_temporal"), "set_use_taa", "get_use_taa");

    ADD_GROUP("Optimization", "optimization_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "optimization_occlusion_culling"), "set_use_occlusion_culling", "get_use_occlusion_culling");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "optimization_mesh_lod_threshold", PROPERTY_HINT_RANGE, "0,20,0.1,suffix:px"), "set_mesh_lod_threshold", "get_mesh_lod_threshold");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "optimization_shadow_atlas_size", PROPERTY_HINT_ENUM, "256:256,512:512,1024:1024,2048:2048,4096:4096"), "set_positional_shadow_atlas_size", "get_positional_shadow_atlas_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "optimization_anisotropic_filtering", PROPERTY_HINT_ENUM, "Disabled,2x,4x,8x,16x"), "set_anisotropic_filtering", "get_anisotropic_filtering");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "optimization_texture_mipmap_bias", PROPERTY_HINT_RANGE, "-4,4,0.25"), "set_texture_mipmap_bias", "get_texture_mipmap_bias");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "optimization_debanding"), "set_use_debanding", "get_use_debanding");

    ADD_GROUP("Advanced", "advanced_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "advanced_fsr_sharpness", PROPERTY_HINT_RANGE, "0,1,0.05"), "set_fsr_sharpness", "get_fsr_sharpness");

    ADD_GROUP("Renderer Compatibility", "renderer_");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "renderer_effective_scaling_mode", PROPERTY_HINT_ENUM, "Bilinear,FSR,FSR2,MetalFX Spatial,MetalFX Temporal,Nearest"), "", "get_effective_scaling_mode");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "renderer_scaling_status"), "", "get_scaling_mode_status");

    BIND_ENUM_CONSTANT(MOBILE);
    BIND_ENUM_CONSTANT(BALANCED);
    BIND_ENUM_CONSTANT(HIGH);
    BIND_ENUM_CONSTANT(CUSTOM);
}

void ArutRenderProfile3D::_notification(int p_what) {
    if (p_what == NOTIFICATION_READY) {
        _on_ready();
    }
}

void ArutRenderProfile3D::_on_ready() {
    Engine *engine = Engine::get_singleton();
    if (apply_on_ready && (engine == nullptr || !engine->is_editor_hint())) {
        apply_to_current_viewport();
    }
}

void ArutRenderProfile3D::_apply_if_active() {
    Engine *engine = Engine::get_singleton();
    if (apply_on_change && is_inside_tree() && (engine == nullptr || !engine->is_editor_hint())) {
        apply_to_current_viewport();
    }
}

void ArutRenderProfile3D::set_preset(int32_t p_value) {
    preset = std::clamp(p_value, static_cast<int32_t>(MOBILE), static_cast<int32_t>(CUSTOM));
    switch (preset) {
        case MOBILE:
            render_scale = 0.80;
            // Godot 4.7's Mobile renderer falls back to bilinear for FSR and
            // temporal upscalers. Use bilinear explicitly instead of silently
            // asking for a mode that this pipeline cannot provide.
            scaling_mode = Viewport::SCALING_3D_MODE_BILINEAR;
            msaa_3d = Viewport::MSAA_DISABLED;
            screen_space_aa = Viewport::SCREEN_SPACE_AA_FXAA;
            anisotropic_filtering = Viewport::ANISOTROPY_4X;
            use_taa = false;
            use_occlusion_culling = true;
            mesh_lod_threshold = 1.5;
            use_debanding = true;
            fsr_sharpness = 0.25;
            texture_mipmap_bias = 0.0;
            positional_shadow_atlas_size = 1024;
            break;
        case BALANCED:
            render_scale = 0.90;
            scaling_mode = Viewport::SCALING_3D_MODE_BILINEAR;
            msaa_3d = Viewport::MSAA_2X;
            screen_space_aa = Viewport::SCREEN_SPACE_AA_DISABLED;
            anisotropic_filtering = Viewport::ANISOTROPY_8X;
            use_taa = false;
            use_occlusion_culling = true;
            mesh_lod_threshold = 1.0;
            use_debanding = true;
            fsr_sharpness = 0.20;
            texture_mipmap_bias = -0.25;
            positional_shadow_atlas_size = 2048;
            break;
        case HIGH:
            render_scale = 1.0;
            scaling_mode = Viewport::SCALING_3D_MODE_BILINEAR;
            msaa_3d = Viewport::MSAA_DISABLED;
            screen_space_aa = Viewport::SCREEN_SPACE_AA_DISABLED;
            anisotropic_filtering = Viewport::ANISOTROPY_16X;
            use_taa = true;
            use_occlusion_culling = true;
            mesh_lod_threshold = 0.5;
            use_debanding = true;
            fsr_sharpness = 0.0;
            texture_mipmap_bias = 0.0;
            positional_shadow_atlas_size = 4096;
            break;
        case CUSTOM:
        default:
            // Selecting Custom preserves all currently edited values.
            break;
    }
    _apply_if_active();
}

void ArutRenderProfile3D::set_render_scale(real_t p_value) {
    render_scale = std::clamp(p_value, static_cast<real_t>(0.25), static_cast<real_t>(1.5));
    _apply_if_active();
}

void ArutRenderProfile3D::set_scaling_mode(int32_t p_value) {
    scaling_mode = std::clamp(p_value, 0, static_cast<int32_t>(Viewport::SCALING_3D_MODE_MAX - 1));
    _apply_if_active();
}

int32_t ArutRenderProfile3D::get_effective_scaling_mode() const {
    const OS *os = OS::get_singleton();
    if (os == nullptr) {
        return scaling_mode;
    }

    const Viewport::Scaling3DMode requested = static_cast<Viewport::Scaling3DMode>(scaling_mode);
    const String renderer = os->get_current_rendering_method();
    const String platform = os->get_name().to_lower();
    const bool apple_platform = platform == "macos" || platform == "ios" || platform == "visionos";
    const bool requests_metalfx = requested == Viewport::SCALING_3D_MODE_METALFX_SPATIAL ||
            requested == Viewport::SCALING_3D_MODE_METALFX_TEMPORAL;

    // MetalFX is only available on supported Apple render devices. Avoid
    // routing Android/Linux/Windows projects into a known unsupported mode.
    if (requests_metalfx && !apple_platform) {
        return Viewport::SCALING_3D_MODE_BILINEAR;
    }

    // Mirror Godot 4.7's renderer-level fallbacks so the Inspector can show
    // the effective mode and ARUT can avoid unsupported upscaling requests.
    if (renderer == "mobile" &&
            (requested == Viewport::SCALING_3D_MODE_FSR ||
                    requested == Viewport::SCALING_3D_MODE_FSR2 ||
                    requested == Viewport::SCALING_3D_MODE_METALFX_TEMPORAL)) {
        return Viewport::SCALING_3D_MODE_BILINEAR;
    }

    if (renderer == "gl_compatibility" &&
            requested != Viewport::SCALING_3D_MODE_BILINEAR &&
            requested != Viewport::SCALING_3D_MODE_NEAREST) {
        return Viewport::SCALING_3D_MODE_BILINEAR;
    }

    return scaling_mode;
}

String ArutRenderProfile3D::get_scaling_mode_status() const {
    if (get_effective_scaling_mode() == scaling_mode) {
        return "Requested scaling mode is passed to Godot. A GPU/driver may still apply its own capability fallback.";
    }

    const OS *os = OS::get_singleton();
    if (os == nullptr) {
        return "Using Bilinear as a safe fallback.";
    }

    const Viewport::Scaling3DMode requested = static_cast<Viewport::Scaling3DMode>(scaling_mode);
    const String renderer = os->get_current_rendering_method();
    const String platform = os->get_name().to_lower();

    if ((requested == Viewport::SCALING_3D_MODE_METALFX_SPATIAL ||
                requested == Viewport::SCALING_3D_MODE_METALFX_TEMPORAL) &&
            platform != "macos" && platform != "ios" && platform != "visionos") {
        return "ARUT fallback: MetalFX requires a supported Apple rendering device; using Bilinear.";
    }
    if (renderer == "mobile") {
        return "ARUT fallback: Godot's Mobile renderer does not support FSR/FSR2/MetalFX Temporal upscaling; using Bilinear.";
    }
    if (renderer == "gl_compatibility") {
        return "ARUT fallback: the Compatibility renderer does not support FSR/MetalFX upscaling; using Bilinear.";
    }

    return "ARUT is using Bilinear because the requested scaling mode is not compatible with the active renderer.";
}

void ArutRenderProfile3D::set_msaa_3d(int32_t p_value) {
    msaa_3d = std::clamp(p_value, 0, static_cast<int32_t>(Viewport::MSAA_MAX - 1));
    _apply_if_active();
}

void ArutRenderProfile3D::set_screen_space_aa(int32_t p_value) {
    screen_space_aa = std::clamp(p_value, 0, static_cast<int32_t>(Viewport::SCREEN_SPACE_AA_MAX - 1));
    _apply_if_active();
}

void ArutRenderProfile3D::set_anisotropic_filtering(int32_t p_value) {
    anisotropic_filtering = std::clamp(p_value, 0, static_cast<int32_t>(Viewport::ANISOTROPY_MAX - 1));
    _apply_if_active();
}

void ArutRenderProfile3D::set_use_taa(bool p_value) {
    use_taa = p_value;
    _apply_if_active();
}

void ArutRenderProfile3D::set_use_occlusion_culling(bool p_value) {
    use_occlusion_culling = p_value;
    _apply_if_active();
}

void ArutRenderProfile3D::set_mesh_lod_threshold(real_t p_value) {
    mesh_lod_threshold = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(20.0));
    _apply_if_active();
}

void ArutRenderProfile3D::set_use_debanding(bool p_value) {
    use_debanding = p_value;
    _apply_if_active();
}

void ArutRenderProfile3D::set_fsr_sharpness(real_t p_value) {
    fsr_sharpness = std::clamp(p_value, static_cast<real_t>(0.0), static_cast<real_t>(1.0));
    _apply_if_active();
}

void ArutRenderProfile3D::set_texture_mipmap_bias(real_t p_value) {
    texture_mipmap_bias = std::clamp(p_value, static_cast<real_t>(-4.0), static_cast<real_t>(4.0));
    _apply_if_active();
}

void ArutRenderProfile3D::set_positional_shadow_atlas_size(int32_t p_value) {
    const int32_t sizes[] = { 256, 512, 1024, 2048, 4096 };
    int32_t nearest = sizes[0];
    for (int32_t size : sizes) {
        if (std::abs(size - p_value) < std::abs(nearest - p_value)) {
            nearest = size;
        }
    }
    positional_shadow_atlas_size = nearest;
    _apply_if_active();
}

void ArutRenderProfile3D::set_apply_on_ready(bool p_value) {
    apply_on_ready = p_value;
}

void ArutRenderProfile3D::set_apply_on_change(bool p_value) {
    apply_on_change = p_value;
    if (apply_on_change) {
        _apply_if_active();
    }
}

void ArutRenderProfile3D::apply_to_current_viewport() const {
    // Explicit application bypasses automatic-application flags by design.
    apply_to_viewport(get_viewport());
}

void ArutRenderProfile3D::apply_now() {
    // Keep the original native API available for existing callers.
    apply_to_current_viewport();
}

void ArutRenderProfile3D::apply_to_viewport(Viewport *p_viewport) const {
    if (p_viewport == nullptr) {
        return;
    }

    p_viewport->set_scaling_3d_mode(static_cast<Viewport::Scaling3DMode>(get_effective_scaling_mode()));
    p_viewport->set_scaling_3d_scale(static_cast<float>(render_scale));
    p_viewport->set_msaa_3d(static_cast<Viewport::MSAA>(msaa_3d));
    p_viewport->set_use_taa(use_taa);
    p_viewport->set_screen_space_aa(use_taa ? Viewport::SCREEN_SPACE_AA_DISABLED : static_cast<Viewport::ScreenSpaceAA>(screen_space_aa));
    p_viewport->set_anisotropic_filtering_level(static_cast<Viewport::AnisotropicFiltering>(anisotropic_filtering));
    p_viewport->set_use_occlusion_culling(use_occlusion_culling);
    p_viewport->set_mesh_lod_threshold(static_cast<float>(mesh_lod_threshold));
    p_viewport->set_use_debanding(use_debanding);
    p_viewport->set_fsr_sharpness(static_cast<float>(fsr_sharpness));
    p_viewport->set_texture_mipmap_bias(static_cast<float>(texture_mipmap_bias));
    p_viewport->set_positional_shadow_atlas_size(positional_shadow_atlas_size);
}
