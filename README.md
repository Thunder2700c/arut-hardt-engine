# ARUT-HARDT Engine

A Godot-based open-source engine project focused on mobile-first performance, approachable workflows, native C++ systems and stylized rendering.

## One engine, two editions
- **ARUT Mobile:** Android-first editor/runtime and scalable mobile graphics defaults.
- **HARDT Studio:** desktop editor distribution, starting with Linux x86_64.

Both editions share the same code, project format, version and APIs. They are not separate engines.

## Status
Early foundation. This is not yet a complete engine fork or custom renderer. CI fetches the matching Godot source at build time, applies reviewed patches from `engine/patches/`, and builds the targets. Native extensions live under `native/`; renderer resources and isolated tests live under `renderer/` and `tests/`.

This repository must not contain PEAK game assets. The PEAK game remains separate.

Read [DEVELOPMENT_PROTOCOL.md](DEVELOPMENT_PROTOCOL.md) and [ROADMAP.md](ROADMAP.md) before making changes. We will delay the first on-device user test until a combined vertical slice is ready: reproducible editor build, C++ extension smoke test, and one lightweight stylized material demo.

Keep Godot and third-party notices in any distributable build; see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
