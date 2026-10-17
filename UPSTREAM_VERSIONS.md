# Upstream revisions

These are the source versions targeted by the initial build setup. Update them deliberately as a paired version change, then rebuild and verify the native API and editor.

- Godot source: `godotengine/godot`, tag `4.7-stable`.
- C++ bindings: `godotengine/godot-cpp`, commit `2bb1acf6f8ae1ce96ac7b9ba3a10b499c59c49ec` (selected from master on 2026-10-17; supports API 4.7 according to its SCons configuration).

The Godot tag is a fixed release tag; the godot-cpp revision is an immutable commit SHA. Check the actual build results before declaring this pairing validated. These initial builds intentionally target the editor version already installed for PEAK, rather than silently upgrading the project to another Godot minor version.
