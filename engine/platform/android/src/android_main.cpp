#include "arut/platform/android/runtime.hpp"
#include "arut/core/version.hpp"
#include "arut/core/cube_stress_scene.hpp"
#include "arut/core/asset_manager.hpp"
#include "arut/core/world_systems.hpp"
#include "arut/core/test_lab_profile.hpp"
#include "arut/core/humanoid_skeleton.hpp"
#include "arut/core/character_controller.hpp"
#include "arut/core/character_input.hpp"
#include "arut/core/character_collision.hpp"
#include "arut/core/third_person_camera.hpp"
#include "arut/platform/android/asset_source.hpp"
#include "arut/rendering/vulkan/android_renderer.hpp"
#include "arut/rendering/humanoid_animation.hpp"
#include "arut/rendering/humanoid_ik.hpp"
#include "arut/rendering/humanoid_skinning.hpp"
#include "arut/animation/pose_history.hpp"
#include "arut/animation/character_animation.hpp"
#include "arut/animation/gltf_importer.hpp"
#include "arut/animation/retarget.hpp"
#include "arut/animation/root_motion.hpp"
#include <android/input.h>
#include <android/asset_manager.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <vulkan/vulkan.h>
#include <cinttypes>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <string>
#include <cctype>
#include <ctime>
#include <mutex>
#include <atomic>
#include <memory>
#include <utility>
#include <cerrno>
#include <sys/stat.h>
#include <unistd.h>
#include <chrono>
#include <exception>
#include <span>
#include <vector>
#include <initializer_list>
#include <jni.h>
#include <stdarg.h>
extern "C" std::uint32_t arut_tasks_abi_version() noexcept;
namespace {
constexpr char kLogTag[] = "ARUT-HARDT";
std::atomic<std::int32_t> g_requested_test_profile{-1};
std::atomic<bool> g_imported_character_enabled{false};
std::atomic<std::uint32_t> g_touch_combat_pending{0U};
std::atomic<bool> g_touch_combat_block_held{false};
struct FootPlantState final {
 arut::core::Vec3 world_anchor{};
 float blend{0.0F};
 bool active{false};
};

struct RuntimeState final {
 bool resumed{false};
 bool focused{false};
 bool has_window{false};
 arut::core::World ecs_world{};
 arut::core::CubeSpawnSystem cube_spawn_system{};
 arut::core::CubeRenderExtractionSystem cube_render_extraction_system{};
 arut::core::SceneGraphSystem scene_graph_system{};
 std::vector<arut::core::TerrainMesh> terrain_cells{};
 arut::core::LodSystem lod_system{};
 arut::core::WorldPartition world_partition{128.0F};
 arut::core::WorldCellStreamer world_streamer{};
 arut::core::LodSystem cell_lod_system{};
 bool ecs_scene_ready{false};
 bool render_scene_ready{false};
 arut::core::TestLabProfile selected_profile{arut::core::TestLabProfile::EcsStress};
 bool terrain_cells_dirty{false};
 arut::rendering::vulkan::AndroidRenderer renderer{};
 arut::core::CharacterController character_controller{};
 arut::core::CharacterInputRouter character_input_router{};
 bool mouse_look_active{false};
 float gamepad_move_x{0.0F};
 float gamepad_move_y{0.0F};
 bool combat_attack_light_pressed{false};
 bool combat_attack_heavy_pressed{false};
 bool combat_attack_kick_pressed{false};
 bool combat_block_held{false};
 bool combat_dodge_pressed{false};
 bool combat_hit_reaction_pressed{false};
 bool combat_recover_pressed{false};
 arut::core::HumanoidSkeleton humanoid_skeleton{arut::core::make_robot_mannequin_skeleton()};
 arut::rendering::HumanoidAnimator humanoid_animator{};
 std::vector<arut::animation::AnimationClip> humanoid_animation_library{};
 arut::animation::CharacterAnimationController humanoid_clip_animator{};
 bool humanoid_clip_animator_ready{false};
 arut::animation::PoseHistory animation_pose_history{};
 arut::animation::SkeletalAsset imported_character_asset{};
 arut::animation::CharacterAnimationController imported_character_animator{};
 arut::animation::Pose imported_character_rest_pose{};
 bool imported_character_active{false};
 bool imported_character_animator_ready{false};
 float imported_character_model_scale{1.0F};
 float imported_character_bounds_min_x{0.0F};
 float imported_character_bounds_min_y{0.0F};
 float imported_character_bounds_min_z{0.0F};
 float imported_character_bounds_center_x{0.0F};
 float imported_character_bounds_center_z{0.0F};
 std::size_t imported_character_root_joint{arut::animation::kMaxPoseJoints};
 double animation_clock_seconds{0.0};
 float previous_facing_yaw{0.0F};
 bool has_previous_facing_yaw{false};
 std::array<arut::animation::MotionTrajectoryPoint, arut::animation::kMotionTrajectorySamples> predicted_trajectory{};
 bool predicted_trajectory_valid{false};
 std::array<FootPlantState, 2> foot_plants{};
 std::array<bool, 2> foot_ik_clamped{};
 std::array<arut::core::CharacterAabbObstacle, 6> arena_obstacles{{
  {{-8.0F,-2.0F,-8.0F},{-7.6F,3.0F,8.0F}},
  {{ 7.6F,-2.0F,-8.0F},{ 8.0F,3.0F,8.0F}},
  {{-8.0F,-2.0F,-8.0F},{ 8.0F,3.0F,-7.6F}},
  {{-8.0F,-2.0F, 7.6F},{ 8.0F,3.0F,8.0F}},
  {{ 2.0F,-2.0F,-3.4F},{ 3.5F,1.0F,-1.9F}},
  {{-3.5F,-2.0F,-5.5F},{-2.0F,0.7F,-4.0F}}
 }};
 std::array<arut::rendering::Mat4, 6> arena_render_instances{};
 bool test_arena_enabled{false};
 float camera_follow_distance{4.2F};
 bool camera_terrain_obstructed{false};
 bool camera_geometry_obstructed{false};
 std::uint64_t character_fixed_steps{0};
 std::uint64_t lifecycle_events{0};
 std::uint64_t input_events{0};
};
arut::core::CharacterGroundSample terrain_ground_query(
 float x, float z, void*) noexcept {
 arut::core::CharacterGroundSample sample{};
 const float height = arut::core::TerrainSystem::height(x, z);
 constexpr float epsilon = 0.1F;
 const float dx = (arut::core::TerrainSystem::height(x + epsilon, z) -
                   arut::core::TerrainSystem::height(x - epsilon, z)) /
                  (2.0F * epsilon);
 const float dz = (arut::core::TerrainSystem::height(x, z + epsilon) -
                   arut::core::TerrainSystem::height(x, z - epsilon)) /
                  (2.0F * epsilon);
 const float inverse = 1.0F / std::sqrt(dx * dx + 1.0F + dz * dz);
 if (!std::isfinite(height) || !std::isfinite(inverse)) return sample;
 sample.height = height;
 sample.normal = {-dx * inverse, inverse, -dz * inverse};
 sample.valid = true;
 return sample;
}

bool test_arena_collision_query(
 const arut::core::Vec3& feet, float radius, float height, void* context) noexcept {
 const auto* state = static_cast<const RuntimeState*>(context);
 if (!state || !state->test_arena_enabled) return false;
 return arut::core::character_capsule_overlaps_obstacles(
  feet, radius, height,
  std::span<const arut::core::CharacterAabbObstacle>{
   state->arena_obstacles.data(), state->arena_obstacles.size()});
}

void build_test_arena_geometry(RuntimeState& state) noexcept {
 for (std::size_t i = 0; i < state.arena_obstacles.size(); ++i) {
  const auto& obstacle = state.arena_obstacles[i];
  auto model = arut::rendering::Mat4::identity();
  model.value[0] = obstacle.maximum.x - obstacle.minimum.x;
  model.value[5] = obstacle.maximum.y - obstacle.minimum.y;
  model.value[10] = obstacle.maximum.z - obstacle.minimum.z;
  model.value[12] = (obstacle.minimum.x + obstacle.maximum.x) * 0.5F;
  model.value[13] = (obstacle.minimum.y + obstacle.maximum.y) * 0.5F;
  model.value[14] = (obstacle.minimum.z + obstacle.maximum.z) * 0.5F;
  state.arena_render_instances[i] = model;
 }
}

std::uint64_t cell_lod_id(arut::core::WorldCell cell) noexcept {
 return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(cell.x)) << 32U) |
        static_cast<std::uint32_t>(cell.z);
}
arut::core::TerrainLod select_cell_lod(RuntimeState& state,
 arut::core::WorldCell cell, float camera_x, float camera_z) {
 const float size = state.world_partition.cell_size();
 const float center_x = (static_cast<float>(cell.x) + 0.5F) * size;
 const float center_z = (static_cast<float>(cell.z) + 0.5F) * size;
 const float dx = center_x - camera_x, dz = center_z - camera_z;
 return state.cell_lod_system.select(cell_lod_id(cell), std::sqrt(dx * dx + dz * dz));
}
std::uint32_t cell_lod_resolution(arut::core::TerrainLod lod) noexcept {
 switch (lod) {
  case arut::core::TerrainLod::High: return 32U;
  case arut::core::TerrainLod::Medium: return 16U;
  case arut::core::TerrainLod::Low: return 8U;
  case arut::core::TerrainLod::Impostor: return 4U;
 }
 return 4U;
}
bool build_cell_content(RuntimeState& state, arut::core::AssetManager& assets,
 arut::core::WorldCell cell, float camera_x, float camera_z,
 arut::core::WorldCellContent& content) {
 const auto material = assets.load("textures/demo_base_color.png");
 if (!material) return false;
 content.material_asset = material.bytes;
 content.cell = cell;
 content.lod = select_cell_lod(state, cell, camera_x, camera_z);
 const float size = state.world_partition.cell_size();
 content.terrain = arut::core::TerrainSystem::build(size, cell_lod_resolution(content.lod));
 const float offset_x = (static_cast<float>(cell.x) + 0.5F) * size;
 const float offset_z = (static_cast<float>(cell.z) + 0.5F) * size;
 constexpr float epsilon = 0.1F;
 for (auto& vertex : content.terrain.vertices) {
  vertex.x += offset_x;
  vertex.z += offset_z;
  vertex.y = arut::core::TerrainSystem::height(vertex.x, vertex.z);
  const float dx = (arut::core::TerrainSystem::height(vertex.x + epsilon, vertex.z) -
                    arut::core::TerrainSystem::height(vertex.x - epsilon, vertex.z)) / (2.0F * epsilon);
  const float dz = (arut::core::TerrainSystem::height(vertex.x, vertex.z + epsilon) -
                    arut::core::TerrainSystem::height(vertex.x, vertex.z - epsilon)) / (2.0F * epsilon);
  const float inverse = 1.0F / std::sqrt(dx * dx + 1.0F + dz * dz);
  vertex.nx = -dx * inverse;
  vertex.ny = inverse;
  vertex.nz = -dz * inverse;
 }
 return content.terrain.valid();
}
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

