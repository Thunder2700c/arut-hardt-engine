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
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>
#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <jni.h>
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
std::mutex g_log_mutex;
FILE* g_log_file = nullptr;
char g_log_path[1024]{};
constexpr long kMaxLogBytes = 2L * 1024L * 1024L;

void open_file_log(const char* internal_data_path) {
 if (!internal_data_path || !*internal_data_path) return;
 char directory[1024]{};
 std::snprintf(directory, sizeof(directory), "%s/logs", internal_data_path);
 if (mkdir(directory, 0700) != 0 && errno != EEXIST) return;
 std::snprintf(g_log_path, sizeof(g_log_path), "%s/runtime.log", directory);
 g_log_file = std::fopen(g_log_path, "a");
 if (g_log_file) {
  std::setvbuf(g_log_file, nullptr, _IOLBF, 0);
 }
}

void write_log(int priority, const char* level, const char* format, va_list args) {
 char message[2048]{};
 std::vsnprintf(message, sizeof(message), format, args);
 __android_log_write(priority, kLogTag, message);
 std::lock_guard<std::mutex> lock(g_log_mutex);
 if (!g_log_file) return;
 timespec now{};
 clock_gettime(CLOCK_REALTIME, &now);
 std::tm local{};
 localtime_r(&now.tv_sec, &local);
 char timestamp[40]{};
 std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &local);
 std::fprintf(g_log_file, "%s.%03ld %s %s\n", timestamp, now.tv_nsec / 1000000L, level, message);
 std::fflush(g_log_file);
 const long position = std::ftell(g_log_file);
 if (position > kMaxLogBytes) {
  std::fclose(g_log_file);
  g_log_file = nullptr;
  char previous[1050]{};
  std::snprintf(previous, sizeof(previous), "%s.1", g_log_path);
  std::remove(previous);
  std::rename(g_log_path, previous);
  g_log_file = std::fopen(g_log_path, "a");
 }
}

