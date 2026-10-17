# Release checklist

Do not publish a release until all applicable checks pass.

- [ ] Source revision and Godot upstream revision are pinned.
- [ ] Patch validation passed.
- [ ] Android editor build passed (if shipping ARUT Mobile).
- [ ] Desktop editor build passed (if shipping HARDT Studio).
- [ ] C++ extension built for every advertised platform/ABI.
- [ ] Native smoke-test project opened and passed on each advertised target.
- [ ] No PEAK game assets were included in engine artifacts.
- [ ] Archive contents and file sizes were inspected.
- [ ] Third-party licenses/copyright notices were included.
- [ ] Changelog lists actual changes and known limitations.
- [ ] Artifact checksums were generated.
- [ ] User-facing install, backup and rollback instructions were tested.