// Android thermal sysfs is device/vendor dependent. Read every exposed zone,
// preserve the hottest reported sensor, and explicitly report unavailable data.
bool read_thermal_peak(float& temperature_c, char* sensor, std::size_t sensor_size) noexcept {
 temperature_c = 0.0F;
 if (sensor && sensor_size) sensor[0] = '\0';
 bool found = false;
 for (unsigned int zone = 0; zone < 64U; ++zone) {
  char temp_path[96]{}, type_path[96]{};
  std::snprintf(temp_path, sizeof(temp_path),
   "/sys/class/thermal/thermal_zone%u/temp", zone);
  std::snprintf(type_path, sizeof(type_path),
   "/sys/class/thermal/thermal_zone%u/type", zone);
  std::ifstream temperature_file(temp_path);
  long raw = 0;
  if (!(temperature_file >> raw)) continue;
  std::ifstream type_file(type_path);
  std::string type;
  std::getline(type_file, type);
  float value = static_cast<float>(raw);
  if (std::abs(value) >= 1000.0F) value /= 1000.0F;
  if (!std::isfinite(value) || value < -40.0F || value > 150.0F) continue;
  if (!found || value > temperature_c) {
   temperature_c = value;
   if (sensor && sensor_size)
    std::snprintf(sensor, sensor_size, "%s", type.empty() ? "unknown" : type.c_str());
   found = true;
  }
 }
 return found;
}
void log_warn(const char* format, ...) {
 va_list args; va_start(args, format);
 write_log(ANDROID_LOG_WARN, "WARN", format, args);
 va_end(args);
}

bool load_cc0_humanoid_animation_library(
 AAssetManager* manager, const arut::core::HumanoidSkeleton& target_skeleton,
 std::vector<arut::animation::AnimationClip>& output) {
 if (!manager || !target_skeleton.valid()) return false;

 // The animation pack contains animation-only GLBs (no skins or meshes).
 // Use the bundled mannequin GLB as the source rig and bind channels by its
 // exact node names before retargeting onto the engine's gameplay skeleton.
 AAsset* rig_file = AAssetManager_open(
  manager, "Characters/hero.glb", AASSET_MODE_BUFFER);
 if (!rig_file) {
  log_warn("ANIMATION_PACK reference Characters/hero.glb unavailable");
  return false;
 }
 const off_t rig_length = AAsset_getLength(rig_file);
 if (rig_length <= 0 || rig_length > 32LL * 1024LL * 1024LL) {
  AAsset_close(rig_file);
  log_warn("ANIMATION_PACK reference rig size invalid");
  return false;
 }
 std::vector<std::uint8_t> rig_bytes;
 try { rig_bytes.resize(static_cast<std::size_t>(rig_length)); }
 catch (...) { AAsset_close(rig_file); return false; }
 std::size_t rig_read = 0U;
 while (rig_read < rig_bytes.size()) {
  const int amount = AAsset_read(
   rig_file, rig_bytes.data() + rig_read, rig_bytes.size() - rig_read);
  if (amount <= 0) break;
  rig_read += static_cast<std::size_t>(amount);
 }
 AAsset_close(rig_file);
 if (rig_read != rig_bytes.size()) {
  log_warn("ANIMATION_PACK reference rig read incomplete");
  return false;
 }
 arut::animation::SkeletalAsset source_asset{};
 arut::animation::GltfImportReport rig_report{};
 if (!arut::animation::import_glb(
      rig_bytes.data(), rig_bytes.size(), source_asset, rig_report)) {
  log_warn("ANIMATION_PACK reference rig import failed: %s",
   rig_report.message.c_str());
  return false;
 }
 const auto source_rig = arut::animation::make_retarget_rig(source_asset);
 const auto target_rig = arut::animation::make_retarget_rig(target_skeleton);
 arut::animation::RetargetMap retarget_map{};
 if (!arut::animation::build_retarget_map(source_rig, target_rig, retarget_map)) {
  log_warn("ANIMATION_PACK reference rig incompatible source=%zu target=%zu",
   source_rig.joint_count, target_rig.joint_count);
  return false;
 }
 log_info("ANIMATION_PACK reference_rig_joints=%zu mapped=%zu unmatched_source=%zu unmatched_target=%zu",
  source_rig.joint_count, retarget_map.pair_count,
  retarget_map.unmatched_source, retarget_map.unmatched_target);

 AAssetDir* directory = AAssetManager_openDir(
  manager, "Characters/AnimationPack/GLB");
 if (!directory) {
  log_warn("ANIMATION_PACK directory unavailable");
  return false;
 }
 std::vector<arut::animation::AnimationClip> candidate;
 std::size_t files_seen = 0U, clips_loaded = 0U, clips_rejected = 0U;
 while (const char* entry = AAssetDir_getNextFileName(directory)) {
  const std::string filename(entry);
  if (filename.size() < 4U ||
      (filename.compare(filename.size() - 4U, 4U, ".glb") != 0 &&
       filename.compare(filename.size() - 4U, 4U, ".GLB") != 0)) continue;
  ++files_seen;
  const std::string asset_path = "Characters/AnimationPack/GLB/" + filename;
  AAsset* file = AAssetManager_open(manager, asset_path.c_str(), AASSET_MODE_BUFFER);
  if (!file) { ++clips_rejected; continue; }
  const off_t length = AAsset_getLength(file);
  if (length <= 0 || length > 16LL * 1024LL * 1024LL) {
   AAsset_close(file); ++clips_rejected; continue;
  }
  std::vector<std::uint8_t> bytes;
  try { bytes.resize(static_cast<std::size_t>(length)); }
  catch (...) { AAsset_close(file); ++clips_rejected; continue; }
  std::size_t read = 0U;
  while (read < bytes.size()) {
   const int amount = AAsset_read(file, bytes.data() + read, bytes.size() - read);
   if (amount <= 0) break;
   read += static_cast<std::size_t>(amount);
  }
  AAsset_close(file);
  if (read != bytes.size()) { ++clips_rejected; continue; }

  std::vector<arut::animation::AnimationClip> source_clips;
  arut::animation::GltfImportReport report{};
  if (!arut::animation::import_glb_animation_clips(
       bytes.data(), bytes.size(), source_asset, source_clips, report)) {
   ++clips_rejected;
   log_warn("ANIMATION_PACK rejected=%s reason=%s",
    filename.c_str(), report.message.c_str());
   continue;
  }
  for (const auto& source_clip : source_clips) {
   arut::animation::AnimationClip converted{};
   if (!arut::animation::retarget_clip(
        source_clip, source_rig, target_rig, retarget_map, converted)) {
    ++clips_rejected;
    continue;
   }
   converted.name = filename.substr(0U, filename.size() - 4U);
   candidate.push_back(std::move(converted));
   ++clips_loaded;
  }
 }
 AAssetDir_close(directory);
 if (clips_loaded == 0U) {
  log_warn("ANIMATION_PACK no usable clips files=%zu rejected=%zu",
   files_seen, clips_rejected);
  return false;
 }
 output = std::move(candidate);
 log_info("ANIMATION_PACK loaded=%zu files=%zu rejected=%zu mapped_joints=%zu target_joints=%zu",
  clips_loaded, files_seen, clips_rejected, retarget_map.pair_count,
  target_rig.joint_count);
 return true;
}

