# Godot source patches

Put one reviewed Godot patch per `*.patch` file here. Both editor workflows target the pinned `godotengine/godot:4.7-stable` source, check patches with `git apply --check`, and apply them in filename order.

## Active patch series

| Patch | Scope | Why a source patch is required | Validation |
|---|---|---|---|
| `0001-arut-editor-identity.patch` | Default theme, editor/project-manager title, Android application label and editor logos | Product identity is part of the engine/editor shell and cannot be fixed by a project shader or GDExtension | Apply-check and compile Android ARM64/Linux editor |
| `0002-arut-about-and-project-manager.patch` | About dialog and project-manager welcome copy | These are built-in editor UI surfaces; retain the explicit Godot basis statement and legal notices | Apply-check and inspect resulting editor UI |
| `0003-arut-runtime-identity.patch` | Product-facing version name and Android run-window label | The startup/version banner is generated from upstream version metadata, while the Android play-window label is a native resource string | Apply-check; compile once and inspect startup log plus Android run window |

Keep `short_name = "godot"` in `version.py` intentionally: this patch changes the human-facing product name without casually changing internal compatibility/path assumptions. Never remove required upstream copyright or licence notices. Avoid patching renderer internals before simpler engine-level options have been evaluated.
