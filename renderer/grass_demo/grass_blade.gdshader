shader_type spatial;
render_mode cull_disabled, depth_draw_opaque;

// First-party, texture-free grass blade prototype.
// Hard cutouts avoid smooth alpha blending across the whole field.
uniform vec3 base_color : source_color = vec3(0.12, 0.30, 0.055);
uniform vec3 tip_color : source_color = vec3(0.38, 0.57, 0.14);
uniform float wind_strength : hint_range(0.0, 0.45, 0.005) = 0.12;
uniform float wind_speed : hint_range(0.0, 5.0, 0.05) = 1.35;
uniform float spatial_variation : hint_range(0.0, 4.0, 0.05) = 0.85;
uniform vec2 wind_direction = vec2(1.0, 0.28);
uniform float blade_width : hint_range(0.15, 0.49, 0.01) = 0.44;
uniform float tip_width : hint_range(0.0, 0.08, 0.002) = 0.012;
uniform float blade_roughness : hint_range(0.45, 1.0, 0.01) = 0.88;

// INSTANCE_CUSTOM is a vertex-stage built-in; pass values on to fragment().
varying float blade_phase;
varying float blade_tint;

void vertex() {
	// MultiMesh custom data: x = height, y = phase, z = color variation,
	// w = width variation. All custom values are stored in the 0..1 range.
	float height_scale = mix(0.72, 1.28, INSTANCE_CUSTOM.x);
	float width_scale = mix(0.72, 1.35, INSTANCE_CUSTOM.w);
	blade_phase = INSTANCE_CUSTOM.y;
	blade_tint = INSTANCE_CUSTOM.z;

	// QuadMesh is centered at its origin. Keep the blade root at y = 0
	// while applying height/width variation in the vertex stage.
	VERTEX.x *= width_scale;
	VERTEX.y = (VERTEX.y + 0.5) * height_scale - 0.5;

	vec3 world_origin = (MODEL_MATRIX * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
	float phase = TIME * wind_speed
		+ dot(world_origin.xz, vec2(0.73, 1.11)) * spatial_variation
		+ blade_phase * TAU;
	float gust = sin(phase) + 0.28 * sin(phase * 1.73 + 1.9);
	float bend_weight = clamp(1.0 - UV.y, 0.0, 1.0);
	float bend = gust * wind_strength * bend_weight * bend_weight;

	// The demo stores rigid yaw-only instance transforms. Transpose converts
	// the shared world-space wind direction to the current blade's local space.
	vec2 direction_xz = wind_direction;
	if (length(direction_xz) < 0.001) {
		direction_xz = vec2(1.0, 0.0);
	} else {
		direction_xz = normalize(direction_xz);
	}
	vec3 world_wind = vec3(direction_xz.x, 0.0, direction_xz.y);
	vec3 local_wind = normalize(transpose(mat3(MODEL_MATRIX)) * world_wind);
	VERTEX += local_wind * bend;

	// Approximate the normal change caused by the bend so the blade's
	// lighting does not remain completely rigid as the tip moves.
	float normal_alignment = dot(NORMAL.xz, local_wind.xz);
	float bend_slope = 2.0 * gust * wind_strength * bend_weight / max(height_scale, 0.001);
	NORMAL = normalize(vec3(NORMAL.x, NORMAL.y - bend_slope * normal_alignment, NORMAL.z));
}

void fragment() {
	if (!FRONT_FACING) {
		NORMAL = -NORMAL;
	}

	// Procedural tapered silhouette: UV.y = 1 at the root and 0 at the tip.
	float taper = mix(tip_width, blade_width, pow(clamp(UV.y, 0.0, 1.0), 0.82));
	float serration = 0.018 * sin(UV.y * 46.0 + blade_phase * TAU) * UV.y;
	float half_width = max(0.002, taper + serration);
	float distance_from_center = abs(UV.x * 2.0 - 1.0);
	float cutout = step(distance_from_center, half_width);

	float tip_mix = smoothstep(0.04, 1.0, 1.0 - UV.y);
	float color_variation = mix(0.84, 1.12, blade_tint);
	ALBEDO = mix(base_color, tip_color, tip_mix) * color_variation;
	ROUGHNESS = blade_roughness;
	SPECULAR = 0.12;

	// Alpha scissor gives the shader a hard cutout silhouette.
	ALPHA = cutout;
	ALPHA_SCISSOR_THRESHOLD = 0.5;
}
