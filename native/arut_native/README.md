# ARUT native C++ layer

This is the first GDExtension proof, not a claim that existing GDScript has been converted. `ArutRuntime` exposes `get_build_info()` so an isolated test project can confirm class registration and instantiation.

Build dynamic libraries for the exact Godot API, platform, architecture and build type. The workflow packages the libraries and `.gdextension` manifest; generated binaries must not be committed.

Only port measured CPU hot paths one at a time. Keep existing project scenes and scripts working while validating native code.
