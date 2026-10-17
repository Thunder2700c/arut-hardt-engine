shader_type spatial;
render_mode world_vertex_coords, diffuse_toon, specular_schlick_ggx, cull_back;

// ARUT-HARDT Anime Surface v0.1
// Lightweight art-directed surface shader. Expensive GI/SSR effects are not required.
// In triplanar mode, projections use world-space position. Keep texture_scale explicit.

uniform sampler2D albedo_texture : source_color, filter_linear_mipmap, repeat_enable;
uniform sampler2D normal_texture : hint_normal, filter_linear_mipmap, repeat_enable;
uniform sampler2D ao_texture : hint_default_white, filter_linear_mipmap, repeat_enable;

uniform vec4 tint : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform float texture_scale : hint_range(0.01, 8.0, 0.01) = 1.0;
uniform bool use_triplanar = false;
uniform float triplanar_sharpness : hint_range(1.0, 16.0, 1.0) = 4.0;

uniform bool use_normal_map = false;
uniform float normal_depth : hint_range(0.0, 1.0, 0.05) = 1.0;
uniform bool use_ao_map = false;

uniform float surface_roughness : hint_range(0.0, 1.0, 0.01) = 0.9;
uniform float specular_strength : hint_range(0.0, 1.0, 0.01) = 0.2;
uniform float rim_strength : hint_range(0.0, 0.12, 0.005) = 0.02;

varying vec3 world_position;
varying vec3 world_normal;

void vertex() {
    // world_vertex_coords puts VERTEX and NORMAL into world coordinates here.
    world_position = VERTEX;
    world_normal = normalize(NORMAL);
}

void fragment() {
    vec3 n = normalize(world_normal);
    vec2 scaled_uv = UV * texture_scale;
    vec3 base_color = vec3(1.0);
    AO = 1.0;

    if (use_triplanar) {
        vec3 weights = pow(abs(n), vec3(triplanar_sharpness));
        weights /= max(weights.x + weights.y + weights.z, 0.0001);

        vec3 color_x = texture(albedo_texture, world_position.zy * texture_scale).rgb;
        vec3 color_y = texture(albedo_texture, world_position.xz * texture_scale).rgb;
        vec3 color_z = texture(albedo_texture, world_position.xy * texture_scale).rgb;
        base_color = color_x * weights.x + color_y * weights.y + color_z * weights.z;

        if (use_ao_map) {
            float ao_x = texture(ao_texture, world_position.zy * texture_scale).r;
            float ao_y = texture(ao_texture, world_position.xz * texture_scale).r;
            float ao_z = texture(ao_texture, world_position.xy * texture_scale).r;
            AO = ao_x * weights.x + ao_y * weights.y + ao_z * weights.z;
        }

        if (use_normal_map) {
            vec3 sample_x = texture(normal_texture, world_position.zy * texture_scale).rgb * 2.0 - 1.0;
            vec3 sample_y = texture(normal_texture, world_position.xz * texture_scale).rgb * 2.0 - 1.0;
            vec3 sample_z = texture(normal_texture, world_position.xy * texture_scale).rgb * 2.0 - 1.0;

            // Convert the three projection samples into approximate world-space normals.
            vec3 normal_x = normalize(vec3(sample_x.z * sign(n.x), sample_x.y, sample_x.x));
            vec3 normal_y = normalize(vec3(sample_y.x, sample_y.z * sign(n.y), sample_y.y));
            vec3 normal_z = normalize(vec3(sample_z.x, sample_z.y, sample_z.z * sign(n.z)));
            vec3 mapped_world_normal = normalize(
                normal_x * weights.x + normal_y * weights.y + normal_z * weights.z
            );
            mapped_world_normal = normalize(mix(n, mapped_world_normal, normal_depth));

            // Godot's fragment NORMAL is in view space.
            NORMAL = normalize((VIEW_MATRIX * vec4(mapped_world_normal, 0.0)).xyz);
        }
    } else {
        base_color = texture(albedo_texture, scaled_uv).rgb;

        if (use_ao_map) {
            AO = texture(ao_texture, scaled_uv).r;
        }

        if (use_normal_map) {
            NORMAL_MAP = texture(normal_texture, scaled_uv).rgb;
            NORMAL_MAP_DEPTH = normal_depth;
        }
    }

    ALBEDO = base_color * tint.rgb;
    ROUGHNESS = surface_roughness;
    SPECULAR = specular_strength;
    RIM = rim_strength;
    RIM_TINT = 0.8;
}