bool configure_default_humanoid_clip_animator(RuntimeState& state) {
 using S = arut::animation::CharacterAnimationState;
 arut::animation::CharacterAnimationSet set{};
 const auto find_clip = [&](std::initializer_list<const char*> tokens)
     -> const arut::animation::AnimationClip* {
  for (const char* token : tokens) {
   for (const auto& clip : state.humanoid_animation_library) {
    std::string name = clip.name;
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) {
     return static_cast<char>(std::tolower(c));
    });
    if (name.find(token) != std::string::npos) return &clip;
   }
  }
  return nullptr;
 };
 const auto idle = find_clip({"idle_relaxed", "idle"});
 const auto walk = find_clip({"walk_forwards", "walk"});
 const auto run = find_clip({"run_sprint", "run"});
 const auto backward = find_clip({"walk_backwards", "walk_back"});
 const auto jump = find_clip({"jump_in_the_air", "jump"});
 const auto land = find_clip({"jump_landing", "landing"});
 const auto punch = find_clip({"punch_jab", "punch_cross", "punch_hook", "punch"});
 const auto heavy = find_clip({"sword_slash", "punch_hook", "punch_cross", "punch"});
 const auto block = find_clip({"block_with_the_sword", "block"});
 const auto dodge = find_clip({"dodge", "roll"});
 const auto hit = find_clip({"hit_knockback", "hit_to_the_chest", "hit_to_the_head", "hit"});
 const auto recover = find_clip({"move_get_up", "action_stand_up", "get_up"});
 set.set(S::Idle, idle);
 set.set(S::Walk, walk ? walk : idle);
 set.set(S::Run, run ? run : (walk ? walk : idle));
 set.set(S::StrafeLeft, walk ? walk : idle);
 set.set(S::StrafeRight, walk ? walk : idle);
 set.set(S::Backward, backward ? backward : (walk ? walk : idle));
 set.set(S::Jump, jump ? jump : idle);
 set.set(S::Fall, jump ? jump : idle);
 set.set(S::Land, land ? land : (jump ? jump : idle));
 set.set(S::AttackLight, punch ? punch : idle);
 set.set(S::AttackHeavy, heavy ? heavy : (punch ? punch : idle));
 set.set(S::AttackKick, heavy ? heavy : (punch ? punch : idle));
 set.set(S::Block, block ? block : idle);
 set.set(S::Dodge, dodge ? dodge : (walk ? walk : idle));
 set.set(S::HitReact, hit ? hit : idle);
 set.set(S::Recover, recover ? recover : idle);
 if (!idle) {
  log_warn("ANIMATION_PACK has no idle clip; default humanoid keeps its procedural animator");
  return false;
 }
 const bool configured = state.humanoid_clip_animator.configure(set) &&
                          state.humanoid_clip_animator.start();
 log_info("DEFAULT_HUMANOID animation_graph=%s library_clips=%zu idle=%s walk=%s run=%s punch=%s",
  configured ? "ready" : "failed", state.humanoid_animation_library.size(),
  idle ? idle->name.c_str() : "none", walk ? walk->name.c_str() : "fallback",
  run ? run->name.c_str() : "fallback", punch ? punch->name.c_str() : "fallback");
 return configured;
}

bool configure_imported_character_animator(RuntimeState& state) {
 using S = arut::animation::CharacterAnimationState;
 arut::animation::CharacterAnimationSet clips{};
 for (const auto& clip : state.imported_character_asset.animations) {
  std::string name = clip.name;
  name.erase(std::remove_if(name.begin(), name.end(), [](unsigned char ch) {
   return !std::isalnum(ch);
  }), name.end());
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) {
   return static_cast<char>(std::tolower(ch));
  });
  auto bind = [&](S state_id) {
   if (!clips.get(state_id)) clips.set(state_id, &clip);
  };
  if (name.find("idle") != std::string::npos || name.find("stand") != std::string::npos)
   bind(S::Idle);
  else if (name.find("strafeleft") != std::string::npos || name.find("leftstrafe") != std::string::npos)
   bind(S::StrafeLeft);
  else if (name.find("straferight") != std::string::npos || name.find("rightstrafe") != std::string::npos)
   bind(S::StrafeRight);
  else if (name.find("backward") != std::string::npos || name.find("backpedal") != std::string::npos)
   bind(S::Backward);
  else if (name.find("attackheavy") != std::string::npos || name.find("heavyattack") != std::string::npos ||
           name.find("heavy") != std::string::npos)
   bind(S::AttackHeavy);
  else if (name.find("attacklight") != std::string::npos || name.find("lightattack") != std::string::npos ||
           name.find("punch") != std::string::npos || name.find("jab") != std::string::npos)
   bind(S::AttackLight);
  else if (name.find("block") != std::string::npos || name.find("guard") != std::string::npos)
   bind(S::Block);
  else if (name.find("dodge") != std::string::npos || name.find("evade") != std::string::npos ||
           name.find("roll") != std::string::npos)
   bind(S::Dodge);
  else if (name.find("hitreact") != std::string::npos || name.find("damage") != std::string::npos ||
           name == "hit")
   bind(S::HitReact);
  else if (name.find("recover") != std::string::npos || name.find("getup") != std::string::npos)
   bind(S::Recover);
  else if (name.find("jump") != std::string::npos)
   bind(S::Jump);
  else if (name.find("fall") != std::string::npos || name.find("airborne") != std::string::npos)
   bind(S::Fall);
  else if (name.find("land") != std::string::npos || name.find("landing") != std::string::npos)
   bind(S::Land);
  else if (name.find("run") != std::string::npos || name.find("sprint") != std::string::npos)
   bind(S::Run);
  else if (name.find("walk") != std::string::npos)
   bind(S::Walk);
 }
 // Preserve every authored GLB clip that has a recognized semantic name.
 // The bundled default pack currently exposes only one generic clip, so
 // author a complete original fallback set against this exact 66-joint rest rig.
 if (!clips.get(S::Idle) && !state.imported_character_asset.animations.empty())
  clips.set(S::Idle, &state.imported_character_asset.animations.front());

 std::vector<std::string> joint_names;
 joint_names.reserve(state.imported_character_asset.joints.size());
 for (const auto& joint : state.imported_character_asset.joints)
  joint_names.push_back(joint.name);
 auto procedural_clips = arut::animation::make_procedural_humanoid_clips(
  state.imported_character_rest_pose, joint_names);
 std::size_t generated_count = 0U;
 for (const auto& clip : procedural_clips) {
  const auto name = clip.name;
  S target = S::Count;
  if (name == "ARUT_Idle") target = S::Idle;
  else if (name == "ARUT_Walk") target = S::Walk;
  else if (name == "ARUT_Run") target = S::Run;
  else if (name == "ARUT_StrafeLeft") target = S::StrafeLeft;
  else if (name == "ARUT_StrafeRight") target = S::StrafeRight;
  else if (name == "ARUT_Backward") target = S::Backward;
  else if (name == "ARUT_Jump") target = S::Jump;
  else if (name == "ARUT_Fall") target = S::Fall;
  else if (name == "ARUT_Land") target = S::Land;
  else if (name == "ARUT_AttackLight") target = S::AttackLight;
  else if (name == "ARUT_AttackHeavy") target = S::AttackHeavy;
  else if (name == "ARUT_Block") target = S::Block;
  else if (name == "ARUT_Dodge") target = S::Dodge;
  else if (name == "ARUT_HitReact") target = S::HitReact;
  else if (name == "ARUT_Recover") target = S::Recover;
  if (target != S::Count && !clips.get(target)) {
   clips.set(target, &clip);
   ++generated_count;
  }
 }
 const bool configured = state.imported_character_animator.configure(clips) &&
                         state.imported_character_animator.start();
 log_info("IMPORTED_CHARACTER animation_library authored=%zu imported=%zu generated_fallback=%zu ready=%s",
  clips.clips.size(), state.imported_character_asset.animations.size(),
  generated_count, configured ? "true" : "false");
 return configured;
}

