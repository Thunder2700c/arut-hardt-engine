#include "arut_render_profile_3d.h"

#include <cstdlib>

#include "core/object/class_db.h"
#include "core/variant/variant.h"

void ArutRenderProfile3D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_preset", "value"), &ArutRenderProfile3D::set_preset);
    ClassDB::bind_method(D_METHOD("get_preset"), &ArutRenderProfile3D::get_preset);
    ClassDB::bind_method(D_METHOD("set_render_scale", "value"), &ArutRenderProfile3D::set_render_scale);
    ClassDB::bind_method(D_METHOD("get_render_scale"), &ArutRenderProfile3D::get_render_scale);
    ClassDB::bind_method(D_METHOD("set_scaling_mode", "value"), &ArutRenderProfile3D::set_scaling_mode);
    ClassDB::bind_method(D_METHOD("get_scaling_mode"), &ArutRenderProfile3D::get_scaling_mode);
    ClassDB::bind_method(D_METHOD("set_msaa_3d", "value"), &ArutRenderProfile3D::set_msaa_3d);
    ClassDB::bind_method(D_METHOD("get_msaa_3d"), &ArutRenderProfile3D::get_msaa_3d);
    ClassDB::bind_method(D_METHOD("set_screen_space_aa", "value"), &ArutRenderProfile3D::set_screen_space_aa);
    ClassDB::bind_method(D_METHOD("get_screen_space_aa"), &ArutRenderProfile3D::get_screen_space_aa);
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
    ClassDB::bind_method(D_METHOD("apply_to_viewport", "viewport"), &ArutRenderProfile3D::apply_to_viewport);

    ADD_GROUP("Quality Profile", "quality_");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "quality_preset", PROPERTY_HINT_ENUM, "Mobile,Balanced,High,Custom"), "set_preset", "get_preset");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "quality_apply_on_ready"), "set_apply_on_ready", "get_apply_on_ready");
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
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "optimization_texture_mipmap_bias", PROPERTY_HINT_RANGE, "-4,4,0.25"), "set_texture_mipmap_bias", "get_texture_mipmap_bias");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "optimization_debanding"), "set_use_debanding", "get_use_debanding");

    ADD_GROUP("Advanced", "advanced_");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "advanced_fsr_sharpness", PROPERTY_HINT_RANGE, "0,1,0.05"), "set_fsr_sharpness", "get_fsr_sharpness");

    BIND_ENUM_CONSTANT(MOBILE);
    BIND_ENUM_CONSTANT(BALANCED);
    BIND_ENUM_CONSTANT(HIGH);
    BIND_ENUM_CONSTANT(CUSTOM);
}

void ArutRenderProfile3D::_ready() {
    if (apply_on_ready) {
        apply_to_viewport(get_viewport());
    }
}

void ArutRenderProfile3D::set_preset(int32_t p_value) {
    preset = std::clamp(p_value, static_cast<int32_t>(MOBILE), static_cast<int32_t>(CUSTOM));
    switch (preset) {
        case MOBILE:
            render_scale = 0.80;
            scaling_mode = Viewport::SCALING_3D_MODE_FSR;
            msaa_3d = Viewport::MSAA_DISABLED;
            screen_space_aa = Viewport::SCREEN_SPACE_AA_FXAA;
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
            scaling_mode = Viewport::SCALING_3D_MODE_FSR;
            msaa_3d = Viewport::MSAA_2X;
            screen_space_aa = Viewport::SCREEN_SPACE_AA_DISABLED;
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
}

void ArutRenderProfile3D::apply_to_viewport(Viewport *p_viewport) const {
    if (p_viewport == nullptr) {
        return;
    }

    p_viewport->set_scaling_3d_mode(static_cast<Viewport::Scaling3DMode>(scaling_mode));
    p_viewport->set_scaling_3d_scale(static_cast<float>(render_scale));
    p_viewport->set_msaa_3d(static_cast<Viewport::MSAA>(msaa_3d));
    p_viewport->set_use_taa(use_taa);
    p_viewport->set_screen_space_aa(use_taa ? Viewport::SCREEN_SPACE_AA_DISABLED : static_cast<Viewport::ScreenSpaceAA>(screen_space_aa));
    p_viewport->set_use_occlusion_culling(use_occlusion_culling);
    p_viewport->set_mesh_lod_threshold(static_cast<float>(mesh_lod_threshold));
    p_viewport->set_use_debanding(use_debanding);
    p_viewport->set_fsr_sharpness(static_cast<float>(fsr_sharpness));
    p_viewport->set_texture_mipmap_bias(static_cast<float>(texture_mipmap_bias));
    p_viewport->set_positional_shadow_atlas_size(positional_shadow_atlas_size);
}