void log_message(int priority, const char* level, const char* message) {
 __android_log_write(priority, kLogTag, message ? message : "");
 std::lock_guard<std::mutex> lock(g_log_mutex);
 if (!g_log_file) return;
 timespec now{};
 clock_gettime(CLOCK_REALTIME, &now);
 std::tm local{};
 localtime_r(&now.tv_sec, &local);
 char timestamp[40]{};
 std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &local);
 std::fprintf(g_log_file, "%s.%03ld %s %s\n", timestamp, now.tv_nsec / 1000000L, level, message ? message : "");
 std::fflush(g_log_file);
 const long position = std::ftell(g_log_file);
 if (position > kMaxLogBytes) {
  std::fclose(g_log_file); g_log_file = nullptr;
  char previous[1050]{};
  std::snprintf(previous, sizeof(previous), "%s.1", g_log_path);
  std::remove(previous); std::rename(g_log_path, previous);
  g_log_file = std::fopen(g_log_path, "a");
 }
}
void log_info(const char* format, ...) {
 va_list args; va_start(args, format);
 char message[2048]{}; std::vsnprintf(message, sizeof(message), format, args); va_end(args);
 log_message(ANDROID_LOG_INFO, "INFO", message);
}
void log_warn(const char* format, ...) {
 va_list args; va_start(args, format);
 write_log(ANDROID_LOG_WARN, "WARN", format, args);
 va_end(args);
}
bool export_runtime_log(android_app* app) {
 if (!app || !app->activity || !app->activity->vm || !g_log_path[0]) return false;
 JavaVM* vm = app->activity->vm;
 JNIEnv* env = nullptr;
 bool attached = false;
 const jint state = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
 if (state == JNI_EDETACHED) {
  if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK) return false;
  attached = true;
 } else if (state != JNI_OK) return false;

 bool exported = false;
 jobject activity = app->activity->clazz;
 jclass activity_class = env->GetObjectClass(activity);
 jmethodID get_loader = activity_class ? env->GetMethodID(activity_class, "getClassLoader", "()Ljava/lang/ClassLoader;") : nullptr;
 jobject loader = get_loader ? env->CallObjectMethod(activity, get_loader) : nullptr;
 jclass loader_class = loader ? env->GetObjectClass(loader) : nullptr;
 jmethodID load_class = loader_class ? env->GetMethodID(loader_class, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;") : nullptr;
 jstring class_name = env->NewStringUTF("com.arut.engine.LogExporter");
 jobject exporter_object = (load_class && class_name) ? env->CallObjectMethod(loader, load_class, class_name) : nullptr;
 jclass exporter = static_cast<jclass>(exporter_object);
 jmethodID export_method = exporter ? env->GetStaticMethodID(exporter, "exportRuntimeLog", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;)Z") : nullptr;
 jstring source_path = env->NewStringUTF(g_log_path);
 char previous_path[1050]{};
 std::snprintf(previous_path, sizeof(previous_path), "%s.1", g_log_path);
 jstring previous_log_path = env->NewStringUTF(previous_path);
 if (export_method && source_path && previous_log_path) {
  exported = env->CallStaticBooleanMethod(exporter, export_method, activity, source_path, previous_log_path) == JNI_TRUE;
 }
 if (env->ExceptionCheck()) {
  env->ExceptionDescribe();
  env->ExceptionClear();
  exported = false;
 }
 if (source_path) env->DeleteLocalRef(source_path);
 if (previous_log_path) env->DeleteLocalRef(previous_log_path);
 if (exporter_object) env->DeleteLocalRef(exporter_object);
 if (class_name) env->DeleteLocalRef(class_name);
 if (loader_class) env->DeleteLocalRef(loader_class);
 if (loader) env->DeleteLocalRef(loader);
 if (activity_class) env->DeleteLocalRef(activity_class);
 if (attached) vm->DetachCurrentThread();
 return exported;
}
void close_file_log() {
 std::lock_guard<std::mutex> lock(g_log_mutex);
 if (g_log_file) { std::fflush(g_log_file); std::fclose(g_log_file); g_log_file = nullptr; }
}
void handle_app_command(android_app* app, std::int32_t command) {
 auto* state = static_cast<RuntimeState*>(app->userData);
 if (!state) return;
 ++state->lifecycle_events;
 switch (command) {
  case APP_CMD_START: log_info("Lifecycle START"); break;
  case APP_CMD_RESUME: state->resumed = true; log_info("Lifecycle RESUME"); break;
  case APP_CMD_PAUSE: state->resumed = false; log_info("Lifecycle PAUSE");
   if (export_runtime_log(app)) __android_log_print(ANDROID_LOG_INFO, kLogTag, "Runtime log exported to Downloads/ARUT-HARDT/ARUT-HARDT-runtime.log");
   else __android_log_print(ANDROID_LOG_WARN, kLogTag, "Runtime log export failed; private app log retained.");
   break;
  case APP_CMD_STOP: log_info("Lifecycle STOP"); break;
  case APP_CMD_INIT_WINDOW:
   state->has_window = app->window != nullptr;
   if (state->has_window) {
    log_info("Window created: %dx%d", ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
    if (!state->renderer.initialize(app->window, app->activity->assetManager)) log_warn("Vulkan renderer unavailable; activity remains alive.");
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
   const std::size_t p = static_cast<std::size_t>((AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
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
extern "C" void arut_android_log(int priority, const char* message) noexcept {
 const char* level = priority >= ANDROID_LOG_ERROR ? "ERROR" : (priority >= ANDROID_LOG_WARN ? "WARN" : "INFO");
 log_message(priority, level, message);
}
namespace arut::platform::android {
void run(android_app* app) {
 RuntimeState state{};
 open_file_log(app->activity ? app->activity->internalDataPath : nullptr);
 app->userData = &state;
 app->onAppCmd = handle_app_command;
 app->onInputEvent = handle_input;
 const auto v = arut::core::version();
 log_info("%.*s %u.%u.%u starting (NativeActivity).",
  static_cast<int>(arut::core::name().size()), arut::core::name().data(), v.major, v.minor, v.patch);
 log_info("Rust systems ABI version: 0x%06x", arut_tasks_abi_version());
 if (g_log_file) log_info("Persistent runtime log: %s", g_log_path);
 else log_warn("Persistent runtime log unavailable; logcat remains active.");
 auto metrics_start = std::chrono::steady_clock::now();
 std::uint64_t frame_count = 0;
 double frame_ms_total = 0.0;
 double frame_ms_max = 0.0;
 while (app->destroyRequested == 0) {
  const auto frame_start = std::chrono::steady_clock::now();
  android_poll_source* source = nullptr;
  const bool can_render = state.resumed && state.has_window && state.focused && state.renderer.ready();
  const int timeout = can_render ? 16 : (state.has_window ? 100 : -1);
  const int ident = ALooper_pollOnce(timeout, nullptr, nullptr, reinterpret_cast<void**>(&source));
  if (ident >= 0 && source) source->process(app, source);
  if (ident == ALOOPER_POLL_ERROR) log_warn("Android looper returned an error.");
  if (state.resumed && state.has_window && state.focused && state.renderer.ready()) {
   if (!state.renderer.draw_frame()) log_warn("Vulkan frame submission/presentation failed.");
   else {
    const double elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - frame_start).count();
    frame_ms_total += elapsed;
    frame_ms_max = std::max(frame_ms_max, elapsed);
    ++frame_count;
   }
  }
  const auto metrics_now = std::chrono::steady_clock::now();
  const double metrics_seconds = std::chrono::duration<double>(metrics_now - metrics_start).count();
  if (metrics_seconds >= 5.0) {
   if (frame_count > 0) log_info("PERF window=%.2fs fps=%.2f frame_interval_avg_ms=%.3f frame_interval_max_ms=%.3f frames=%" PRIu64,
    metrics_seconds, static_cast<double>(frame_count) / metrics_seconds,
    frame_ms_total / static_cast<double>(frame_count), frame_ms_max, frame_count);
   else log_info("PERF window=%.2fs renderer_idle=true", metrics_seconds);
   if (state.renderer.ready()) {
    const auto render = state.renderer.profile_snapshot();
    log_info("RENDER_PROFILE cpu_frame_ms=%.3f gpu_frame_ms=%.3f gpu_timestamps=%s draw_calls=%u visible_objects=%u triangles=%u",
     render.cpu_frame_ms, render.gpu_frame_ms, render.gpu_timing_available ? "available" : "unavailable",
     render.draw_calls, render.visible_objects, render.triangles);
   }
   metrics_start = metrics_now; frame_count = 0; frame_ms_total = 0.0; frame_ms_max = 0.0;
  }
 }
 log_info("Runtime shutdown. lifecycle=%" PRIu64 " input=%" PRIu64, state.lifecycle_events, state.input_events);
 close_file_log();
 if (export_runtime_log(app)) __android_log_print(ANDROID_LOG_INFO, kLogTag, "Final runtime log exported to Downloads/ARUT-HARDT/ARUT-HARDT-runtime.log");
 else __android_log_print(ANDROID_LOG_WARN, kLogTag, "Final runtime log export failed; private app log retained.");
}
}
void android_main(android_app* app) {
 arut::platform::android::run(app);
}
