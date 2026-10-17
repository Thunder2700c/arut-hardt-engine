#pragma once

#include "scene/resources/material.h"

// First-party anime/PBR hybrid material preset. It inherits Godot's native
// StandardMaterial3D controls for albedo, normal/AO textures, UV scale, roughness,
// metallic/specular, and other surface features. The default is tuned for toon
// diffuse shading with restrained GGX highlights and rim lighting.
class ArutAnimeMaterial3D : public StandardMaterial3D {
    GDCLASS(ArutAnimeMaterial3D, StandardMaterial3D);

protected:
    static void _bind_methods();

public:
    ArutAnimeMaterial3D();
};
