//! Rust-side systems boundary for ARUT-HARDT.
//!
//! Keep this crate independent of C++ implementation details. Cross-language
//! interfaces must use the versioned C ABI and explicitly owned POD data.

/// Packed ABI version: major in bits 16..23, minor in bits 8..15, patch in bits 0..7.
pub const ABI_VERSION: u32 = (0 << 16) | (1 << 8) | 0;

/// Returns the Rust systems ABI version for native host/runtime validation.
#[no_mangle]
pub extern "C" fn arut_tasks_abi_version() -> u32 {
    ABI_VERSION
}

#[cfg(test)]
mod tests {
    use super::{arut_tasks_abi_version, ABI_VERSION};

    #[test]
    fn exported_abi_version_matches_the_crate_contract() {
        assert_eq!(arut_tasks_abi_version(), ABI_VERSION);
        assert_eq!(ABI_VERSION, 0x0001_00);
    }
}
