# Name and distribution decision

## Umbrella name
**ARUT-HARDT Engine**

The name honours the inspiration behind both candidate names while giving the codebase one stable identity. The brand should be checked for trademarks, domains and repository-name collisions before a public release.

## Editions
- **ARUT Mobile** — Android/mobile-first editor/runtime profile.
- **HARDT Studio** — desktop editor profile, initially Linux x86_64 for CI validation.

These are not separate engines and must not diverge into incompatible formats. They share version numbers, engine APIs, native extensions, shader assets and project formats.

## Branding rule
Keep upstream attribution visible: “ARUT-HARDT Engine is based on Godot Engine.” Don't represent the project as an independent engine from scratch. Keep upstream license and copyright notices and document modifications.

Build artifacts may have distinct names, but all user docs should explain that a project created with either edition is intended to open in the other edition at the same matching engine version, except for clearly documented platform-specific features.