bool initialize_profile_renderer(RuntimeState& state, android_app* app) {
 if (!app || !app->window || !state.has_window || !state.render_scene_ready) return true;
 std::span<const arut::core::CubeRenderInstance> stress_instances{};
 if (arut::core::test_lab_includes_stress(state.selected_profile)) {
  const auto& instances = state.cube_render_extraction_system.instances();
  stress_instances = std::span<const arut::core::CubeRenderInstance>{instances.data(), instances.size()};
 }
 const bool include_mannequin =
  arut::core::test_lab_includes_humanoid(state.selected_profile);
 std::span<const arut::rendering::Mat4> arena_instances{};
 if (arut::core::test_lab_includes_humanoid(state.selected_profile)) {
  arena_instances = {state.arena_render_instances.data(), state.arena_render_instances.size()};
 }
 if (!state.renderer.initialize(app->window, app->activity->assetManager,
      stress_instances,
      std::span<const arut::core::TerrainMesh>{state.terrain_cells.data(), state.terrain_cells.size()},
      include_mannequin, arena_instances)) {
  log_warn("TEST_LAB renderer initialization failed profile=%.*s",
   static_cast<int>(arut::core::test_lab_profile_name(state.selected_profile).size()),
   arut::core::test_lab_profile_name(state.selected_profile).data());
  return false;
 }
 log_info("TEST_LAB renderer_ready=true profile=%.*s stress_instances=%zu mannequin=%s",
  static_cast<int>(arut::core::test_lab_profile_name(state.selected_profile).size()),
  arut::core::test_lab_profile_name(state.selected_profile).data(),
  stress_instances.size(), include_mannequin ? "true" : "false");
 if (include_mannequin && app->activity && app->activity->assetManager) {
  state.humanoid_clip_animator_ready = load_cc0_humanoid_animation_library(
   app->activity->assetManager, state.humanoid_skeleton,
   state.humanoid_animation_library) && configure_default_humanoid_clip_animator(state);
 }

 state.imported_character_active = false;
 state.imported_character_animator_ready = false;
 const bool request_imported_character =
  g_imported_character_enabled.load(std::memory_order_acquire);
 if (include_mannequin && request_imported_character &&
     app->activity && app->activity->assetManager) {
  AAsset* probe = AAssetManager_open(app->activity->assetManager,
                                     "Characters/hero.glb", AASSET_MODE_STREAMING);
  if (probe) {
   AAsset_close(probe);
   const auto identity = arut::rendering::Mat4::identity();
   if (state.renderer.load_imported_character_glb(
        "Characters/hero.glb", state.imported_character_asset, identity)) {
    log_info("IMPORTED_CHARACTER animation_source=hero_glb clips=%zu; default mannequin CC0 library is retargeted independently",
     state.imported_character_asset.animations.size());
    // Imported GLBs are not guaranteed to use the engine mannequin's units,
    // origin, or mesh centering. Fit their rest-pose mesh bounds to the same
    // 1.8 m gameplay height and put the feet on the character controller's
    // ground plane; this also makes the third-person camera framing consistent.
    float bounds_min_x = std::numeric_limits<float>::max();
    float bounds_min_y = std::numeric_limits<float>::max();
    float bounds_min_z = std::numeric_limits<float>::max();
    float bounds_max_x = std::numeric_limits<float>::lowest();
    float bounds_max_y = std::numeric_limits<float>::lowest();
    float bounds_max_z = std::numeric_limits<float>::lowest();
    for (const auto& mesh : state.imported_character_asset.meshes) {
     for (const auto& vertex : mesh.vertices) {
      const auto& p = vertex.position;
      if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) continue;
      bounds_min_x = std::min(bounds_min_x, p.x);
      bounds_min_y = std::min(bounds_min_y, p.y);
      bounds_min_z = std::min(bounds_min_z, p.z);
      bounds_max_x = std::max(bounds_max_x, p.x);
      bounds_max_y = std::max(bounds_max_y, p.y);
      bounds_max_z = std::max(bounds_max_z, p.z);
     }
    }
    const float source_height = bounds_max_y - bounds_min_y;
    if (std::isfinite(source_height) && source_height > 1.0e-4F &&
        bounds_min_x <= bounds_max_x && bounds_min_z <= bounds_max_z) {
     state.imported_character_model_scale = 1.8F / source_height;
     state.imported_character_bounds_min_x = bounds_min_x;
     state.imported_character_bounds_min_y = bounds_min_y;
     state.imported_character_bounds_min_z = bounds_min_z;
     state.imported_character_bounds_center_x = (bounds_min_x + bounds_max_x) * 0.5F;
     state.imported_character_bounds_center_z = (bounds_min_z + bounds_max_z) * 0.5F;
     log_info("IMPORTED_CHARACTER framing source_height=%.4f model_scale=%.5f local_center=%.4f,%.4f,%.4f target_height=1.8",
      source_height, state.imported_character_model_scale,
      state.imported_character_bounds_center_x, bounds_min_y,
      state.imported_character_bounds_center_z);
    } else {
     state.imported_character_model_scale = 1.0F;
     log_warn("IMPORTED_CHARACTER mesh bounds invalid; retaining source scale and origin");
    }
    state.imported_character_rest_pose = {};
    state.imported_character_rest_pose.joint_count =
     state.imported_character_asset.joint_count;
    for (std::size_t joint = 0; joint < state.imported_character_asset.joint_count; ++joint)
     state.imported_character_rest_pose.joints[joint] =
      state.imported_character_asset.joints[joint].rest_local;
    state.imported_character_active = true;
    state.imported_character_root_joint = arut::animation::kMaxPoseJoints;
    for (std::size_t joint = 0; joint < state.imported_character_asset.joint_count; ++joint) {
     if (state.imported_character_asset.joints[joint].parent < 0) {
      state.imported_character_root_joint = joint;
      break;
     }
    }
    state.imported_character_animator_ready =
     configure_imported_character_animator(state);
    log_info("IMPORTED_CHARACTER runtime_active=true joints=%zu clips=%zu graph_ready=%s",
     state.imported_character_asset.joint_count,
     state.imported_character_asset.animations.size(),
     state.imported_character_animator_ready ? "true" : "false");
   } else {
    log_warn("IMPORTED_CHARACTER packaged GLB was found but could not be installed");
   }
  } else {
   log_info("IMPORTED_CHARACTER optional asset absent; using procedural robot mannequin");
  }
 } else if (include_mannequin) {
  log_info("CHARACTER_SOURCE selected=procedural_robot_mannequin imported_glb_enabled=false");
 }
 return true;
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
  case APP_CMD_PAUSE:
   state->resumed = false;
   state->character_input_router.clear();
   state->mouse_look_active = false;
   state->gamepad_move_x = state->gamepad_move_y = 0.0F;
   state->combat_attack_light_pressed = false;
   state->combat_attack_heavy_pressed = false;
   state->combat_attack_kick_pressed = false;
   state->combat_block_held = false;
   state->combat_dodge_pressed = false;
   state->combat_hit_reaction_pressed = false;
   state->combat_recover_pressed = false;
   g_touch_combat_pending.store(0U, std::memory_order_release);
   g_touch_combat_block_held.store(false, std::memory_order_release);
   state->renderer.clear_input();
   log_info("Lifecycle PAUSE");
   if (export_runtime_log(app)) __android_log_print(ANDROID_LOG_INFO, kLogTag, "Runtime log exported to Downloads/ARUT-HARDT/ARUT-HARDT-runtime.log");
   else __android_log_print(ANDROID_LOG_WARN, kLogTag, "Runtime log export failed; private app log retained.");
   break;
  case APP_CMD_STOP: log_info("Lifecycle STOP"); break;
  case APP_CMD_INIT_WINDOW:
   state->has_window = app->window != nullptr;
   if (state->has_window) {
    log_info("Window created: %dx%d", ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
    if (!state->render_scene_ready) {
     log_info("TEST_LAB renderer deferred until a valid profile scene is ready.");
    } else {
     (void)initialize_profile_renderer(*state, app);
    }
   }
   break;
  case APP_CMD_TERM_WINDOW:
   state->has_window = false;
   state->renderer.shutdown();
   log_info("Window destroyed; Vulkan surface and swapchain released.");
   break;
  case APP_CMD_GAINED_FOCUS: state->focused = true; log_info("Focus gained"); break;
  case APP_CMD_LOST_FOCUS:
   state->focused = false;
   state->character_input_router.clear();
   state->mouse_look_active = false;
   state->gamepad_move_x = state->gamepad_move_y = 0.0F;
   state->combat_attack_light_pressed = false;
   state->combat_attack_heavy_pressed = false;
   state->combat_attack_kick_pressed = false;
   state->combat_block_held = false;
   state->combat_dodge_pressed = false;
   state->combat_hit_reaction_pressed = false;
   state->combat_recover_pressed = false;
   g_touch_combat_pending.store(0U, std::memory_order_release);
   g_touch_combat_block_held.store(false, std::memory_order_release);
   state->renderer.set_gamepad_look_input(0.0F, 0.0F);
   log_info("Focus lost"); break;
  case APP_CMD_CONFIG_CHANGED:
   if (app->window) log_info("Configuration changed: %dx%d", ANativeWindow_getWidth(app->window), ANativeWindow_getHeight(app->window));
   break;
  case APP_CMD_LOW_MEMORY: log_warn("Android low-memory callback received."); break;
  case APP_CMD_DESTROY: log_info("Lifecycle DESTROY pid=%d destroyRequested=%d", static_cast<int>(getpid()), app->destroyRequested); break;
  default: break;
 }
}
int32_t handle_input(android_app* app, AInputEvent* event) {
 auto* state = static_cast<RuntimeState*>(app->userData);
 if (!state || !event) return 0;
 ++state->input_events;
 if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
  const auto raw_action = AMotionEvent_getAction(event);
  const auto action = raw_action & AMOTION_EVENT_ACTION_MASK;
  const std::int32_t source = AInputEvent_getSource(event);
  if ((source & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) {
   const auto deadzone = [](float value) noexcept {
    if (!std::isfinite(value) || std::abs(value) <= 0.15F) return 0.0F;
    const float scaled = (std::abs(value) - 0.15F) / 0.85F;
    return std::copysign(std::clamp(scaled, 0.0F, 1.0F), value);
   };
   state->gamepad_move_x = deadzone(
    AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0U));
   state->gamepad_move_y = -deadzone(
    AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0U));
   const float look_x = deadzone(
    AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0U));
   const float look_y = deadzone(
    AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0U));
   state->renderer.set_gamepad_movement_input(
    state->gamepad_move_x, state->gamepad_move_y);
   state->renderer.set_gamepad_look_input(look_x, look_y);
   return 1;
  }
  if ((source & AINPUT_SOURCE_MOUSE) == AINPUT_SOURCE_MOUSE) {
   if (!app->window) return 0;
   const float width = static_cast<float>(ANativeWindow_getWidth(app->window));
   const float height = static_cast<float>(ANativeWindow_getHeight(app->window));
   const float x = AMotionEvent_getX(event, 0U);
   const float y = AMotionEvent_getY(event, 0U);
   const auto buttons = AMotionEvent_getButtonState(event);
   if (action == AMOTION_EVENT_ACTION_BUTTON_PRESS &&
       (buttons & AMOTION_EVENT_BUTTON_SECONDARY) != 0U &&
       x >= width * 0.48F && y < height * 0.68F) {
    state->mouse_look_active = true;
    state->renderer.set_touch_input(0, 0x7ffffffe, x, y, width, height);
    return 1;
   }
   if (state->mouse_look_active &&
       (action == AMOTION_EVENT_ACTION_MOVE ||
        action == AMOTION_EVENT_ACTION_HOVER_MOVE)) {
    state->renderer.set_touch_input(1, 0x7ffffffe, x, y, width, height);
    return 1;
   }
   if (state->mouse_look_active &&
       (action == AMOTION_EVENT_ACTION_BUTTON_RELEASE ||
        action == AMOTION_EVENT_ACTION_CANCEL)) {
    state->renderer.set_touch_input(2, 0x7ffffffe, x, y, width, height);
    state->mouse_look_active = false;
    return 1;
   }
   return 0;
  }
  const auto pointer_index = static_cast<std::size_t>((raw_action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
  const auto pointer_count = AMotionEvent_getPointerCount(event);
  const float width = static_cast<float>(ANativeWindow_getWidth(app->window));
  const float height = static_cast<float>(ANativeWindow_getHeight(app->window));
  if (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_POINTER_DOWN) {
   const auto id = AMotionEvent_getPointerId(event, pointer_index);
   state->renderer.set_touch_input(0, id, AMotionEvent_getX(event, pointer_index),
                                   AMotionEvent_getY(event, pointer_index), width, height);
  } else if (action == AMOTION_EVENT_ACTION_MOVE) {
   for (std::size_t p = 0; p < pointer_count; ++p) {
    state->renderer.set_touch_input(1, AMotionEvent_getPointerId(event, p),
                                    AMotionEvent_getX(event, p), AMotionEvent_getY(event, p), width, height);
   }
  } else if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_POINTER_UP ||
             action == AMOTION_EVENT_ACTION_CANCEL) {
   const auto id = AMotionEvent_getPointerId(event, pointer_index);
   state->renderer.set_touch_input(2, id, AMotionEvent_getX(event, pointer_index),
                                   AMotionEvent_getY(event, pointer_index), width, height);
  }
  return 1;
 }
 if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
  const auto key_action = AKeyEvent_getAction(event);
  if (key_action != AKEY_EVENT_ACTION_DOWN && key_action != AKEY_EVENT_ACTION_UP)
   return 0;
  const bool pressed = key_action == AKEY_EVENT_ACTION_DOWN;
  const auto key_code = AKeyEvent_getKeyCode(event);
  using Key = arut::core::CharacterInputKey;
  bool handled = true;
  switch (key_code) {
   case AKEYCODE_W: case AKEYCODE_DPAD_UP:
    state->character_input_router.set_key(Key::MoveForward, pressed); break;
   case AKEYCODE_S: case AKEYCODE_DPAD_DOWN:
    state->character_input_router.set_key(Key::MoveBackward, pressed); break;
   case AKEYCODE_A: case AKEYCODE_DPAD_LEFT:
    state->character_input_router.set_key(Key::MoveLeft, pressed); break;
   case AKEYCODE_D: case AKEYCODE_DPAD_RIGHT:
    state->character_input_router.set_key(Key::MoveRight, pressed); break;
   case AKEYCODE_SPACE: case AKEYCODE_BUTTON_A:
    state->character_input_router.set_key(Key::Jump, pressed); break;
   case AKEYCODE_J: case AKEYCODE_BUTTON_B:
    if (pressed) state->combat_attack_light_pressed = true; break;
   case AKEYCODE_K: case AKEYCODE_BUTTON_X:
    if (pressed) state->combat_attack_heavy_pressed = true; break;
   case AKEYCODE_L: case AKEYCODE_BUTTON_Y:
    state->combat_block_held = pressed; break;
   case AKEYCODE_I: case AKEYCODE_BUTTON_R1:
    if (pressed) state->combat_dodge_pressed = true; break;
   case AKEYCODE_M: case AKEYCODE_BUTTON_L1:
    if (pressed) state->combat_attack_kick_pressed = true; break;
   // Diagnostic actions exercise these poses until gameplay supplies hit and knockdown events.
   case AKEYCODE_H: case AKEYCODE_BUTTON_L2:
    if (pressed) state->combat_hit_reaction_pressed = true; break;
   case AKEYCODE_R: case AKEYCODE_BUTTON_R2:
    if (pressed) state->combat_recover_pressed = true; break;
   default: handled = false; break;
  }
  if (handled) {
   log_info("CHARACTER_KEY code=%d pressed=%s",
    key_code, pressed ? "true" : "false");
   return 1;
  }
  return 0;
 }
 return 0;
}
 }
