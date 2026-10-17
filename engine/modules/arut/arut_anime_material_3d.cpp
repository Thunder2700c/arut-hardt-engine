#include "arut_anime_material_3d.h"

ArutAnimeMaterial3D::ArutAnimeMaterial3D() {
    // Start with a mobile-friendly material variant and sensible neutral defaults.
    // Textures remain optional and can be assigned using the inherited Inspector UI.
    set_shading_mode(SHADING_MODE_PER_PIXEL);
    set_diffuse_mode(DIFFUSE_TOON);
    set_specular_mode(SPECULAR_SCHLICK_GGX);
    set_albedo(Color(1.0, 1.0, 1.0, 1.0));
    set_roughness(0.85f);
    set_specular(0.2f);
    set_uv1_scale(Vector3(1.0, 1.0, 1.0));
    set_texture_filter(TEXTURE_FILTER_LINEAR_WITH_MIPMAPS);

    // Keep optional texture features disabled until the user enables them in the
    // inherited material Inspector. Rim light is part of the default anime look.
    set_feature(FEATURE_RIM, true);
    set_rim(0.03f);
    set_rim_tint(0.8f);
}

void ArutAnimeMaterial3D::_bind_methods() {
}
