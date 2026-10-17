# Godot source patches

Put one reviewed Godot patch per `*.patch` file here. Manual workflows run `git apply --check` before applying patches.

Every patch must be small, single-purpose, identify the upstream revision it targets, explain why a shader/GDExtension cannot solve the need, and include expected impact plus validation steps. Never patch renderer internals before ruling out simpler approaches.
