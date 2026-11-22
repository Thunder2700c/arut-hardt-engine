# C++/Rust bridge

All cross-language calls must pass through a narrow, versioned C ABI.

Use fixed-layout C-compatible types, integer handles, explicit lengths, and explicit ownership. Never pass C++ or Rust object layouts across this boundary. The current Rust crate exports an ABI version symbol; production bridge types and Android linkage are not implemented yet.
