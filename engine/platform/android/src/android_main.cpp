#include "arut/platform/android/runtime.hpp"
#include "arut/core/version.hpp"
#include "arut/rendering/vulkan/android_renderer.hpp"
#include <android/input.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <vulkan/vulkan.h>
#include <cinttypes>
#include <cstdint>
#include <cstddef>
#include <stdarg.h>
extern "C" std::uint32_t arut_tasks_abi_version() noexcept;
namespace {
constexpr char kLogTag[] = "ARUT-HARDT";
struct RuntimeState final {
 bool resumed{false};
 bool focused{false};
 bool has_window{false};
 arut::rendering::vulkan::AndroidRenderer renderer{};
 std::uint64_t lifecycle_events{0};
 std::uint64_t input_events{0};
};
void log_info(const char* format, ...) {
 va_list args; va_start(args, format);
 __android_log_vprint(ANDROID_LOG_INFO, kLogTag, format, args);
 va_end(args);
}
void log_warn(const char* format, ...) {
 va_list args; va_start(args, format);
 __android_log_vprint(ANDROID_LOG_WARN, kLogTag, format, args);
 va_end(args);
}
void handle_app_command(android_app* app, std::int32_t command) {
 auto* state = static_cast<RuntimeState*>(app->userData);
 if (!state) return;
 ++state->lifecycle_events;
 switch (command) {
  case APP_CMD_START: log_info("Lifecycle START"); break;
  case APP_CMD_RESUME: state->resumed = true; log_info("Lifecycle RESUME"); break;
  case APP_CMD_PAUSE: state->resumed = false; log_info("Lifecycle PAUSE"); break;
  case APP_CMD_STOP: log_info("Lifecycle STOP"); break;
  case APP_CMD_INIT_WINDOW:
   state->has_window = app->window != nullptr;
   if (state->has_window) {
    log_info("Window created: %dx%d", ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
    if (!state->renderer.initialize(app->window)) log_warn("Vulkan renderer unavailable; activity remains alive.");
   }
   break;
  case APP_CMD_TERM_WINDOW:
   state->has_window = false;
   state->renderer.shutdown();
   log_info("Window destroyed; Vulkan surface and swapchain released.");
   break;
  case APP_CMD_GAINED_FOCUS: state->focused = true; log_info("Focus gained"); break;
  case APP_CMD_LOST_FOCUS: state->focused = false; log_info("Focus lost"); break;
  case APP_CMD_CONFIG_CHANGED:
   if (app->window) log_info("Configuration changed: %dx%d", ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
   break;
  case APP_CMD_LOW_MEMORY: log_warn("Android low-memory callback received."); break;
  case APP_CMD_DESTROY: log_info("Lifecycle DESTROY"); break;
  default: break;
 }
}
int32_t handle_input(android_app* app, AInputEvent* event) {
 auto* state = static_cast<RuntimeState*>(app->userData);
 if (!state || !event) return 0;
 ++state->input_events;
 if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
  const auto action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
  if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN ||
      action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP) {
   const std::size_t p = AMotionEvent_getActionIndex(event);
   log_info("Touch action=%d pointer=%zu x=%.1f y=%.1f", action, p,
            AMotionEvent_getX(event, p), AMotionEvent_getY(event, p));
  }
  return 1;
 }
 if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
  log_info("Key action=%d keycode=%d", AKeyEvent_getAction(event), AKeyEvent_getKeyCode(event));
  return 0;
 }
 return 0;
}
}
namespace arut::platform::android {
void run(android_app* app) {
 RuntimeState state{};
 app->userData = &state;
 app->onAppCmd = handle_app_command;
 app->onInputEvent = handle_input;
 const auto v = arut::core::version();
 log_info("%.*s %u.%u.%u starting (NativeActivity).",
  static_cast<int>(arut::core::name().size()), arut::core::name().data(), v.major, v.minor, v.patch);
 log_info("Rust systems ABI version: 0x%06x", arut_tasks_abi_version());
 while (app->destroyRequested == 0) {
  android_poll_source* source = nullptr;
  const bool can_render = state.resumed && state.has_window && state.focused && state.renderer.ready();
  const int timeout = can_render ? 16 : (state.has_window ? 100 : -1);
  const int ident = ALooper_pollOnce(timeout, nullptr, nullptr, reinterpret_cast<void**>(&source));
  if (ident >= 0 && source) source->process(app, source);
  if (ident == ALOOPER_POLL_ERROR) log_warn("Android looper returned an error.");
  if (can_render && !state.renderer.draw_frame()) log_warn("Vulkan frame submission/presentation failed.");
 }
 log_info("Runtime shutdown. lifecycle=%" PRIu64 " input=%" PRIu64, state.lifecycle_events, state.input_events);
}
}
void android_main(android_app* app) {
 app_dummy();
 arut::platform::android::run(app);
}
