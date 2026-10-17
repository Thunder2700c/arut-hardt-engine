extends Control

func _ready() -> void:
	var report := Label.new()
	report.anchor_left = 0.05
	report.anchor_top = 0.05
	report.anchor_right = 0.95
	report.anchor_bottom = 0.95
	report.offset_left = 12.0
	report.offset_top = 12.0
	report.offset_right = -12.0
	report.offset_bottom = -12.0
	report.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	report.add_theme_font_size_override("font_size", 22)
	add_child(report)

	report.text = "ARUT-HARDT Native C++\n\nChecking extension..."

	if not ClassDB.class_exists("ArutRuntime"):
		report.text = "EXTENSION NOT LOADED\n\nArutRuntime is not registered. Check Godot Output/Debugger for the GDExtension loader error."
		push_error("[ARUT] ArutRuntime is not registered.")
		return

	var instance: Object = ClassDB.instantiate("ArutRuntime")
	if instance == null:
		report.text = "CLASS FOUND, BUT CREATION FAILED\n\nCheck the extension logs."
		push_error("[ARUT] Could not instantiate ArutRuntime.")
		return

	if not instance.has_method("get_build_info"):
		report.text = "CLASS FOUND, BUT METHOD IS MISSING."
		instance.free()
		push_error("[ARUT] get_build_info() is missing.")
		return

	var info: String = str(instance.call("get_build_info"))
	instance.free()

	report.text = "NATIVE C++ EXTENSION LOADED\n\n" + info + "\n\nSMOKE TEST PASSED"
	print("[ARUT] SMOKE TEST PASSED: ", info)
