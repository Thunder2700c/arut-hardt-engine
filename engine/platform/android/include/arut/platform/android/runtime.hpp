#pragma once
#include <cstdint>
struct android_app;
namespace arut::platform::android {
struct RuntimeSnapshot final {
 bool resumed{false};
 bool has_window{false};
 bool focused{false};
 bool destroyed{false};
 std::uint32_t window_width{0};
 std::uint32_t window_height{0};
 std::uint64_t lifecycle_events{0};
 std::uint64_t input_events{0};
};
void run(android_app* app);
}
