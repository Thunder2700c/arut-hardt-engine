# Logging and diagnostics

ARUT-HARDT writes two complementary diagnostic streams.

## Android runtime log

The NativeActivity writes timestamped engine messages to:

`<app internal data directory>/logs/runtime.log`

The file is flushed after each message and rotates at approximately 2 MiB, retaining one previous file as `runtime.log.1`. The Downloads export concatenates the previous log (when present) and the current log. The same messages are also sent to Android logcat under the `ARUT-HARDT` tag.

When the activity pauses and when the native runtime shuts down, the app exports the current log to:

`Downloads/ARUT-HARDT/ARUT-HARDT-runtime.log`

On Android versions before API 29, the fallback location is the app-specific Documents directory under Android data storage. Export failure does not delete the private log; a warning is written to logcat.

The runtime stream includes:
- engine version and Rust ABI version;
- Android lifecycle, window and input events;
- Vulkan GPU vendor/device IDs, API and driver versions;
- swapchain creation failures and renderer errors;
- five-second frame-loop interval summaries (FPS, average and maximum frame interval);
- runtime shutdown event and input counters.

The current frame timing is CPU-side frame-loop timing, not GPU timestamp-query timing. GPU timings are not claimed until timestamp queries are implemented.

## CI build logs

The Android GitHub Actions workflow uploads a separate `arut-hardt-build-logs-<run-id>` artifact even when the build fails. It contains:
- `build-info.txt`: commit, run ID, runner and toolchain versions;
- `sdk-licenses.log` and `sdk-install.log`;
- `rust-build.log`;
- `android-build.log`.

The APK is uploaded separately as `arut-hardt-android-arm64-debug` when the build succeeds.

Artifacts are retained for 30 days (logs) and 14 days (debug APK). Open the repository's Actions tab, select the run, then download the artifact from the run's Artifacts section.

## Sharing diagnostics

The Android runtime log can be attached directly from the phone's Downloads folder in ChatGPT. The CI logs artifact can be downloaded from GitHub Actions and attached here. Redact personal data before sharing any logs.