extern "C" JNIEXPORT void JNICALL
Java_com_arut_engine_TestLabActivity_nativeSelectProfile(
    JNIEnv*, jclass, jint profile_id) noexcept {
 __android_log_print(ANDROID_LOG_INFO, kLogTag,
  "TEST_LAB JNI profile_selection_callback profile_id=%d", profile_id);
 if (arut::core::valid_test_lab_profile(profile_id)) {
  g_requested_test_profile.store(profile_id, std::memory_order_release);
  __android_log_print(ANDROID_LOG_INFO, kLogTag,
   "TEST_LAB JNI profile_selection_accepted profile_id=%d", profile_id);
 } else {
  __android_log_print(ANDROID_LOG_ERROR, kLogTag,
   "TEST_LAB JNI invalid_profile_id=%d", profile_id);
 }
}
extern "C" JNIEXPORT void JNICALL
Java_com_arut_engine_TestLabActivity_nativeSetImportedCharacterEnabled(
    JNIEnv*, jclass, jboolean enabled) noexcept {
 g_imported_character_enabled.store(enabled == JNI_TRUE, std::memory_order_release);
 __android_log_print(ANDROID_LOG_INFO, kLogTag,
  "CHARACTER_SOURCE selection=%s",
  enabled == JNI_TRUE ? "optional_imported_glb" : "procedural_robot_mannequin");
}
extern "C" JNIEXPORT void JNICALL
Java_com_arut_engine_EngineNativeActivity_nativeSetCombatAction(
    JNIEnv*, jclass, jint action, jboolean pressed) noexcept {
 if (action < 0 || action > 7) return;
 if (action == 3) {
  g_touch_combat_block_held.store(pressed == JNI_TRUE, std::memory_order_release);
 } else if (pressed == JNI_TRUE) {
  g_touch_combat_pending.fetch_or(1U << static_cast<std::uint32_t>(action),
                                  std::memory_order_release);
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
 log_info("%.*s %u.%u.%u starting (NativeActivity) pid=%d.",
  static_cast<int>(arut::core::name().size()), arut::core::name().data(), v.major, v.minor, v.patch,
  static_cast<int>(getpid()));
 log_info("Rust systems ABI version: 0x%06x", arut_tasks_abi_version());
 log_info("TEST_LAB waiting_for_profile_selection=true");
 while (g_requested_test_profile.load(std::memory_order_acquire) < 0 &&
        app->destroyRequested == 0) {
  android_poll_source* source = nullptr;
  const int ident = ALooper_pollOnce(100, nullptr, nullptr,
                                     reinterpret_cast<void**>(&source));
  if (ident >= 0 && source) source->process(app, source);
  if (ident == ALOOPER_POLL_ERROR) log_warn("Android looper error while waiting for Test Laboratory selection.");
 }
 if (app->destroyRequested != 0) {
  log_info("Runtime destroyed before a Test Laboratory profile was selected.");
  close_file_log();
  return;
 }
 const std::int32_t selected_id =
  g_requested_test_profile.load(std::memory_order_acquire);
 if (!arut::core::valid_test_lab_profile(selected_id)) {
  log_warn("Test Laboratory returned an invalid profile id=%d.", selected_id);
  close_file_log();
  return;
 }
 state.selected_profile = static_cast<arut::core::TestLabProfile>(selected_id);
 if (arut::core::test_lab_includes_humanoid(state.selected_profile)) {
  state.test_arena_enabled = true;
  build_test_arena_geometry(state);
  log_info("TEST_ARENA_COLLISION enabled=true obstacles=%zu",
   state.arena_obstacles.size());
 }
 if (arut::core::test_lab_includes_humanoid(state.selected_profile)) {
  const float spawn_z =
   state.selected_profile == arut::core::TestLabProfile::Combined ? -5.0F : 0.0F;
  const auto spawn_ground = terrain_ground_query(0.0F, spawn_z, nullptr);
  const float spawn_y = spawn_ground.valid ? spawn_ground.height : 0.0F;
  state.character_controller.reset({0.0F, spawn_y, spawn_z});
  log_info("CHARACTER_SPAWN position=%.3f,%.3f,%.3f grounded_on_terrain=%s",
   0.0F, spawn_y, spawn_z, spawn_ground.valid ? "true" : "false");
 }
 log_info("TEST_LAB selected=%.*s profile_id=%d",
  static_cast<int>(arut::core::test_lab_profile_name(state.selected_profile).size()),
  arut::core::test_lab_profile_name(state.selected_profile).data(), selected_id);
 auto asset_source = std::make_shared<arut::platform::android::AndroidAssetSource>(
  app->activity ? app->activity->assetManager : nullptr);
 arut::core::AssetManager asset_manager(asset_source);
 const auto default_environment = asset_manager.load("environments/default.hdr");
 const auto asset_stats = asset_manager.stats();
 log_info("ASSET_MANAGER smoke_test=%s path=environments/default.hdr bytes=%zu cache_entries=%zu",
  default_environment ? "passed" : "failed",
  default_environment ? default_environment.bytes->size() : 0U, asset_stats.entries);
 try {
  const auto streaming = state.world_streamer.update(
   state.world_partition, 0.0F, 0.0F, 2U,
   [&state, &asset_manager](arut::core::WorldCell cell, arut::core::WorldCellContent& content) {
    return build_cell_content(state, asset_manager, cell, 0.0F, 0.0F, content);
   });
  state.terrain_cells = state.world_streamer.collect_terrain_meshes(state.world_partition);
  std::size_t terrain_vertices = 0U, terrain_indices = 0U;
  for (const auto& cell_mesh : state.terrain_cells) {
   terrain_vertices += cell_mesh.vertices.size();
   terrain_indices += cell_mesh.indices.size();
  }
  log_info("TERRAIN_CELLS generated=true cells=%zu vertices=%zu indices=%zu",
   state.terrain_cells.size(), terrain_vertices, terrain_indices);
  std::size_t cells_with_material = 0U;
  for (const auto cell : state.world_partition.active_cells()) {
   const auto* payload = state.world_streamer.find(cell);
   if (payload && payload->material_asset) ++cells_with_material;
  }
  const auto cache = asset_manager.stats();
  log_info("WORLD_STREAMING active_cells=%zu resident_cells=%zu loaded=%zu unloaded=%zu failed=%zu cell_size=%.1f",
   state.world_partition.active_count(), streaming.resident, streaming.loaded,
   streaming.unloaded, streaming.failed, state.world_partition.cell_size());
  log_info("WORLD_STREAMING_ASSETS cells_with_material=%zu cache_entries=%zu cache_hits=%" PRIu64 " cache_misses=%" PRIu64 " cached_bytes=%zu",
   cells_with_material, cache.entries, cache.hits, cache.misses, cache.bytes);
 } catch (const std::exception& error) {
  log_warn("Phase 3 world-system initialization failed: %s", error.what());
 }
 if (g_log_file) log_info("Persistent runtime log: %s", g_log_path);
 else log_warn("Persistent runtime log unavailable; logcat remains active.");
 if (arut::core::test_lab_includes_stress(state.selected_profile)) {
  try {
   state.cube_spawn_system.update(state.ecs_world);
   state.scene_graph_system.update(state.ecs_world);
   state.cube_render_extraction_system.update(state.ecs_world, &state.scene_graph_system);
   log_info("SCENE_GRAPH synchronized=true bound_entities=%zu nodes=%u",
    state.scene_graph_system.bound_entity_count(), state.scene_graph_system.graph().node_count());
   state.ecs_scene_ready =
    state.ecs_world.entity_count() == arut::core::CubeSpawnSystem::target_count &&
    state.cube_spawn_system.spawned_count() == arut::core::CubeSpawnSystem::target_count &&
    state.cube_render_extraction_system.extracted_count() == arut::core::CubeSpawnSystem::target_count;
   log_info("ECS_STRESS_SCENE ready=%s entities=%u spawned=%zu render_instances=%zu source=CubeSpawnSystem",
    state.ecs_scene_ready ? "true" : "false", state.ecs_world.entity_count(),
    state.cube_spawn_system.spawned_count(),
    state.cube_render_extraction_system.instances().size());
  } catch (const std::exception& error) {
   state.ecs_scene_ready = false;
   log_warn("ECS 5,000-cube scene creation failed: %s", error.what());
  }
 } else {
  state.ecs_scene_ready = false;
  log_info("ECS_STRESS_SCENE enabled=false reason=humanoid_only_profile");
 }
 state.render_scene_ready =
  !arut::core::test_lab_includes_stress(state.selected_profile) || state.ecs_scene_ready;
 if (arut::core::test_lab_includes_humanoid(state.selected_profile)) {
  log_info("HUMANOID_SCENE selected=true rig_bones=%zu mode=%s",
   arut::core::kHumanoidBoneCount,
   arut::core::test_lab_includes_stress(state.selected_profile) ? "combined" : "isolated");
 }
 if (state.has_window && state.render_scene_ready) {
  (void)initialize_profile_renderer(state, app);
 }
 std::size_t lod_high = 0U, lod_medium = 0U, lod_low = 0U, lod_impostor = 0U;
 for (std::size_t i = 0; i < state.cube_render_extraction_system.instances().size(); ++i) {
  const auto& instance = state.cube_render_extraction_system.instances()[i];
  const float x = instance.bounds[0], y = instance.bounds[1], z = instance.bounds[2];
  const float distance = std::sqrt(x*x + y*y + z*z);
  switch (state.lod_system.select(static_cast<std::uint64_t>(i), distance)) {
   case arut::core::TerrainLod::High: ++lod_high; break;
   case arut::core::TerrainLod::Medium: ++lod_medium; break;
   case arut::core::TerrainLod::Low: ++lod_low; break;
   case arut::core::TerrainLod::Impostor: ++lod_impostor; break;
  }
 }
 log_info("LOD tracked=%zu high=%zu medium=%zu low=%zu impostor=%zu",
  state.lod_system.tracked_count(), lod_high, lod_medium, lod_low, lod_impostor);
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
   if (!arut::core::test_lab_includes_humanoid(state.selected_profile)) {
    state.character_input_router.set_analog(0.0F, 0.0F);
    const auto keyboard = state.character_input_router.sample();
    state.renderer.set_gamepad_movement_input(
     std::clamp(keyboard.move_x + state.gamepad_move_x, -1.0F, 1.0F),
     std::clamp(keyboard.move_y + state.gamepad_move_y, -1.0F, 1.0F));
   }
   if (arut::core::test_lab_includes_humanoid(state.selected_profile)) {
    const float frame_delta = std::chrono::duration<float>(
     std::chrono::steady_clock::now() - frame_start).count();
    const auto movement = state.renderer.movement_input();
    state.character_input_router.set_analog(
     std::clamp(movement[0] + state.gamepad_move_x, -1.0F, 1.0F),
     std::clamp(movement[1] + state.gamepad_move_y, -1.0F, 1.0F));
    arut::core::CharacterInput character_input =
     state.character_input_router.sample();
    const std::uint32_t touch_combat_actions =
     g_touch_combat_pending.exchange(0U, std::memory_order_acq_rel);
    character_input.jump_pressed = character_input.jump_pressed ||
     state.renderer.consume_jump_pressed() || (touch_combat_actions & (1U << 7U)) != 0U;
    constexpr float half_turn = 1.570796327F;
    state.character_controller.set_facing_yaw(state.renderer.camera_yaw() + half_turn);
    state.character_fixed_steps += state.character_controller.update(
     character_input, frame_delta, terrain_ground_query, nullptr,
     test_arena_collision_query, &state);

    const auto& character = state.character_controller.state();
    const float horizontal_speed = std::sqrt(
     character.velocity.x * character.velocity.x +
     character.velocity.z * character.velocity.z);
    arut::rendering::HumanoidAnimationInput animation_input{};
    animation_input.local_move_x = character_input.move_x;
    animation_input.local_move_y = character_input.move_y;
    animation_input.horizontal_speed = horizontal_speed;
    animation_input.vertical_speed = character.velocity.y;
    animation_input.grounded = character.grounded;
    animation_input.attack_light_pressed = state.combat_attack_light_pressed ||
     (touch_combat_actions & (1U << 0U)) != 0U;
    animation_input.attack_heavy_pressed = state.combat_attack_heavy_pressed ||
     (touch_combat_actions & (1U << 1U)) != 0U;
    animation_input.attack_kick_pressed = state.combat_attack_kick_pressed ||
     (touch_combat_actions & (1U << 2U)) != 0U;
    animation_input.block_held = state.combat_block_held ||
     g_touch_combat_block_held.load(std::memory_order_acquire);
    animation_input.dodge_pressed = state.combat_dodge_pressed ||
     (touch_combat_actions & (1U << 4U)) != 0U;
    animation_input.hit_reaction = state.combat_hit_reaction_pressed ||
     (touch_combat_actions & (1U << 5U)) != 0U;
    animation_input.recover_pressed = state.combat_recover_pressed ||
     (touch_combat_actions & (1U << 6U)) != 0U;
    state.combat_attack_light_pressed = false;
    state.combat_attack_heavy_pressed = false;
    state.combat_attack_kick_pressed = false;
    state.combat_dodge_pressed = false;
    state.combat_hit_reaction_pressed = false;
    state.combat_recover_pressed = false;
    auto pose = state.humanoid_animator.update(animation_input, frame_delta);
    const auto& animation_snapshot = state.humanoid_animator.snapshot();
    if (state.humanoid_clip_animator_ready) {
     arut::animation::CharacterAnimationInput clip_input{};
     clip_input.move_x = animation_input.local_move_x;
     clip_input.move_y = animation_input.local_move_y;
     clip_input.horizontal_speed = animation_input.horizontal_speed;
     clip_input.vertical_speed = animation_input.vertical_speed;
     clip_input.grounded = animation_input.grounded;
     clip_input.sprinting = animation_input.horizontal_speed >= 3.2F;
     clip_input.attack_light_pressed = animation_input.attack_light_pressed;
     clip_input.attack_heavy_pressed = animation_input.attack_heavy_pressed;
     clip_input.attack_kick_pressed = animation_input.attack_kick_pressed;
     clip_input.block_held = animation_input.block_held;
     clip_input.dodge_pressed = animation_input.dodge_pressed;
     clip_input.hit_reaction = animation_input.hit_reaction;
     clip_input.recover_pressed = animation_input.recover_pressed;
     arut::animation::Pose clip_pose{};
     std::vector<arut::animation::AnimationEvent> clip_events;
     if (state.humanoid_clip_animator.update(
          frame_delta, clip_input, clip_pose, clip_events) &&
         clip_pose.joint_count == arut::core::kHumanoidBoneCount) {
      for (std::size_t joint = 0; joint < clip_pose.joint_count; ++joint) {
       pose.local_translations[joint] = clip_pose.joints[joint].translation;
       pose.local_rotations[joint] = {
        clip_pose.joints[joint].rotation.x, clip_pose.joints[joint].rotation.y,
        clip_pose.joints[joint].rotation.z, clip_pose.joints[joint].rotation.w};
      }
     } else {
      state.humanoid_clip_animator_ready = false;
      log_warn("DEFAULT_HUMANOID retargeted clip playback failed; using procedural animation");
     }
    }
    const bool stable_grounded_pose =
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::Idle ||
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::Land ||
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::AttackLight ||
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::AttackHeavy ||
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::Block ||
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::HitReact ||
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::Recover;
    const bool kicking =
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::AttackKick;
    const bool airborne =
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::Jump ||
     animation_snapshot.state == arut::rendering::HumanoidAnimationState::Fall ||
     !character.grounded;
    const float gait_wave = std::sin(animation_snapshot.phase_seconds);
    const std::array<bool, 2> stance{
     !airborne && (kicking || stable_grounded_pose || gait_wave >= 0.0F),
     !airborne && !kicking && (stable_grounded_pose || gait_wave < 0.0F)
    };
    const float yaw_cosine = std::cos(character.facing_yaw_radians);
    const float yaw_sine = std::sin(character.facing_yaw_radians);
    for (std::size_t foot = 0; foot < 2U; ++foot) {
     auto& plant = state.foot_plants[foot];
     state.foot_ik_clamped[foot] = false;
     if (!stance[foot]) {
      plant.active = false;
      plant.blend = 0.0F;
      continue;
     }
     const float side = foot == 0U ? -1.0F : 1.0F;
     constexpr float ankle_local_forward = -0.10F;
     const float ankle_local_side = side * 0.11F;
     if (!plant.active) {
      const float anchor_x = character.position.x +
       yaw_cosine * ankle_local_side - yaw_sine * ankle_local_forward;
      const float anchor_z = character.position.z +
       yaw_sine * ankle_local_side + yaw_cosine * ankle_local_forward;
      const auto ground = terrain_ground_query(anchor_x, anchor_z, nullptr);
      if (!ground.valid) continue;
      plant.world_anchor = {anchor_x, ground.height + 0.09F, anchor_z};
      plant.active = true;
      plant.blend = 0.0F;
     }
     const auto ground = terrain_ground_query(
      plant.world_anchor.x, plant.world_anchor.z, nullptr);
     if (!ground.valid) {
      plant.active = false;
      plant.blend = 0.0F;
      continue;
     }
     plant.world_anchor.y = ground.height + 0.09F;
     plant.blend = std::min(1.0F, plant.blend + frame_delta * 12.0F);

     constexpr float root_render_offset = 0.10F;
     constexpr float hip_height = 0.84F;
     const float hip_world_x = character.position.x + yaw_cosine * ankle_local_side;
     const float hip_world_y = character.position.y + root_render_offset + hip_height;
     const float hip_world_z = character.position.z + yaw_sine * ankle_local_side;
     const float delta_x = plant.world_anchor.x - hip_world_x;
     const float delta_y = plant.world_anchor.y - hip_world_y;
     const float delta_z = plant.world_anchor.z - hip_world_z;
     const float target_forward = -yaw_sine * delta_x + yaw_cosine * delta_z;

     const arut::core::Vec3 local_normal{
      yaw_cosine * ground.normal.x + yaw_sine * ground.normal.z,
      ground.normal.y,
      -yaw_sine * ground.normal.x + yaw_cosine * ground.normal.z
     };
     const auto ik = arut::rendering::solve_two_bone_leg_ik(
      -delta_y, target_forward, 0.43F, 0.42F, -0.10F, local_normal);
     if (!ik.valid) {
      log_warn("HUMANOID_IK invalid leg solution foot=%zu", foot);
      continue;
     }
     state.foot_ik_clamped[foot] = ik.reach_clamped;
     using Bone = arut::core::HumanoidBone;
     const Bone upper_bone = foot == 0U ? Bone::LeftUpperLeg : Bone::RightUpperLeg;
     const Bone lower_bone = foot == 0U ? Bone::LeftLowerLeg : Bone::RightLowerLeg;
     const Bone foot_bone = foot == 0U ? Bone::LeftFoot : Bone::RightFoot;
     const auto upper_index = static_cast<std::size_t>(upper_bone);
     const auto lower_index = static_cast<std::size_t>(lower_bone);
     const auto foot_index = static_cast<std::size_t>(foot_bone);
     const auto upper_target = arut::rendering::quaternion_from_axis_angle(
      {1.0F, 0.0F, 0.0F}, ik.upper_leg_pitch);
     const auto lower_target = arut::rendering::quaternion_from_axis_angle(
      {1.0F, 0.0F, 0.0F}, ik.lower_leg_pitch);
     const auto foot_pitch = arut::rendering::quaternion_from_axis_angle(
      {1.0F, 0.0F, 0.0F}, ik.foot_pitch);
     const auto foot_roll = arut::rendering::quaternion_from_axis_angle(
      {0.0F, 0.0F, 1.0F}, ik.foot_roll);
     const auto foot_target = arut::rendering::multiply_humanoid_quaternions(
      foot_roll, foot_pitch);
     pose.local_rotations[upper_index] =
      arut::rendering::blend_humanoid_rotation(
       pose.local_rotations[upper_index], upper_target, plant.blend);
     pose.local_rotations[lower_index] =
      arut::rendering::blend_humanoid_rotation(
       pose.local_rotations[lower_index], lower_target, plant.blend);
     pose.local_rotations[foot_index] =
      arut::rendering::blend_humanoid_rotation(
       pose.local_rotations[foot_index], foot_target, plant.blend);
    }

    // Feed the renderer-independent animation runtime with the final
    // procedural + IK pose. This keeps a rolling history for motion matching.
    state.animation_clock_seconds += std::max(0.00001F, frame_delta);
    arut::animation::Pose runtime_pose{};
    runtime_pose.joint_count = arut::core::kHumanoidBoneCount;
    for (std::size_t joint = 0; joint < runtime_pose.joint_count; ++joint) {
     const auto& translation = pose.local_translations[joint];
     const auto& rotation = pose.local_rotations[joint];
     runtime_pose.joints[joint].translation = translation;
     runtime_pose.joints[joint].rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
    }
    arut::core::Vec3 estimated_acceleration{};
    float estimated_angular_velocity = 0.0F;
    if (const auto* previous = state.animation_pose_history.latest()) {
     const float history_delta = static_cast<float>(
      state.animation_clock_seconds - previous->timestamp_seconds);
     if (history_delta > 0.0001F) {
      estimated_acceleration = {
       (character.velocity.x - previous->root_velocity.x) / history_delta,
       (character.velocity.y - previous->root_velocity.y) / history_delta,
       (character.velocity.z - previous->root_velocity.z) / history_delta};
      if (state.has_previous_facing_yaw) {
       const float yaw_delta = character.facing_yaw_radians - state.previous_facing_yaw;
       estimated_angular_velocity = std::atan2(std::sin(yaw_delta), std::cos(yaw_delta)) /
                                     history_delta;
      }
     }
    }
    (void)state.animation_pose_history.push(
     state.animation_clock_seconds, runtime_pose, character.velocity);
    state.previous_facing_yaw = character.facing_yaw_radians;
    state.has_previous_facing_yaw = true;
    arut::animation::TrajectoryPredictionInput trajectory_input{};
    trajectory_input.world_position = character.position;
    trajectory_input.world_velocity = character.velocity;
    trajectory_input.world_acceleration = estimated_acceleration;
    trajectory_input.facing_yaw_radians = character.facing_yaw_radians;
    trajectory_input.angular_velocity_radians = estimated_angular_velocity;
    trajectory_input.maximum_speed = 8.0F;
    constexpr std::array<float, arut::animation::kMotionTrajectorySamples> trajectory_times{
     0.2F, 0.4F, 0.8F};
    state.predicted_trajectory_valid = arut::animation::TrajectoryPredictor::predict(
     trajectory_input, trajectory_times, state.predicted_trajectory);

    arut::rendering::HumanoidSkinningPalette palette{};
    if (arut::rendering::build_humanoid_skinning_palette(
         state.humanoid_skeleton, pose, palette)) {
     arut::rendering::Mat4 model = arut::rendering::Mat4::identity();
     const float cosine = std::cos(character.facing_yaw_radians);
     const float sine = std::sin(character.facing_yaw_radians);
     model.value[0] = cosine;
     model.value[2] = sine;
     model.value[8] = -sine;
     model.value[10] = cosine;
     model.value[12] = character.position.x;
     model.value[13] = character.position.y + 0.10F;
     model.value[14] = character.position.z;
     const std::array<float, 3> camera_target{
      character.position.x, character.position.y + 1.0F, character.position.z};
     const std::span<const arut::core::CharacterAabbObstacle> camera_obstacles =
      state.test_arena_enabled
       ? std::span<const arut::core::CharacterAabbObstacle>{
          state.arena_obstacles.data(), state.arena_obstacles.size()}
       : std::span<const arut::core::CharacterAabbObstacle>{};
     const auto camera = arut::core::solve_third_person_camera(
      camera_target, state.renderer.camera_yaw(), state.renderer.camera_pitch(),
      terrain_ground_query, nullptr, {}, camera_obstacles);
     if (!camera.valid) {
      log_warn("THIRD_PERSON_CAMERA terrain sweep failed; retaining previous camera.");
     } else {
      state.camera_follow_distance = camera.distance;
      state.camera_terrain_obstructed = camera.terrain_obstructed;
      state.camera_geometry_obstructed = camera.geometry_obstructed;
      state.renderer.set_humanoid_state(
       model, palette, camera_target, camera.position);
      if (state.imported_character_active) {
       arut::animation::Pose imported_pose = state.imported_character_rest_pose;
       if (state.imported_character_animator_ready) {
        arut::animation::CharacterAnimationInput imported_input{};
        imported_input.move_x = animation_input.local_move_x;
        imported_input.move_y = animation_input.local_move_y;
        imported_input.horizontal_speed = animation_input.horizontal_speed;
        imported_input.vertical_speed = animation_input.vertical_speed;
        imported_input.grounded = animation_input.grounded;
        imported_input.sprinting = animation_input.horizontal_speed >= 3.25F;
        imported_input.attack_light_pressed = animation_input.attack_light_pressed;
        imported_input.attack_heavy_pressed = animation_input.attack_heavy_pressed;
        imported_input.attack_kick_pressed = animation_input.attack_kick_pressed;
        imported_input.block_held = animation_input.block_held;
        imported_input.dodge_pressed = animation_input.dodge_pressed;
        imported_input.hit_reaction = animation_input.hit_reaction;
        imported_input.recover_pressed = animation_input.recover_pressed;
        std::vector<arut::animation::AnimationEvent> animation_events;
        if (!state.imported_character_animator.update(
             frame_delta, imported_input, imported_pose, animation_events)) {
         state.imported_character_animator_ready = false;
         log_warn("IMPORTED_CHARACTER animation graph failed; holding imported rest pose");
         imported_pose = state.imported_character_rest_pose;
        }
       }
       // This runtime uses capsule-driven movement. Remove planar root travel
       // from imported clips so animation cannot double-translate the actor.
       if (state.imported_character_root_joint < imported_pose.joint_count) {
        const auto root = state.imported_character_root_joint;
        const auto reference =
         state.imported_character_asset.joints[root].rest_local.translation;
        (void)arut::animation::strip_root_motion(imported_pose, root, reference);
       }
       auto imported_model = model;
       const float imported_scale = state.imported_character_model_scale;
       for (std::size_t column = 0; column < 3U; ++column) {
        imported_model.value[column * 4U + 0U] *= imported_scale;
        imported_model.value[column * 4U + 1U] *= imported_scale;
        imported_model.value[column * 4U + 2U] *= imported_scale;
       }
       const float cosine_yaw = std::cos(character.facing_yaw_radians);
       const float sine_yaw = std::sin(character.facing_yaw_radians);
       const float center_x = state.imported_character_bounds_center_x;
       const float center_z = state.imported_character_bounds_center_z;
       const float world_center_x = cosine_yaw * center_x - sine_yaw * center_z;
       const float world_center_z = sine_yaw * center_x + cosine_yaw * center_z;
       imported_model.value[12] = character.position.x - world_center_x * imported_scale;
       imported_model.value[13] = character.position.y + 0.10F -
        state.imported_character_bounds_min_y * imported_scale;
       imported_model.value[14] = character.position.z - world_center_z * imported_scale;
       if (!state.renderer.set_imported_character_pose(
            state.imported_character_asset, imported_pose, imported_model))
        log_warn("IMPORTED_CHARACTER pose upload rejected; retaining previous GPU pose");
      }
     }
    } else {
     log_warn("HUMANOID_SKINNING pose palette generation failed.");
    }
   }
   if (!state.renderer.draw_frame()) log_warn("Vulkan frame submission/presentation failed.");
   else {
    const double elapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - frame_start).count();
    frame_ms_total += elapsed;
    frame_ms_max = std::max(frame_ms_max, elapsed);
    ++frame_count;
    const auto camera = state.renderer.camera_position();
    const auto streaming = state.world_streamer.update(
     state.world_partition, camera[0], camera[2], 2U,
     [&state, &asset_manager, camera](arut::core::WorldCell cell, arut::core::WorldCellContent& content) {
      return build_cell_content(state, asset_manager, cell, camera[0], camera[2], content);
     }, {},
     [&state, &asset_manager, camera](arut::core::WorldCell cell, arut::core::WorldCellContent& content) {
      const auto lod = select_cell_lod(state, cell, camera[0], camera[2]);
      if (lod == content.lod) return false;
      return build_cell_content(state, asset_manager, cell, camera[0], camera[2], content);
     });
    if (streaming.loaded > 0U || streaming.unloaded > 0U || streaming.updated > 0U)
     state.terrain_cells_dirty = true;
    if (state.terrain_cells_dirty) {
     auto updated_cells = state.world_streamer.collect_terrain_meshes(state.world_partition);
     if (state.renderer.set_terrain_cells(updated_cells)) {
      state.terrain_cells = std::move(updated_cells);
      state.terrain_cells_dirty = false;
      log_info("WORLD_STREAMING camera_cell_updated=true loaded=%zu unloaded=%zu lod_refreshed=%zu resident=%zu gpu_cells=%zu",
       streaming.loaded, streaming.unloaded, streaming.updated, streaming.resident, state.terrain_cells.size());
     } else {
      log_warn("WORLD_STREAMING GPU cell mesh refresh failed; retrying on next frame.");
     }
    }
   }
  }
  const auto metrics_now = std::chrono::steady_clock::now();
  const double metrics_seconds = std::chrono::duration<double>(metrics_now - metrics_start).count();
  if (metrics_seconds >= 30.0) {
   if (frame_count > 0) log_info("PERF window=%.2fs fps=%.2f frame_interval_avg_ms=%.3f frame_interval_max_ms=%.3f frames=%" PRIu64,
    metrics_seconds, static_cast<double>(frame_count) / metrics_seconds,
    frame_ms_total / static_cast<double>(frame_count), frame_ms_max, frame_count);
   else log_info("PERF window=%.2fs renderer_idle=true", metrics_seconds);
   float thermal_peak_c = 0.0F;
   char thermal_sensor[96]{};
   if (read_thermal_peak(thermal_peak_c, thermal_sensor, sizeof(thermal_sensor)))
    log_info("THERMAL window=%.2fs peak_zone_c=%.1f sensor=%s source=sysfs",
     metrics_seconds, thermal_peak_c, thermal_sensor);
   else
    log_info("THERMAL window=%.2fs available=false source=sysfs", metrics_seconds);
   if (state.renderer.ready()) {
    if (arut::core::test_lab_includes_humanoid(state.selected_profile)) {
     const auto& character = state.character_controller.state();
     const auto& animation = state.humanoid_animator.snapshot();
     const float speed = std::sqrt(character.velocity.x * character.velocity.x +
                                   character.velocity.z * character.velocity.z);
     log_info("CHARACTER_CONTROLLER position=%.3f,%.3f,%.3f velocity=%.3f,%.3f,%.3f speed=%.3f grounded=%s fixed_steps=%" PRIu64,
      character.position.x, character.position.y, character.position.z,
      character.velocity.x, character.velocity.y, character.velocity.z,
      speed, character.grounded ? "true" : "false", state.character_fixed_steps);
     const auto support = terrain_ground_query(character.position.x, character.position.z, nullptr);
     log_info("CHARACTER_GROUND valid=%s feet_y=%.4f ground_y=%.4f clearance=%.4f normal=%.3f,%.3f,%.3f walkable=%s",
      support.valid ? "true" : "false", character.position.y,
      support.valid ? support.height : 0.0F,
      support.valid ? character.position.y - support.height : 0.0F,
      support.normal.x, support.normal.y, support.normal.z,
      support.valid && support.normal.y >= 0.6691F ? "true" : "false");
     log_info("ANIMATION_RUNTIME_HISTORY frames=%zu trajectory_valid=%s p0=%.3f,%.3f,%.3f p1=%.3f,%.3f,%.3f p2=%.3f,%.3f,%.3f",
      state.animation_pose_history.size(), state.predicted_trajectory_valid ? "true" : "false",
      state.predicted_trajectory[0].position_offset.x, state.predicted_trajectory[0].position_offset.y, state.predicted_trajectory[0].position_offset.z,
      state.predicted_trajectory[1].position_offset.x, state.predicted_trajectory[1].position_offset.y, state.predicted_trajectory[1].position_offset.z,
      state.predicted_trajectory[2].position_offset.x, state.predicted_trajectory[2].position_offset.y, state.predicted_trajectory[2].position_offset.z);
     log_info("ANIMATION_STATE state=%.*s phase=%.3f blend=%.3f speed=%.3f",
      static_cast<int>(arut::rendering::humanoid_animation_state_name(animation.state).size()),
      arut::rendering::humanoid_animation_state_name(animation.state).data(),
      animation.phase_seconds, animation.blend_alpha, animation.horizontal_speed);
     log_info("HUMANOID_IK left_planted=%s right_planted=%s left_clamped=%s right_clamped=%s",
      state.foot_plants[0].active ? "true" : "false",
      state.foot_plants[1].active ? "true" : "false",
      state.foot_ik_clamped[0] ? "true" : "false",
      state.foot_ik_clamped[1] ? "true" : "false");
     const auto camera_position = state.renderer.camera_position();
     log_info("THIRD_PERSON_CAMERA distance=%.3f terrain_obstructed=%s geometry_obstructed=%s position=%.3f,%.3f,%.3f yaw=%.3f pitch=%.3f",
      state.camera_follow_distance,
      state.camera_terrain_obstructed ? "true" : "false",
      state.camera_geometry_obstructed ? "true" : "false",
      camera_position[0], camera_position[1], camera_position[2],
      state.renderer.camera_yaw(), state.renderer.camera_pitch());
    }
    const auto render = state.renderer.profile_snapshot();
    log_info("RENDER_PROFILE cpu_frame_ms=%.3f gpu_frame_ms=%.3f visibility_pass_ms=%.3f shadow_pass_ms=%.3f main_pass_ms=%.3f gpu_memory_bytes=%" PRIu64 " gpu_memory_usage_bytes=%" PRIu64 " gpu_memory_budget_bytes=%" PRIu64 " budget=%s gpu_visibility=%s gpu_timestamps=%s draw_calls=%u visible_objects=%u triangles=%u",
     render.cpu_frame_ms, render.gpu_frame_ms, render.visibility_pass_ms, render.shadow_pass_ms, render.main_pass_ms,
     render.gpu_memory_bytes, render.gpu_memory_usage_bytes, render.gpu_memory_budget_bytes,
     render.gpu_memory_budget_available ? "available" : "unavailable",
     render.gpu_visibility_enabled ? "compute" : "cpu_fallback",
     render.gpu_timing_available ? "available" : "unavailable",
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
