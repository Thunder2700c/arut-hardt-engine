#pragma once

#include <algorithm>

#include "core/math/math_defs.h"
#include "core/variant/dictionary.h"
#include "scene/main/node.h"
#include "scene/main/viewport.h"

// Native viewport quality profile for ARUT-HARDT. This configures Godot's
// existing renderer; it is not a replacement renderer or a Nanite implementation.
class ArutRenderProfile3D : public Node {
    GDCLASS(ArutRenderProfile3D, Node);

public:
    enum QualityPreset {
        MOBILE = 0,
        BALANCED = 1,
        HIGH = 2,
        CUSTOM = 3
    };

private:
    int32_t preset = MOBILE;
    real_t render_scale = 0.80;
    int32_t scaling_mode = Viewport::SCALING_3D_MODE_FSR;
    int32_t msaa_3d = Viewport::MSAA_DISABLED;
    int32_t screen_space_aa = Viewport::SCREEN_SPACE_AA_FXAA;
    bool use_taa = false;
    bool use_occlusion_culling = true;
    real_t mesh_lod_threshold = 1.5;
    bool use_debanding = true;
    real_t fsr_sharpness = 0.25;
    real_t texture_mipmap_bias = 0.0;
    int32_t positional_shadow_atlas_size = 1024;
    bool apply_on_ready = true;
    bool auto_apply_changes = true;

    void _apply_if_active();

protected:
    void _on_ready();

    static void _bind_methods();
    void _notification(int p_what);

public:
    void set_preset(int32_t value);
    int32_t get_preset() const { return preset; }

    void set_render_scale(real_t value);
    real_t get_render_scale() const { return render_scale; }
    void set_scaling_mode(int32_t value);
    int32_t get_scaling_mode() const { return scaling_mode; }
    void set_msaa_3d(int32_t value);
    int32_t get_msaa_3d() const { return msaa_3d; }
    void set_screen_space_aa(int32_t value);
    int32_t get_screen_space_aa() const { return screen_space_aa; }
    void set_use_taa(bool value);
    bool get_use_taa() const { return use_taa; }
    void set_use_occlusion_culling(bool value);
    bool get_use_occlusion_culling() const { return use_occlusion_culling; }
    void set_mesh_lod_threshold(real_t value);
    real_t get_mesh_lod_threshold() const { return mesh_lod_threshold; }
    void set_use_debanding(bool value);
    bool get_use_debanding() const { return use_debanding; }
    void set_fsr_sharpness(real_t value);
    real_t get_fsr_sharpness() const { return fsr_sharpness; }
    void set_texture_mipmap_bias(real_t value);
    real_t get_texture_mipmap_bias() const { return texture_mipmap_bias; }
    void set_positional_shadow_atlas_size(int32_t value);
    int32_t get_positional_shadow_atlas_size() const { return positional_shadow_atlas_size; }
    void set_apply_on_ready(bool value);
    bool get_apply_on_ready() const { return apply_on_ready; }
    void set_auto_apply_changes(bool value);
    bool get_auto_apply_changes() const { return auto_apply_changes; }

    // Applies this profile to the node's current viewport. Safe to call manually.
    void apply_profile();
    void apply_to_viewport(Viewport *p_viewport) const;

    // Returns native viewport counters. These are per-frame counters, not FPS
    // or GPU timings; values can be zero before the viewport has rendered.
    Dictionary get_viewport_metrics() const;
};

VARIANT_ENUM_CAST(ArutRenderProfile3D::QualityPreset);
