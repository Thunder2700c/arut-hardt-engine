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
void report_vulkan_device() {
 VkApplicationInfo app_info{};
 app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
 app_info.pApplicationName = "ARUT-HARDT";
 app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
 app_info.pEngineName = "ARUT-HARDT Engine";
 app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
 app_info.apiVersion = VK_API_VERSION_1_0;
 VkInstanceCreateInfo create_info{};
 create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
 create_info.pApplicationInfo = &app_info;
 VkInstance instance = VK_NULL_HANDLE;
 const VkResult create_result = vkCreateInstance(&create_info, nullptr, &instance);
 if (create_result != VK_SUCCESS) {
  log_warn("Vulkan unavailable (VkResult=%d); native runtime continues without renderer.", static_cast<int>(create_result));
  return;
 }
 std::uint32_t device_count = 0;
 VkResult result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
 if (result != VK_SUCCESS || device_count == 0) {
  log_warn("No Vulkan physical devices (VkResult=%d, count=%u).", static_cast<int>(result), device_count);
  vkDestroyInstance(instance, nullptr); return;
 }
 VkPhysicalDevice devices[8]{};
 const std::uint32_t reported_count = device_count;
 std::uint32_t capacity = device_count < 8 ? device_count : 8;
 result = vkEnumeratePhysicalDevices(instance, &capacity, devices);
 if (result != VK_SUCCESS) {
  log_warn("Vulkan device enumeration failed (VkResult=%d).", static_cast<int>(result));
  vkDestroyInstance(instance, nullptr); return;
 }
 log_info("Vulkan devices=%u (reporting up to %u).", reported_count, capacity);
 for (std::uint32_t i = 0; i < capacity; ++i) {
  VkPhysicalDeviceProperties props{};
  vkGetPhysicalDeviceProperties(devices[i], &props);
  VkPhysicalDeviceMemoryProperties memory{};
  vkGetPhysicalDeviceMemoryProperties(devices[i], &memory);
  std::uint64_t local_bytes = 0;
  for (std::uint32_t h = 0; h < memory.memoryHeapCount; ++h)
   if ((memory.memoryHeaps[h].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
    local_bytes += memory.memoryHeaps[h].size;
  std::uint32_t queues = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(devices[i], &queues, nullptr);
  std::uint32_t extensions = 0;
  const VkResult ext_result = vkEnumerateDeviceExtensionProperties(devices[i], nullptr, &extensions, nullptr);
  log_info("GPU[%u] %s vendor=0x%04x device=0x%04x API=%u.%u.%u driver=%u local=%" PRIu64 "MiB queues=%u extensions=%u extResult=%d",
   i, props.deviceName, props.vendorID, props.deviceID,
   VK_VERSION_MAJOR(props.apiVersion), VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion),
   props.driverVersion, local_bytes / (1024ULL * 1024ULL), queues, extensions, static_cast<int>(ext_result));
 }
 vkDestroyInstance(instance, nullptr);
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
