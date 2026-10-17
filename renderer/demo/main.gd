extends Node3D

const SURFACE_SHADER: Shader = preload("res://shaders/arut_anime_surface.gdshader")

func _ready() -> void:
	_build_demo()


func _build_demo() -> void:
	var world_env := WorldEnvironment.new()
	var environment := Environment.new()
	environment.background_mode = Environment.BG_COLOR
	environment.background_color = Color(0.40, 0.61, 0.82)
	environment.ambient_light_source = Environment.AMBIENT_SOURCE_COLOR
	environment.ambient_light_color = Color(0.72, 0.80, 1.0)
	environment.ambient_light_energy = 0.65
	environment.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	world_env.environment = environment
	add_child(world_env)

	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-48.0, -28.0, 0.0)
	sun.light_color = Color(1.0, 0.88, 0.72)
	sun.light_energy = 1.2
	sun.shadow_enabled = true
	add_child(sun)

	var camera := Camera3D.new()
	camera.position = Vector3(0.0, 5.2, 10.5)
	camera.look_at(Vector3(0.0, 0.8, 0.0), Vector3.UP)
	camera.current = true
	add_child(camera)

	var checker := _make_checker_texture()
	var grass_material := _make_material(checker, Color(0.90, 1.0, 0.86), true)
	grass_material.set_shader_parameter("texture_scale", 1.0)
	grass_material.set_shader_parameter("surface_roughness", 0.95)
	grass_material.set_shader_parameter("rim_strength", 0.0)

	var ground := MeshInstance3D.new()
	var plane := PlaneMesh.new()
	plane.size = Vector2(18.0, 18.0)
	ground.mesh = plane
	ground.material_override = grass_material
	add_child(ground)

	var character_sphere := MeshInstance3D.new()
	var sphere := SphereMesh.new()
	sphere.radius = 1.0
	sphere.height = 2.0
	character_sphere.mesh = sphere
	character_sphere.position = Vector3(0.0, 1.0, 0.0)
	character_sphere.material_override = _make_material(checker, Color(0.92, 0.78, 0.72), false)
	add_child(character_sphere)

	var box_left := MeshInstance3D.new()
	var box_mesh := BoxMesh.new()
	box_mesh.size = Vector3(1.25, 2.0, 1.25)
	box_left.mesh = box_mesh
	box_left.position = Vector3(-2.25, 1.0, -0.2)
	box_left.material_override = _make_material(checker, Color(0.62, 0.78, 1.0), true)
	add_child(box_left)

	var box_right := MeshInstance3D.new()
	var box_mesh_right := BoxMesh.new()
	box_mesh_right.size = Vector3(1.25, 1.25, 1.25)
	box_right.mesh = box_mesh_right
	box_right.position = Vector3(2.25, 0.625, -0.4)
	box_right.material_override = _make_material(checker, Color(1.0, 0.73, 0.48), true)
	add_child(box_right)

	var title := Label.new()
	title.text = "ARUT-HARDT ENGINE  |  ANIME SURFACE v0.1\nUV + TRIPLANAR MATERIAL TEST"
	title.position = Vector2(24.0, 24.0)
	title.add_theme_font_size_override("font_size", 20)
	var overlay := CanvasLayer.new()
	overlay.add_child(title)
	add_child(overlay)


func _make_material(albedo: Texture2D, color_tint: Color, triplanar: bool) -> ShaderMaterial:
	var material := ShaderMaterial.new()
	material.shader = SURFACE_SHADER
	material.set_shader_parameter("albedo_texture", albedo)
	material.set_shader_parameter("tint", color_tint)
	material.set_shader_parameter("texture_scale", 1.0)
	material.set_shader_parameter("use_triplanar", triplanar)
	material.set_shader_parameter("triplanar_sharpness", 4.0)
	material.set_shader_parameter("use_normal_map", false)
	material.set_shader_parameter("use_ao_map", false)
	material.set_shader_parameter("surface_roughness", 0.9)
	material.set_shader_parameter("specular_strength", 0.18)
	material.set_shader_parameter("rim_strength", 0.02)
	return material


func _make_checker_texture() -> ImageTexture:
	var image := Image.create(64, 64, false, Image.FORMAT_RGBA8)
	for y in range(64):
		for x in range(64):
			var cell: int = (floori(float(x) / 8.0) + floori(float(y) / 8.0)) % 2
			var color := Color(0.30, 0.58, 0.31) if cell == 0 else Color(0.40, 0.68, 0.37)
			image.set_pixel(x, y, color)
	return ImageTexture.create_from_image(image)
