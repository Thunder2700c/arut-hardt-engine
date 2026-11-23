#include "arut/rendering/vulkan/android_renderer.hpp"
#include "arut/rendering/robot_mannequin.hpp"
#include "arut/rendering/humanoid_skinning.hpp"
#include "arut/rendering/imported_mesh.hpp"
#include "arut/rendering/imported_skinning.hpp"

#include <android/log.h>
#include <vulkan/vulkan_android.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <cmath>
#include <cstdio>
#include <vector>
#include <utility>
#include <cstddef>
#include <cstdint>
#include <chrono>
#include <android/asset_manager.h>
#include "arut/rendering/mesh.hpp"
#include "arut/rendering/image.hpp"
#include "arut/rendering/brdf_lut.hpp"
#include "arut/rendering/visibility.hpp"
#include "arut/rendering/shadow.hpp"
#include "arut/rendering/environment_probe.hpp"

extern "C" void arut_android_log(int priority, const char* message) noexcept;

namespace arut::rendering::vulkan {
namespace {
static_assert(sizeof(VkDrawIndexedIndirectCommand) ==
              sizeof(arut::rendering::IndexedIndirectArguments));
static_assert(offsetof(VkDrawIndexedIndirectCommand, instanceCount) ==
              offsetof(arut::rendering::IndexedIndirectArguments, instance_count));
static_assert(offsetof(VkDrawIndexedIndirectCommand, firstInstance) ==
              offsetof(arut::rendering::IndexedIndirectArguments, first_instance));
constexpr char kTag[] = "ARUT-HARDT";
void log_error(const char* message, VkResult result = VK_SUCCESS) {
    char buffer[512]{}; std::snprintf(buffer, sizeof(buffer), "%s (VkResult=%d)", message, static_cast<int>(result));
    arut_android_log(ANDROID_LOG_ERROR, buffer);
}
bool has_instance_extension(const char* name) {
    std::uint32_t count = 0;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr) != VK_SUCCESS) return false;
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data()) != VK_SUCCESS) return false;
    return std::any_of(extensions.begin(), extensions.end(), [name](const auto& e) {
        return std::strcmp(e.extensionName, name) == 0;
    });
}
std::uint64_t imported_skeleton_signature(
    const arut::animation::SkeletalAsset& asset) noexcept {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto mix = [&hash](const void* data, std::size_t size) {
        const auto* bytes = static_cast<const std::uint8_t*>(data);
        for (std::size_t i = 0; i < size; ++i) {
            hash ^= bytes[i];
            hash *= 1099511628211ULL;
        }
    };
    mix(&asset.joint_count, sizeof(asset.joint_count));
    if (asset.joint_count > arut::animation::kMaxImportedJoints) return 0U;
    for (std::size_t joint = 0; joint < asset.joint_count; ++joint) {
        const auto& source = asset.joints[joint];
        mix(&source.parent, sizeof(source.parent));
        const std::size_t name_size = source.name.size();
        mix(&name_size, sizeof(name_size));
        mix(source.name.data(), name_size);
        mix(source.inverse_bind_matrix.data(), sizeof(source.inverse_bind_matrix));
    }
    return hash == 0U ? 1U : hash;
}

bool has_device_extension(VkPhysicalDevice device, const char* name) {
    std::uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS) return false;
    std::vector<VkExtensionProperties> extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, extensions.data()) != VK_SUCCESS) return false;
    return std::any_of(extensions.begin(), extensions.end(), [name](const auto& e) {
        return std::strcmp(e.extensionName, name) == 0;
    });
}
}

AndroidRenderer::~AndroidRenderer() { shutdown(); }

bool AndroidRenderer::initialize(
    ANativeWindow* window, AAssetManager* assets,
    std::span<const arut::core::CubeRenderInstance> ecs_instances,
    std::span<const arut::core::TerrainMesh> terrain_meshes,
    bool render_mannequin,
    std::span<const arut::rendering::Mat4> static_scene_instances) noexcept {
    shutdown();
    mannequin_scene_enabled_ = render_mannequin;
    try {
        static_scene_instances_.assign(static_scene_instances.begin(), static_scene_instances.end());
    } catch (...) {
        log_error("Static test-scene instance allocation failed");
        shutdown();
        return false;
    }
    if (static_scene_instances_.size() > UINT32_MAX) {
        log_error("Static test-scene instance count exceeds Vulkan draw limit");
        shutdown();
        return false;
    }
    has_external_humanoid_state_ = false;
    jump_pointer_ = -1;
    jump_pressed_ = false;
    if (terrain_meshes.empty()) { log_error("Renderer rejected empty streamed terrain cell set"); return false; }
    try {
        terrain_meshes_.assign(terrain_meshes.begin(), terrain_meshes.end());
        if (std::any_of(terrain_meshes_.begin(), terrain_meshes_.end(),
                        [](const auto& mesh) { return !mesh.valid(); })) {
            terrain_meshes_.clear();
            log_error("Renderer rejected invalid streamed terrain cell mesh");
            return false;
        }
    } catch (...) { terrain_meshes_.clear(); log_error("Renderer could not retain streamed terrain cells"); return false; }
    visibility_instances_.clear();
    visibility_source_instance_count_ = 0U;
    if (!ecs_instances.empty() &&
        ecs_instances.size() != arut::core::CubeSpawnSystem::target_count) {
        log_error("Renderer rejected stress scene: expected either zero or exactly 5000 ECS render instances");
        return false;
    }
    try {
        visibility_instances_.reserve(ecs_instances.size());
        for (const auto& source : ecs_instances) {
            arut::rendering::GpuVisibilityInstance instance{};
            instance.bounds = source.bounds;
            instance.model = source.model;
            visibility_instances_.push_back(instance);
        }
    } catch (...) {
        visibility_instances_.clear();
        log_error("Renderer could not copy ECS render-instance data");
        return false;
    }
    visibility_source_instance_count_ = static_cast<std::uint32_t>(visibility_instances_.size());
    char source_message[160]{};
    std::snprintf(source_message, sizeof(source_message),
                  "Renderer input accepted: source=ECS CubeSpawnSystem instances=%u",
                  visibility_source_instance_count_);
    arut_android_log(ANDROID_LOG_INFO, source_message);
    char scene_message[128]{};
    std::snprintf(scene_message, sizeof(scene_message),
        "TEST_LAB_RENDER_PROFILE stress=%s humanoid=%s",
        visibility_source_instance_count_ == 0U ? "false" : "true",
        mannequin_scene_enabled_ ? "true" : "false");
    arut_android_log(ANDROID_LOG_INFO, scene_message);

    window_ = window;
    assets_ = assets;
    if (!window || !assets_ || !create_instance() || !create_surface(window) ||
        !select_device() || !create_device() || !create_swapchain() ||
        !create_frame_resources()) {
        shutdown();
        return false;
    }
    char ready_message[256]{};
    std::snprintf(ready_message, sizeof(ready_message),
                  "Vulkan swapchain ready: %ux%u, images=%zu",
                  extent_.width, extent_.height, images_.size());
    arut_android_log(ANDROID_LOG_INFO, ready_message);
    return true;
}

bool AndroidRenderer::set_terrain_cells(
    std::span<const arut::core::TerrainMesh> terrain_meshes) noexcept {
    if (!ready() || terrain_meshes.empty()) return false;
    std::vector<arut::core::TerrainMesh> replacement;
    try {
        replacement.assign(terrain_meshes.begin(), terrain_meshes.end());
        if (std::any_of(replacement.begin(), replacement.end(),
                        [](const auto& mesh) { return !mesh.valid(); })) return false;
    } catch (...) { return false; }
    if (vkDeviceWaitIdle(device_) != VK_SUCCESS) return false;
    for (const auto& terrain : terrain_gpu_meshes_) {
        if (terrain.vertex_buffer) vkDestroyBuffer(device_, terrain.vertex_buffer, nullptr);
        if (terrain.vertex_memory) vkFreeMemory(device_, terrain.vertex_memory, nullptr);
        if (terrain.index_buffer) vkDestroyBuffer(device_, terrain.index_buffer, nullptr);
        if (terrain.index_memory) vkFreeMemory(device_, terrain.index_memory, nullptr);
        if (terrain.instance_buffer) vkDestroyBuffer(device_, terrain.instance_buffer, nullptr);
        if (terrain.instance_memory) vkFreeMemory(device_, terrain.instance_memory, nullptr);
        if (terrain.allocation_bytes <= tracked_gpu_memory_bytes_)
            tracked_gpu_memory_bytes_ -= terrain.allocation_bytes;
    }
    terrain_gpu_meshes_.clear();
    terrain_meshes_ = std::move(replacement);
    return create_terrain_geometry();
}

bool AndroidRenderer::create_frame_resources() noexcept {
    return create_render_targets() &&
           create_triangle_geometry() && create_triangle_pipeline() &&
           create_pbr_geometry() &&
           (!mannequin_scene_enabled_ || create_mannequin_geometry()) &&
           create_static_scene_geometry() && create_terrain_geometry() && create_gpu_visibility_resources() &&
           create_fallback_texture() &&
           create_environment_cubemap() && create_environment_probe_cubemaps() &&
           create_brdf_lut() &&
           create_pbr_pipeline() && create_shadow_resources() &&
           create_shadow_pipeline() && create_pbr_descriptors() &&
           create_commands_and_sync();
}

bool AndroidRenderer::create_gpu_visibility_resources() noexcept {
    gpu_visibility_enabled_ = false;
    if (!compute_queue_supported_) {
        arut_android_log(ANDROID_LOG_WARN, "GPU visibility unavailable: selected graphics queue has no compute capability; using CPU culling.");
        return true;
    }
    if (visibility_source_instance_count_ == 0U) {
        arut_android_log(ANDROID_LOG_INFO,
            "GPU visibility skipped: selected profile has no ECS stress instances.");
        return true;
    }
    if (!visibility_input_buffer_ || !instance_buffer_ || !indirect_draw_buffer_) {
        log_error("GPU visibility buffers are not initialized; using CPU culling.");
        return true;
    }

    VkPhysicalDeviceProperties compute_properties{};
    vkGetPhysicalDeviceProperties(physical_device_, &compute_properties);
    const VkDeviceSize required_storage_range = std::max<VkDeviceSize>(
        static_cast<VkDeviceSize>(visibility_source_instance_count_) *
            sizeof(arut::rendering::GpuVisibilityInstance),
        static_cast<VkDeviceSize>(visibility_source_instance_count_) *
            sizeof(arut::rendering::Mat4));
    const std::uint32_t dispatch_groups =
        arut::rendering::visibility_workgroup_count(visibility_source_instance_count_);
    if (compute_properties.limits.maxComputeWorkGroupInvocations <
            arut::rendering::kVisibilityWorkgroupSize ||
        compute_properties.limits.maxComputeWorkGroupSize[0] <
            arut::rendering::kVisibilityWorkgroupSize ||
        compute_properties.limits.maxComputeWorkGroupCount[0] < dispatch_groups ||
        compute_properties.limits.maxPushConstantsSize <
            sizeof(arut::rendering::GpuVisibilityPushConstants) ||
        compute_properties.limits.maxStorageBufferRange < required_storage_range) {
        arut_android_log(ANDROID_LOG_WARN, "GPU visibility skipped: device compute/storage limits are insufficient; using CPU culling.");
        return true;
    }

    const auto disable_gpu_visibility = [this]() {
        if (visibility_compute_pipeline_) vkDestroyPipeline(device_, visibility_compute_pipeline_, nullptr);
        if (visibility_pipeline_layout_) vkDestroyPipelineLayout(device_, visibility_pipeline_layout_, nullptr);
        if (visibility_descriptor_pool_) vkDestroyDescriptorPool(device_, visibility_descriptor_pool_, nullptr);
        if (visibility_set_layout_) vkDestroyDescriptorSetLayout(device_, visibility_set_layout_, nullptr);
        visibility_compute_pipeline_ = VK_NULL_HANDLE;
        visibility_pipeline_layout_ = VK_NULL_HANDLE;
        visibility_descriptor_pool_ = VK_NULL_HANDLE;
        visibility_descriptor_set_ = VK_NULL_HANDLE;
        visibility_set_layout_ = VK_NULL_HANDLE;
        gpu_visibility_enabled_ = false;
    };

    const std::array<VkDescriptorSetLayoutBinding, 3> bindings{{
        {0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr}
    }};
    VkDescriptorSetLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
    layout_info.pBindings = bindings.data();
    VkResult result = vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &visibility_set_layout_);
    if (result != VK_SUCCESS) {
        log_error("Create GPU visibility descriptor layout failed", result);
        disable_gpu_visibility();
        return true;
    }

    VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3};
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.maxSets = 1;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &pool_size;
    result = vkCreateDescriptorPool(device_, &pool_info, nullptr, &visibility_descriptor_pool_);
    if (result != VK_SUCCESS) {
        log_error("Create GPU visibility descriptor pool failed; retaining CPU fallback", result);
        disable_gpu_visibility();
        return true;
    }
    VkDescriptorSetAllocateInfo set_allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    set_allocation.descriptorPool = visibility_descriptor_pool_;
    set_allocation.descriptorSetCount = 1;
    set_allocation.pSetLayouts = &visibility_set_layout_;
    result = vkAllocateDescriptorSets(device_, &set_allocation, &visibility_descriptor_set_);
    if (result != VK_SUCCESS) {
        log_error("Allocate GPU visibility descriptor set failed; retaining CPU fallback", result);
        disable_gpu_visibility();
        return true;
    }

    const VkDeviceSize input_bytes =
        static_cast<VkDeviceSize>(visibility_source_instance_count_) *
        sizeof(arut::rendering::GpuVisibilityInstance);
    const VkDeviceSize output_bytes =
        static_cast<VkDeviceSize>(visibility_source_instance_count_) *
        sizeof(arut::rendering::Mat4);
    const std::array<VkDescriptorBufferInfo, 3> buffer_infos{{
        {visibility_input_buffer_, 0, input_bytes},
        {instance_buffer_, 0, output_bytes},
        {indirect_draw_buffer_, 0, sizeof(arut::rendering::IndexedIndirectArguments)}
    }};
    std::array<VkWriteDescriptorSet, 3> writes{};
    for (std::uint32_t index = 0; index < writes.size(); ++index) {
        writes[index] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
                         visibility_descriptor_set_, index, 0, 1,
                         VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, nullptr,
                         &buffer_infos[index], nullptr};
    }
    vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(writes.size()),
                           writes.data(), 0, nullptr);

    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    push.offset = 0;
    push.size = sizeof(arut::rendering::GpuVisibilityPushConstants);
    VkPipelineLayoutCreateInfo pipeline_layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout.setLayoutCount = 1;
    pipeline_layout.pSetLayouts = &visibility_set_layout_;
    pipeline_layout.pushConstantRangeCount = 1;
    pipeline_layout.pPushConstantRanges = &push;
    result = vkCreatePipelineLayout(device_, &pipeline_layout, nullptr, &visibility_pipeline_layout_);
    if (result != VK_SUCCESS) {
        log_error("Create GPU visibility pipeline layout failed; retaining CPU fallback", result);
        disable_gpu_visibility();
        return true;
    }

    AAsset* asset = AAssetManager_open(assets_, "shaders/visibility_cull.comp.spv", AASSET_MODE_BUFFER);
    if (!asset) {
        log_error("GPU visibility SPIR-V missing; retaining CPU fallback");
        disable_gpu_visibility();
        return true;
    }
    const off_t length = AAsset_getLength(asset);
    if (length <= 0 || (length % 4) != 0) {
        AAsset_close(asset);
        log_error("GPU visibility SPIR-V has invalid size; retaining CPU fallback");
        disable_gpu_visibility();
        return true;
    }
    std::vector<std::uint32_t> code(static_cast<std::size_t>(length) / sizeof(std::uint32_t));
    const int bytes_read = AAsset_read(asset, code.data(), static_cast<std::size_t>(length));
    AAsset_close(asset);
    if (bytes_read != length || code.empty() || code.front() != 0x07230203U) {
        log_error("GPU visibility SPIR-V failed read/magic validation; retaining CPU fallback");
        disable_gpu_visibility();
        return true;
    }

    VkShaderModuleCreateInfo module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = code.size() * sizeof(std::uint32_t);
    module_info.pCode = code.data();
    VkShaderModule module = VK_NULL_HANDLE;
    result = vkCreateShaderModule(device_, &module_info, nullptr, &module);
    if (result != VK_SUCCESS) {
        log_error("Create GPU visibility shader module failed; retaining CPU fallback", result);
        disable_gpu_visibility();
        return true;
    }
    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage.module = module;
    stage.pName = "main";
    VkComputePipelineCreateInfo compute_pipeline{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    compute_pipeline.stage = stage;
    compute_pipeline.layout = visibility_pipeline_layout_;
    result = vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &compute_pipeline,
                                      nullptr, &visibility_compute_pipeline_);
    vkDestroyShaderModule(device_, module, nullptr);
    if (result != VK_SUCCESS) {
        log_error("Create GPU visibility compute pipeline failed; retaining CPU fallback", result);
        disable_gpu_visibility();
        return true;
    }

    gpu_visibility_enabled_ = true;
    arut_android_log(ANDROID_LOG_INFO, "GPU frustum culling enabled: compute compaction writes vertex instances and indexed indirect arguments.");
    return true;
}

bool AndroidRenderer::record_gpu_visibility(VkCommandBuffer command_buffer) noexcept {
    if (!gpu_visibility_enabled_) return true;

    const arut::rendering::IndexedIndirectArguments reset_arguments{
        cube_index_count_, 0U, 0U, 0, 0U
    };
    void* mapped = nullptr;
    VkResult result = vkMapMemory(device_, indirect_draw_memory_, 0,
                                  sizeof(reset_arguments), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map indexed indirect arguments before GPU culling failed", result);
        return false;
    }
    std::memcpy(mapped, &reset_arguments, sizeof(reset_arguments));
    vkUnmapMemory(device_, indirect_draw_memory_);

    std::array<VkBufferMemoryBarrier, 3> before_compute{};
    before_compute[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr,
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        visibility_input_buffer_, 0, VK_WHOLE_SIZE};
    before_compute[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr,
        VK_ACCESS_HOST_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        indirect_draw_buffer_, 0, sizeof(arut::rendering::IndexedIndirectArguments)};
    before_compute[2] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr,
        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        instance_buffer_, 0, VK_WHOLE_SIZE};
    vkCmdPipelineBarrier(command_buffer,
        VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0,
        0, nullptr, static_cast<std::uint32_t>(before_compute.size()),
        before_compute.data(), 0, nullptr);

    arut::rendering::GpuVisibilityPushConstants push{};
    for (std::size_t plane = 0; plane < 6U; ++plane) {
        for (std::size_t component = 0; component < 4U; ++component) {
            push.frustum_planes[plane][component] =
                visibility_frustum_planes_[plane * 4U + component];
        }
    }
    push.instance_count = visibility_source_instance_count_;
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, visibility_compute_pipeline_);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            visibility_pipeline_layout_, 0, 1,
                            &visibility_descriptor_set_, 0, nullptr);
    vkCmdPushConstants(command_buffer, visibility_pipeline_layout_, VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(push), &push);
    vkCmdDispatch(command_buffer,
        arut::rendering::visibility_workgroup_count(visibility_source_instance_count_), 1U, 1U);

    std::array<VkBufferMemoryBarrier, 2> after_compute{};
    after_compute[0] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr,
        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        instance_buffer_, 0, VK_WHOLE_SIZE};
    after_compute[1] = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER, nullptr,
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_HOST_WRITE_BIT,
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_HOST_READ_BIT,
        VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED,
        indirect_draw_buffer_, 0, sizeof(arut::rendering::IndexedIndirectArguments)};
    vkCmdPipelineBarrier(command_buffer,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_HOST_BIT,
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
            VK_PIPELINE_STAGE_HOST_BIT, 0,
        0, nullptr, static_cast<std::uint32_t>(after_compute.size()),
        after_compute.data(), 0, nullptr);
    return true;
}

bool AndroidRenderer::create_instance() noexcept {
    std::vector<const char*> extensions{
        VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };
    properties2_extension_enabled_ =
        has_instance_extension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    if (properties2_extension_enabled_)
        extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "ARUT-HARDT";
    app.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app.pEngineName = "ARUT-HARDT Engine";
    app.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    const VkResult result = vkCreateInstance(&info, nullptr, &instance_);
    if (result != VK_SUCCESS) log_error("vkCreateInstance failed", result);
    return result == VK_SUCCESS;
}

bool AndroidRenderer::create_surface(ANativeWindow* window) noexcept {
    VkAndroidSurfaceCreateInfoKHR info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    info.window = window;
    const VkResult result = vkCreateAndroidSurfaceKHR(instance_, &info, nullptr, &surface_);
    if (result != VK_SUCCESS) log_error("vkCreateAndroidSurfaceKHR failed", result);
    return result == VK_SUCCESS;
}

bool AndroidRenderer::select_device() noexcept {
    std::uint32_t count = 0;
    VkResult result = vkEnumeratePhysicalDevices(instance_, &count, nullptr);
    if (result != VK_SUCCESS || count == 0) {
        log_error("No Vulkan physical device available", result);
        return false;
    }
    std::vector<VkPhysicalDevice> devices(count);
    result = vkEnumeratePhysicalDevices(instance_, &count, devices.data());
    if (result != VK_SUCCESS) return false;

    for (VkPhysicalDevice candidate : devices) {
        if (!has_device_extension(candidate, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) continue;
        std::uint32_t family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &family_count, nullptr);
        std::vector<VkQueueFamilyProperties> families(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(candidate, &family_count, families.data());
        std::uint32_t graphics = UINT32_MAX;
        std::uint32_t graphics_compute = UINT32_MAX;
        std::uint32_t present = UINT32_MAX;
        for (std::uint32_t i = 0; i < family_count; ++i) {
            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphics == UINT32_MAX) graphics = i;
            if ((families[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT)) ==
                    (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT) &&
                graphics_compute == UINT32_MAX) graphics_compute = i;
            VkBool32 supported = VK_FALSE;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface_, &supported) == VK_SUCCESS &&
                supported && present == UINT32_MAX) present = i;
        }
        if (graphics_compute != UINT32_MAX) graphics = graphics_compute;
        if (graphics != UINT32_MAX && present != UINT32_MAX) {
            physical_device_ = candidate;
            graphics_family_ = graphics;
            present_family_ = present;
            memory_budget_extension_enabled_ = properties2_extension_enabled_ &&
                has_device_extension(candidate, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
            compute_queue_supported_ =
                (families[graphics].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U;
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(candidate, &properties);
            timestamp_valid_bits_ = families[graphics].timestampValidBits;
            timestamp_period_ns_ = static_cast<double>(properties.limits.timestampPeriod);
            timestamp_queries_available_ = properties.limits.timestampComputeAndGraphics == VK_TRUE &&
                                           timestamp_valid_bits_ > 0U && timestamp_period_ns_ > 0.0;
            char gpu_message[512]{}; std::snprintf(gpu_message, sizeof(gpu_message), "Selected Vulkan GPU: %s vendor=0x%04x device=0x%04x api=%u.%u.%u driver=%u type=%d", properties.deviceName, properties.vendorID, properties.deviceID, VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion), properties.driverVersion, static_cast<int>(properties.deviceType));
            arut_android_log(ANDROID_LOG_INFO, gpu_message);
            return true;
        }
    }
    log_error("No Vulkan GPU supports graphics, Android presentation and VK_KHR_swapchain");
    return false;
}

bool AndroidRenderer::create_device() noexcept {
    const float priority = 1.0F;
    const std::array<std::uint32_t, 2> families{graphics_family_, present_family_};
    std::array<VkDeviceQueueCreateInfo, 2> queues{};
    const std::uint32_t queue_count = graphics_family_ == present_family_ ? 1U : 2U;
    for (std::uint32_t i = 0; i < queue_count; ++i) {
        queues[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queues[i].queueFamilyIndex = families[i];
        queues[i].queueCount = 1;
        queues[i].pQueuePriorities = &priority;
    }
    std::vector<const char*> extensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    if (memory_budget_extension_enabled_)
        extensions.push_back(VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
    VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    info.queueCreateInfoCount = queue_count;
    info.pQueueCreateInfos = queues.data();
    info.enabledExtensionCount = static_cast<std::uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    const VkResult result = vkCreateDevice(physical_device_, &info, nullptr, &device_);
    if (result != VK_SUCCESS) {
        log_error("vkCreateDevice failed", result);
        return false;
    }
    vkGetDeviceQueue(device_, graphics_family_, 0, &graphics_queue_);
    vkGetDeviceQueue(device_, present_family_, 0, &present_queue_);
    return true;
}

bool AndroidRenderer::create_swapchain() noexcept {
    VkSurfaceCapabilitiesKHR caps{};
    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device_, surface_, &caps);
    if (result != VK_SUCCESS) return false;
    std::uint32_t format_count = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, nullptr);
    if (result != VK_SUCCESS || format_count == 0) return false;
    std::vector<VkSurfaceFormatKHR> formats(format_count);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device_, surface_, &format_count, formats.data());
    if (result != VK_SUCCESS) return false;
    VkSurfaceFormatKHR chosen = formats.front();
    if (formats.size() == 1 && formats.front().format == VK_FORMAT_UNDEFINED) {
        chosen.format = VK_FORMAT_B8G8R8A8_UNORM;
        chosen.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    }
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_R8G8B8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { chosen = format; break; }
        if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) chosen = format;
    }
    color_format_ = chosen.format;
    if ((caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0) {
        log_error("Surface images do not support color-attachment usage");
        return false;
    }
    if (caps.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        extent_ = caps.currentExtent;
    } else {
        const auto width = static_cast<std::uint32_t>(std::max(1, ANativeWindow_getWidth(window_)));
        const auto height = static_cast<std::uint32_t>(std::max(1, ANativeWindow_getHeight(window_)));
        extent_.width = std::clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width);
        extent_.height = std::clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    // Android surface capabilities normally provide currentExtent. The fallback is updated
    // by the caller through the native window dimensions before swapchain creation.
    std::uint32_t image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && image_count > caps.maxImageCount) image_count = caps.maxImageCount;
    VkCompositeAlphaFlagBitsKHR alpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if ((caps.supportedCompositeAlpha & alpha) == 0) {
        constexpr std::array<VkCompositeAlphaFlagBitsKHR, 3> choices{
            VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
            VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
        };
        for (auto candidate : choices) if (caps.supportedCompositeAlpha & candidate) { alpha = candidate; break; }
    }
    const std::array<std::uint32_t, 2> family_indices{graphics_family_, present_family_};
    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = surface_;
    info.minImageCount = image_count;
    info.imageFormat = color_format_;
    info.imageColorSpace = chosen.colorSpace;
    info.imageExtent = extent_;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphics_family_ != present_family_) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = family_indices.data();
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = alpha;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;
    result = vkCreateSwapchainKHR(device_, &info, nullptr, &swapchain_);
    if (result != VK_SUCCESS) {
        log_error("vkCreateSwapchainKHR failed", result);
        return false;
    }
    std::uint32_t actual_count = 0;
    result = vkGetSwapchainImagesKHR(device_, swapchain_, &actual_count, nullptr);
    if (result != VK_SUCCESS || actual_count == 0) return false;
    images_.resize(actual_count);
    result = vkGetSwapchainImagesKHR(device_, swapchain_, &actual_count, images_.data());
    return result == VK_SUCCESS;
}

void AndroidRenderer::account_gpu_allocation(VkDeviceSize bytes) noexcept {
    const std::uint64_t amount = static_cast<std::uint64_t>(bytes);
    if (amount > std::numeric_limits<std::uint64_t>::max() - tracked_gpu_memory_bytes_)
        tracked_gpu_memory_bytes_ = std::numeric_limits<std::uint64_t>::max();
    else
        tracked_gpu_memory_bytes_ += amount;
}

std::uint32_t AndroidRenderer::find_memory_type(std::uint32_t type_bits, VkMemoryPropertyFlags properties) const noexcept {
    VkPhysicalDeviceMemoryProperties memory{};
    vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory);
    for (std::uint32_t i = 0; i < memory.memoryTypeCount; ++i) {
        if ((type_bits & (1U << i)) != 0U &&
            (memory.memoryTypes[i].propertyFlags & properties) == properties) return i;
    }
    return UINT32_MAX;
}

bool AndroidRenderer::create_depth_resources() noexcept {
    constexpr std::array<VkFormat, 3> candidates{
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM
    };
    for (const VkFormat candidate : candidates) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physical_device_, candidate, &properties);
        if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0U) {
            depth_format_ = candidate;
            break;
        }
    }
    if (depth_format_ == VK_FORMAT_UNDEFINED) {
        log_error("No supported depth attachment format");
        return false;
    }

    VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = depth_format_;
    image.extent = {extent_.width, extent_.height, 1};
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkResult result = vkCreateImage(device_, &image, nullptr, &depth_image_);
    if (result != VK_SUCCESS) {
        log_error("vkCreateImage for depth attachment failed", result);
        return false;
    }

    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, depth_image_, &requirements);
    const std::uint32_t memory_type = find_memory_type(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memory_type == UINT32_MAX) {
        log_error("No device-local memory type for depth attachment");
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memory_type;
    result = vkAllocateMemory(device_, &allocation, nullptr, &depth_memory_);
    if (result != VK_SUCCESS) {
        log_error("vkAllocateMemory for depth attachment failed", result);
        return false;
    }
    result = vkBindImageMemory(device_, depth_image_, depth_memory_, 0);
    if (result != VK_SUCCESS) {
        log_error("vkBindImageMemory for depth attachment failed", result);
        return false;
    }
    account_gpu_allocation(requirements.size);

    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = depth_image_;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = depth_format_;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (depth_format_ == VK_FORMAT_D24_UNORM_S8_UINT) view.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    view.subresourceRange.levelCount = 1;
    view.subresourceRange.layerCount = 1;
    result = vkCreateImageView(device_, &view, nullptr, &depth_view_);
    if (result != VK_SUCCESS) {
        log_error("vkCreateImageView for depth attachment failed", result);
        return false;
    }
    return true;
}

bool AndroidRenderer::create_render_targets() noexcept {
    image_views_.resize(images_.size(), VK_NULL_HANDLE);
    for (std::size_t i = 0; i < images_.size(); ++i) {
        VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        view.image = images_[i];
        view.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view.format = color_format_;
        view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view.subresourceRange.levelCount = 1;
        view.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device_, &view, nullptr, &image_views_[i]) != VK_SUCCESS) return false;
    }
    if (!create_depth_resources()) return false;
    std::array<VkAttachmentDescription, 2> attachments{};
    attachments[0].format = color_format_;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachments[1].format = depth_format_;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference color_ref{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
    VkAttachmentReference depth_ref{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_ref;
    subpass.pDepthStencilAttachment = &depth_ref;
    VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    pass.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    pass.pAttachments = attachments.data();
    pass.subpassCount = 1;
    pass.pSubpasses = &subpass;
    if (vkCreateRenderPass(device_, &pass, nullptr, &render_pass_) != VK_SUCCESS) return false;

    framebuffers_.resize(image_views_.size(), VK_NULL_HANDLE);
    for (std::size_t i = 0; i < image_views_.size(); ++i) {
        VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        fb.renderPass = render_pass_;
        const std::array<VkImageView, 2> framebuffer_attachments{image_views_[i], depth_view_};
        fb.attachmentCount = static_cast<std::uint32_t>(framebuffer_attachments.size());
        fb.pAttachments = framebuffer_attachments.data();
        fb.width = extent_.width;
        fb.height = extent_.height;
        fb.layers = 1;
        if (vkCreateFramebuffer(device_, &fb, nullptr, &framebuffers_[i]) != VK_SUCCESS) return false;
    }
    return true;
}

bool AndroidRenderer::create_shadow_resources() noexcept {
    constexpr std::array<VkFormat, 3> candidates{
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM
    };
    shadow_cascade_count_ = quality_.shadows
        ? std::clamp<std::uint32_t>(quality_.shadow_cascades, 1U, 4U) : 1U;
    shadow_extent_ = quality_.shadows
        ? VkExtent2D{quality_.shadow_map_resolution, quality_.shadow_map_resolution}
        : VkExtent2D{1U, 1U};
    shadow_enabled_ = false;
    for (const VkFormat candidate : candidates) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physical_device_, candidate, &properties);
        const VkFormatFeatureFlags required =
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
        if (quality_.shadows && (properties.optimalTilingFeatures & required) == required) {
            shadow_format_ = candidate;
            shadow_enabled_ = true;
            break;
        }
    }
    if (shadow_format_ == VK_FORMAT_UNDEFINED) {
        for (const VkFormat candidate : candidates) {
            VkFormatProperties properties{};
            vkGetPhysicalDeviceFormatProperties(physical_device_, candidate, &properties);
            if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) != 0U) {
                shadow_format_ = candidate;
                break;
            }
        }
        if (shadow_format_ == VK_FORMAT_UNDEFINED) {
            log_error("No sampled depth format; shadow fallback unavailable");
            return false;
        }
        shadow_extent_ = {1U, 1U};
        arut_android_log(ANDROID_LOG_WARN, "Sampled depth attachment unsupported; using fully-lit shadow fallback.");
    }

    VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = shadow_format_;
    image.extent = {shadow_extent_.width, shadow_extent_.height, 1};
    image.mipLevels = 1;
    image.arrayLayers = 4;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = shadow_enabled_
        ? (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
        : (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkResult result = vkCreateImage(device_, &image, nullptr, &shadow_image_);
    if (result != VK_SUCCESS) {
        log_error("Create directional shadow image failed", result);
        return false;
    }
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, shadow_image_, &requirements);
    const std::uint32_t memory_type = find_memory_type(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memory_type == UINT32_MAX) {
        log_error("No device-local memory for directional shadow image");
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memory_type;
    result = vkAllocateMemory(device_, &allocation, nullptr, &shadow_memory_);
    if (result == VK_SUCCESS) result = vkBindImageMemory(device_, shadow_image_, shadow_memory_, 0);
    if (result != VK_SUCCESS) {
        log_error("Allocate/bind directional shadow image memory failed", result);
        return false;
    }
    account_gpu_allocation(requirements.size);

    if (!shadow_enabled_) {
        VkBuffer staging = VK_NULL_HANDLE;
        VkDeviceMemory staging_memory = VK_NULL_HANDLE;
        std::array<std::uint8_t, 4> one_layer_depth_bytes{};
        std::vector<std::uint8_t> fully_lit_depth_bytes;
        VkDeviceSize fallback_depth_size = 0;
        if (shadow_format_ == VK_FORMAT_D32_SFLOAT) {
            const float fully_lit_depth = 1.0F;
            std::memcpy(one_layer_depth_bytes.data(), &fully_lit_depth, sizeof(fully_lit_depth));
            fallback_depth_size = sizeof(fully_lit_depth);
        } else if (shadow_format_ == VK_FORMAT_D24_UNORM_S8_UINT) {
            const std::uint32_t fully_lit_depth = 0x00ffffffU;
            std::memcpy(one_layer_depth_bytes.data(), &fully_lit_depth, sizeof(fully_lit_depth));
            fallback_depth_size = sizeof(fully_lit_depth);
        } else if (shadow_format_ == VK_FORMAT_D16_UNORM) {
            const std::uint16_t fully_lit_depth = 0xffffU;
            std::memcpy(one_layer_depth_bytes.data(), &fully_lit_depth, sizeof(fully_lit_depth));
            fallback_depth_size = sizeof(fully_lit_depth);
        } else {
            log_error("Unsupported sampled depth fallback format");
            return false;
        }
        fully_lit_depth_bytes.resize(static_cast<std::size_t>(fallback_depth_size) * 4U);
        for (std::uint32_t layer = 0; layer < 4U; ++layer)
            std::memcpy(fully_lit_depth_bytes.data() + static_cast<std::size_t>(layer) * fallback_depth_size,
                        one_layer_depth_bytes.data(), static_cast<std::size_t>(fallback_depth_size));
        if (!create_buffer(fallback_depth_size * 4U, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging, staging_memory))
            return false;
        void* mapped = nullptr;
        result = vkMapMemory(device_, staging_memory, 0, fallback_depth_size * 4U, 0, &mapped);
        if (result == VK_SUCCESS && mapped) {
            std::memcpy(mapped, fully_lit_depth_bytes.data(), static_cast<std::size_t>(fallback_depth_size) * 4U);
            vkUnmapMemory(device_, staging_memory);
        } else {
            vkDestroyBuffer(device_, staging, nullptr);
            vkFreeMemory(device_, staging_memory, nullptr);
            log_error("Map fallback shadow staging buffer failed", result);
            return false;
        }
        VkCommandPool pool = VK_NULL_HANDLE;
        VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        pool_info.queueFamilyIndex = graphics_family_;
        result = vkCreateCommandPool(device_, &pool_info, nullptr, &pool);
        VkCommandBuffer command = VK_NULL_HANDLE;
        if (result == VK_SUCCESS) {
            VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
            command_info.commandPool = pool;
            command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            command_info.commandBufferCount = 1;
            result = vkAllocateCommandBuffers(device_, &command_info, &command);
        }
        if (result == VK_SUCCESS) {
            VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
            begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            result = vkBeginCommandBuffer(command, &begin);
        }
        if (result == VK_SUCCESS) {
            VkImageMemoryBarrier to_transfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            to_transfer.image = shadow_image_;
            to_transfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            to_transfer.subresourceRange.levelCount = 1;
            to_transfer.subresourceRange.layerCount = 4;
            vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0, 0, nullptr, 0, nullptr, 1, &to_transfer);
            VkBufferImageCopy copy{};
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            copy.imageSubresource.layerCount = 4;
            copy.imageExtent = {1, 1, 1};
            vkCmdCopyBufferToImage(command, staging, shadow_image_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            VkImageMemoryBarrier to_shader{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
            to_shader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            to_shader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            to_shader.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            to_shader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            to_shader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            to_shader.image = shadow_image_;
            to_shader.subresourceRange = to_transfer.subresourceRange;
            vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                                 0, nullptr, 0, nullptr, 1, &to_shader);
            result = vkEndCommandBuffer(command);
        }
        if (result == VK_SUCCESS) {
            VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
            submit.commandBufferCount = 1;
            submit.pCommandBuffers = &command;
            result = vkQueueSubmit(graphics_queue_, 1, &submit, VK_NULL_HANDLE);
            if (result == VK_SUCCESS) result = vkQueueWaitIdle(graphics_queue_);
        }
        if (pool) vkDestroyCommandPool(device_, pool, nullptr);
        vkDestroyBuffer(device_, staging, nullptr);
        vkFreeMemory(device_, staging_memory, nullptr);
        if (result != VK_SUCCESS) {
            log_error("Upload fully-lit fallback shadow depth failed", result);
            return false;
        }
    } else {
        VkAttachmentDescription attachment{};
        attachment.format = shadow_format_;
        attachment.samples = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        VkAttachmentReference depth_ref{0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &depth_ref;
        const std::array<VkSubpassDependency, 2> dependencies{{
            {VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
             VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
             VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, 0},
            {0, VK_SUBPASS_EXTERNAL, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, 0}
        }};
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
        pass.attachmentCount = 1;
        pass.pAttachments = &attachment;
        pass.subpassCount = 1;
        pass.pSubpasses = &subpass;
        pass.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        pass.pDependencies = dependencies.data();
        result = vkCreateRenderPass(device_, &pass, nullptr, &shadow_render_pass_);
        if (result != VK_SUCCESS) {
            log_error("Create directional shadow render pass failed", result);
            return false;
        }
    }

    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = shadow_image_;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    view_info.format = shadow_format_;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.layerCount = 4;
    result = vkCreateImageView(device_, &view_info, nullptr, &shadow_view_);
    if (result != VK_SUCCESS) {
        log_error("Create directional shadow depth view failed", result);
        return false;
    }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = VK_FILTER_NEAREST;
    sampler.minFilter = VK_FILTER_NEAREST;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sampler.compareEnable = VK_TRUE;
    sampler.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    sampler.maxLod = 0.0F;
    result = vkCreateSampler(device_, &sampler, nullptr, &shadow_sampler_);
    if (result != VK_SUCCESS) {
        log_error("Create directional shadow compare sampler failed", result);
        return false;
    }

    if (shadow_enabled_) {
        for (std::uint32_t layer = 0; layer < 4U; ++layer) {
            VkImageViewCreateInfo layer_view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
            layer_view.image = shadow_image_;
            layer_view.viewType = VK_IMAGE_VIEW_TYPE_2D;
            layer_view.format = shadow_format_;
            layer_view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            layer_view.subresourceRange.baseArrayLayer = layer;
            layer_view.subresourceRange.layerCount = 1;
            layer_view.subresourceRange.levelCount = 1;
            result = vkCreateImageView(device_, &layer_view, nullptr, &shadow_layer_views_[layer]);
            if (result != VK_SUCCESS) {
                log_error("Create directional shadow layer view failed", result);
                return false;
            }
            VkFramebufferCreateInfo framebuffer{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
            framebuffer.renderPass = shadow_render_pass_;
            framebuffer.attachmentCount = 1;
            framebuffer.pAttachments = &shadow_layer_views_[layer];
            framebuffer.width = shadow_extent_.width;
            framebuffer.height = shadow_extent_.height;
            framebuffer.layers = 1;
            result = vkCreateFramebuffer(device_, &framebuffer, nullptr, &shadow_framebuffers_[layer]);
            if (result != VK_SUCCESS) {
                log_error("Create directional shadow cascade framebuffer failed", result);
                return false;
            }
        }
        char message[160]{};
        std::snprintf(message, sizeof(message), "Directional cascaded shadows enabled: cascades=%u resolution=%ux%u",
                      shadow_cascade_count_, shadow_extent_.width, shadow_extent_.height);
        arut_android_log(ANDROID_LOG_INFO, message);
    }
    return true;
}

bool AndroidRenderer::create_shadow_pipeline() noexcept {
    if (!shadow_enabled_) return true;
    AAsset* asset = AAssetManager_open(assets_, "shaders/mobile_shadow_depth.vert.spv", AASSET_MODE_BUFFER);
    if (!asset) {
        log_error("Missing packaged directional shadow vertex shader");
        return false;
    }
    const off_t length = AAsset_getLength(asset);
    if (length <= 0 || (length % 4) != 0) {
        AAsset_close(asset);
        log_error("Invalid directional shadow SPIR-V size");
        return false;
    }
    std::vector<std::uint32_t> code(static_cast<std::size_t>(length) / sizeof(std::uint32_t));
    const int read = AAsset_read(asset, code.data(), static_cast<std::size_t>(length));
    AAsset_close(asset);
    if (read != length || code.empty() || code.front() != 0x07230203U) {
        log_error("Directional shadow SPIR-V read/magic validation failed");
        return false;
    }
    VkShaderModuleCreateInfo module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = code.size() * sizeof(std::uint32_t);
    module_info.pCode = code.data();
    VkShaderModule module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(device_, &module_info, nullptr, &module);
    if (result != VK_SUCCESS) {
        log_error("Create directional shadow shader module failed", result);
        return false;
    }
    VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    stage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    stage.module = module;
    stage.pName = "main";

    const std::array<VkVertexInputBindingDescription, 2> bindings{{
        {0, sizeof(arut::rendering::MeshVertex), VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof(arut::rendering::Mat4), VK_VERTEX_INPUT_RATE_INSTANCE}
    }};
    const std::array<VkVertexInputAttributeDescription, 7> attributes{{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, position))},
        {4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
        {5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16},
        {6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32},
        {7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48},
        {8, 0, VK_FORMAT_R32G32B32A32_UINT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, joints))},
        {9, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, weights))}
    }};
    VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input.vertexBindingDescriptionCount = static_cast<std::uint32_t>(bindings.size());
    vertex_input.pVertexBindingDescriptions = bindings.data();
    vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    constexpr std::array<VkDynamicState, 2> dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamics.size());
    dynamic.pDynamicStates = dynamics.data();
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_BACK_BIT;
    raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster.depthBiasEnable = VK_TRUE;
    raster.depthBiasConstantFactor = 1.25F;
    raster.depthBiasSlopeFactor = 1.75F;
    raster.lineWidth = 1.0F;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline.stageCount = 1;
    pipeline.pStages = &stage;
    pipeline.pVertexInputState = &vertex_input;
    pipeline.pInputAssemblyState = &assembly;
    pipeline.pViewportState = &viewport;
    pipeline.pRasterizationState = &raster;
    pipeline.pMultisampleState = &multisample;
    pipeline.pDepthStencilState = &depth;
    pipeline.pColorBlendState = &blend;
    pipeline.pDynamicState = &dynamic;
    pipeline.layout = pbr_pipeline_layout_;
    pipeline.renderPass = shadow_render_pass_;
    pipeline.subpass = 0;
    result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline, nullptr, &shadow_pipeline_);
    vkDestroyShaderModule(device_, module, nullptr);
    if (result != VK_SUCCESS) {
        log_error("Create directional shadow graphics pipeline failed", result);
        return false;
    }
    return true;
}

bool AndroidRenderer::load_environment_hdr_asset(
    const char* path, arut::rendering::Rgba32fImage& decoded) noexcept {
    decoded = {};
    if (!assets_ || !path || !*path) return false;
    AAsset* asset = AAssetManager_open(assets_, path, AASSET_MODE_BUFFER);
    if (!asset) {
        arut_android_log(ANDROID_LOG_INFO,
            "Optional HDR environment asset not present; using procedural fallback.");
        return false;
    }
    const off_t length = AAsset_getLength(asset);
    if (length <= 0 || static_cast<std::uint64_t>(length) > 64ULL * 1024ULL * 1024ULL) {
        AAsset_close(asset);
        arut_android_log(ANDROID_LOG_WARN, "HDR environment asset has invalid or excessive size.");
        return false;
    }
    try {
        std::vector<std::uint8_t> encoded(static_cast<std::size_t>(length));
        const int read = AAsset_read(asset, encoded.data(), encoded.size());
        AAsset_close(asset);
        if (read != length) {
            arut_android_log(ANDROID_LOG_WARN, "HDR environment asset read was incomplete.");
            return false;
        }
        const auto result = arut::rendering::decode_image_rgba32f(encoded, decoded);
        if (!result) {
            decoded = {};
            arut_android_log(ANDROID_LOG_WARN,
                "HDR environment decode failed; using procedural environment fallback.");
            return false;
        }
    } catch (...) {
        AAsset_close(asset);
        decoded = {};
        arut_android_log(ANDROID_LOG_WARN, "HDR environment asset allocation failed.");
        return false;
    }
    return true;
}

bool AndroidRenderer::load_environment_probe_cubemap(
    const char* path,
    VkImage& specular_image, VkDeviceMemory& specular_memory,
    VkImageView& specular_view, VkSampler& specular_sampler,
    std::uint32_t& specular_mip_levels,
    VkImage& diffuse_image, VkDeviceMemory& diffuse_memory,
    VkImageView& diffuse_view, VkSampler& diffuse_sampler,
    std::uint32_t& diffuse_mip_levels) noexcept {
    arut::rendering::Rgba32fImage hdr_source{};
    if (!load_environment_hdr_asset(path, hdr_source)) return false;

    VkFormatProperties format_properties{};
    vkGetPhysicalDeviceFormatProperties(
        physical_device_, VK_FORMAT_R16G16B16A16_SFLOAT, &format_properties);
    const VkFormatFeatureFlags required =
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((format_properties.optimalTilingFeatures & required) != required) {
        arut_android_log(ANDROID_LOG_WARN,
            "RGBA16F is unavailable for a local environment probe; retaining global fallback.");
        return false;
    }

    arut::rendering::EnvironmentCubeF32Mip base{};
    std::vector<arut::rendering::EnvironmentCubeF32Mip> specular_float;
    arut::rendering::EnvironmentCubeF32Mip diffuse_float{};
    if (!arut::rendering::build_environment_cube_from_equirectangular(hdr_source, 32U, base) ||
        !arut::rendering::build_ggx_prefiltered_environment_cube(base, 96U, specular_float) ||
        !arut::rendering::build_diffuse_irradiance_cube(base, 8U, 96U, diffuse_float)) {
        log_error("Local HDR environment probe convolution failed; retaining global fallback.");
        return false;
    }

    std::vector<arut::rendering::EnvironmentCubeF16Mip> specular_half;
    arut::rendering::EnvironmentCubeF16Mip diffuse_half{};
    try {
        specular_half.reserve(specular_float.size());
        for (const auto& mip : specular_float) {
            arut::rendering::EnvironmentCubeF16Mip half{};
            if (!arut::rendering::convert_environment_cube_to_rgba16f(mip, half)) {
                log_error("Local HDR probe RGBA16F conversion failed.");
                return false;
            }
            specular_half.push_back(std::move(half));
        }
    } catch (...) {
        log_error("Local HDR probe mip allocation failed.");
        return false;
    }
    if (!arut::rendering::convert_environment_cube_to_rgba16f(diffuse_float, diffuse_half))
        return false;

    const std::vector<arut::rendering::EnvironmentCubeF16Mip> diffuse_mips{
        std::move(diffuse_half)};
    const auto release_partial = [this](VkImage& image, VkDeviceMemory& memory,
                                        VkImageView& view, VkSampler& sampler) {
        if (sampler) vkDestroySampler(device_, sampler, nullptr);
        if (view) vkDestroyImageView(device_, view, nullptr);
        if (image) {
            VkMemoryRequirements requirements{};
            vkGetImageMemoryRequirements(device_, image, &requirements);
            if (requirements.size <= tracked_gpu_memory_bytes_)
                tracked_gpu_memory_bytes_ -= requirements.size;
            vkDestroyImage(device_, image, nullptr);
        }
        if (memory) vkFreeMemory(device_, memory, nullptr);
        image = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        view = VK_NULL_HANDLE;
        sampler = VK_NULL_HANDLE;
    };
    if (!upload_environment_cube_rgba16f(
            specular_half, specular_image, specular_memory, specular_view,
            specular_sampler, specular_mip_levels)) {
        release_partial(specular_image, specular_memory, specular_view, specular_sampler);
        specular_mip_levels = 0U;
        return false;
    }
    if (upload_environment_cube_rgba16f(
            diffuse_mips, diffuse_image, diffuse_memory, diffuse_view,
            diffuse_sampler, diffuse_mip_levels)) return true;

    release_partial(specular_image, specular_memory, specular_view, specular_sampler);
    release_partial(diffuse_image, diffuse_memory, diffuse_view, diffuse_sampler);
    specular_mip_levels = 0U;
    diffuse_mip_levels = 0U;
    return false;
}

bool AndroidRenderer::create_environment_probe_cubemaps() noexcept {
    for (std::uint32_t index = 0; index < arut::rendering::kMaxEnvironmentProbes; ++index) {
        char path[96]{};
        std::snprintf(path, sizeof(path), "environments/probes/probe_%u.hdr", index);
        if (!load_environment_probe_cubemap(
                path,
                environment_probe_specular_images_[index],
                environment_probe_specular_memories_[index],
                environment_probe_specular_views_[index],
                environment_probe_specular_samplers_[index],
                environment_probe_specular_mip_levels_[index],
                environment_probe_diffuse_images_[index],
                environment_probe_diffuse_memories_[index],
                environment_probe_diffuse_views_[index],
                environment_probe_diffuse_samplers_[index],
                environment_probe_diffuse_mip_levels_[index])) {
            // Missing or unsupported local assets deliberately retain the global environment.
            continue;
        }
        char message[160]{};
        std::snprintf(message, sizeof(message),
            "Loaded independent HDR environment probe slot %u.", index);
        arut_android_log(ANDROID_LOG_INFO, message);
    }
    return true;
}

bool AndroidRenderer::create_environment_cubemap() noexcept {
    arut::rendering::Rgba32fImage hdr_source{};
    if (load_environment_hdr_asset("environments/default.hdr", hdr_source)) {
        VkFormatProperties hdr_format_properties{};
        vkGetPhysicalDeviceFormatProperties(
            physical_device_, VK_FORMAT_R16G16B16A16_SFLOAT, &hdr_format_properties);
        const VkFormatFeatureFlags hdr_required =
            VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
        const bool hdr_format_supported =
            (hdr_format_properties.optimalTilingFeatures & hdr_required) == hdr_required;
        if (hdr_format_supported) {
            arut::rendering::EnvironmentCubeF32Mip hdr_base{};
            std::vector<arut::rendering::EnvironmentCubeF32Mip> hdr_specular_float;
            arut::rendering::EnvironmentCubeF32Mip hdr_diffuse_float{};
            bool hdr_ready =
                arut::rendering::build_environment_cube_from_equirectangular(
                    hdr_source, 32U, hdr_base) &&
                arut::rendering::build_ggx_prefiltered_environment_cube(
                    hdr_base, 96U, hdr_specular_float) &&
                arut::rendering::build_diffuse_irradiance_cube(
                    hdr_base, 8U, 96U, hdr_diffuse_float);
            std::vector<arut::rendering::EnvironmentCubeF16Mip> hdr_specular_half;
            arut::rendering::EnvironmentCubeF16Mip hdr_diffuse_half{};
            if (hdr_ready) {
                try {
                    hdr_specular_half.reserve(hdr_specular_float.size());
                    for (const auto& mip : hdr_specular_float) {
                        arut::rendering::EnvironmentCubeF16Mip half{};
                        if (!arut::rendering::convert_environment_cube_to_rgba16f(mip, half)) {
                            hdr_ready = false;
                            break;
                        }
                        hdr_specular_half.push_back(std::move(half));
                    }
                } catch (...) {
                    hdr_ready = false;
                }
                if (hdr_ready) {
                    hdr_ready = arut::rendering::convert_environment_cube_to_rgba16f(
                        hdr_diffuse_float, hdr_diffuse_half);
                }
            }
            if (hdr_ready) {
                VkImage hdr_environment_image = VK_NULL_HANDLE;
                VkDeviceMemory hdr_environment_memory = VK_NULL_HANDLE;
                VkImageView hdr_environment_view = VK_NULL_HANDLE;
                VkSampler hdr_environment_sampler = VK_NULL_HANDLE;
                std::uint32_t hdr_environment_levels = 0U;
                VkImage hdr_irradiance_image = VK_NULL_HANDLE;
                VkDeviceMemory hdr_irradiance_memory = VK_NULL_HANDLE;
                VkImageView hdr_irradiance_view = VK_NULL_HANDLE;
                VkSampler hdr_irradiance_sampler = VK_NULL_HANDLE;
                std::uint32_t hdr_irradiance_levels = 0U;
                const std::vector<arut::rendering::EnvironmentCubeF16Mip> hdr_irradiance_mips{
                    std::move(hdr_diffuse_half)};
                const bool uploaded =
                    upload_environment_cube_rgba16f(
                        hdr_specular_half, hdr_environment_image, hdr_environment_memory,
                        hdr_environment_view, hdr_environment_sampler, hdr_environment_levels) &&
                    upload_environment_cube_rgba16f(
                        hdr_irradiance_mips, hdr_irradiance_image, hdr_irradiance_memory,
                        hdr_irradiance_view, hdr_irradiance_sampler, hdr_irradiance_levels);
                if (uploaded) {
                    environment_image_ = hdr_environment_image;
                    environment_memory_ = hdr_environment_memory;
                    environment_view_ = hdr_environment_view;
                    environment_sampler_ = hdr_environment_sampler;
                    environment_mip_levels_ = hdr_environment_levels;
                    irradiance_image_ = hdr_irradiance_image;
                    irradiance_memory_ = hdr_irradiance_memory;
                    irradiance_view_ = hdr_irradiance_view;
                    irradiance_sampler_ = hdr_irradiance_sampler;
                    irradiance_mip_levels_ = hdr_irradiance_levels;
                    arut_android_log(ANDROID_LOG_INFO,
                        "Radiance HDR environment converted, GGX-prefiltered, convolved and uploaded as RGBA16F.");
                    return true;
                }

                const auto release_cube = [this](VkImage& image, VkDeviceMemory& memory,
                                                  VkImageView& view, VkSampler& sampler) {
                    if (sampler) vkDestroySampler(device_, sampler, nullptr);
                    if (view) vkDestroyImageView(device_, view, nullptr);
                    if (image) {
                        if (memory) {
                            VkMemoryRequirements requirements{};
                            vkGetImageMemoryRequirements(device_, image, &requirements);
                            if (requirements.size <= tracked_gpu_memory_bytes_)
                                tracked_gpu_memory_bytes_ -= requirements.size;
                        }
                        vkDestroyImage(device_, image, nullptr);
                    }
                    if (memory) vkFreeMemory(device_, memory, nullptr);
                    image = VK_NULL_HANDLE;
                    memory = VK_NULL_HANDLE;
                    view = VK_NULL_HANDLE;
                    sampler = VK_NULL_HANDLE;
                };
                release_cube(hdr_environment_image, hdr_environment_memory,
                             hdr_environment_view, hdr_environment_sampler);
                release_cube(hdr_irradiance_image, hdr_irradiance_memory,
                             hdr_irradiance_view, hdr_irradiance_sampler);
            }
        } else {
            arut_android_log(ANDROID_LOG_WARN,
                "RGBA16F sampled cubemaps unsupported; using procedural RGBA8 environment.");
        }
    }

    std::vector<arut::rendering::EnvironmentCubeMip> specular_mips;
    if (!arut::rendering::build_procedural_environment_cube(32U, specular_mips) || specular_mips.empty()) {
        log_error("Procedural environment cubemap generation failed");
        return false;
    }
    arut::rendering::EnvironmentCubeMip diffuse_irradiance{};
    if (!arut::rendering::build_diffuse_irradiance_cube(specular_mips.front(), 8U, 96U, diffuse_irradiance)) {
        log_error("Cosine-convolved diffuse irradiance cubemap generation failed");
        return false;
    }
    const std::vector<arut::rendering::EnvironmentCubeMip> diffuse_mips{std::move(diffuse_irradiance)};
    if (!upload_environment_cube(specular_mips, environment_image_, environment_memory_,
                                 environment_view_, environment_sampler_, environment_mip_levels_)) return false;
    if (!upload_environment_cube(diffuse_mips, irradiance_image_, irradiance_memory_,
                                 irradiance_view_, irradiance_sampler_, irradiance_mip_levels_)) return false;
    arut_android_log(ANDROID_LOG_INFO, "GGX-prefiltered environment and cosine-convolved diffuse irradiance cubemaps uploaded.");
    return true;
}

bool AndroidRenderer::upload_environment_cube(
    const std::vector<arut::rendering::EnvironmentCubeMip>& mips,
    VkImage& out_image, VkDeviceMemory& out_memory, VkImageView& out_view,
    VkSampler& out_sampler, std::uint32_t& out_mip_levels) noexcept {
    std::vector<CubeUploadMip> raw_mips;
    try {
        raw_mips.reserve(mips.size());
        for (const auto& mip : mips)
            raw_mips.push_back({mip.width, mip.height, mip.pixels.data(), mip.pixels.size()});
    } catch (...) {
        return false;
    }
    return upload_environment_cube_raw(raw_mips, VK_FORMAT_R8G8B8A8_UNORM, 4U,
        out_image, out_memory, out_view, out_sampler, out_mip_levels);
}

bool AndroidRenderer::upload_environment_cube_rgba16f(
    const std::vector<arut::rendering::EnvironmentCubeF16Mip>& mips,
    VkImage& out_image, VkDeviceMemory& out_memory, VkImageView& out_view,
    VkSampler& out_sampler, std::uint32_t& out_mip_levels) noexcept {
    std::vector<CubeUploadMip> raw_mips;
    try {
        raw_mips.reserve(mips.size());
        for (const auto& mip : mips)
            raw_mips.push_back({mip.width, mip.height, mip.pixels.data(),
                                mip.pixels.size() * sizeof(std::uint16_t)});
    } catch (...) {
        return false;
    }
    return upload_environment_cube_raw(raw_mips, VK_FORMAT_R16G16B16A16_SFLOAT, 8U,
        out_image, out_memory, out_view, out_sampler, out_mip_levels);
}

bool AndroidRenderer::upload_environment_cube_raw(
    const std::vector<CubeUploadMip>& mips, VkFormat format, VkDeviceSize bytes_per_pixel,
    VkImage& out_image, VkDeviceMemory& out_memory, VkImageView& out_view,
    VkSampler& out_sampler, std::uint32_t& out_mip_levels) noexcept {
    if (mips.empty() || bytes_per_pixel == 0U) return false;
    VkFormatProperties format_properties{};
    vkGetPhysicalDeviceFormatProperties(physical_device_, format, &format_properties);
    const VkFormatFeatureFlags required_features =
        VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT;
    if ((format_properties.optimalTilingFeatures & required_features) != required_features) {
        log_error("Requested cubemap format lacks optimal sampled/transfer-destination support");
        return false;
    }
    const bool linear_filter =
        (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0U;

    VkDeviceSize staging_size = 0;
    for (const auto& mip : mips) {
        if (!mip.pixels || mip.byte_size == 0U ||
            mip.byte_size > std::numeric_limits<VkDeviceSize>::max() - staging_size) return false;
        staging_size += static_cast<VkDeviceSize>(mip.byte_size);
    }
    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    if (!create_buffer(staging_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging, staging_memory)) return false;
    auto release_staging = [&]() {
        if (staging) vkDestroyBuffer(device_, staging, nullptr);
        if (staging_memory) vkFreeMemory(device_, staging_memory, nullptr);
        staging = VK_NULL_HANDLE;
        staging_memory = VK_NULL_HANDLE;
    };
    void* mapped = nullptr;
    VkResult result = vkMapMemory(device_, staging_memory, 0, staging_size, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map environment cubemap staging buffer failed", result);
        release_staging();
        return false;
    }
    std::vector<VkBufferImageCopy> copies;
    try {
        copies.reserve(mips.size() * 6U);
    } catch (...) {
        vkUnmapMemory(device_, staging_memory);
        release_staging();
        return false;
    }
    VkDeviceSize offset = 0;
    for (std::uint32_t level = 0; level < mips.size(); ++level) {
        const auto& mip = mips[level];
        if (mip.width == 0U || mip.height != mip.width ||
            static_cast<VkDeviceSize>(mip.width) >
                std::numeric_limits<VkDeviceSize>::max() / mip.height) {
            vkUnmapMemory(device_, staging_memory);
            release_staging();
            log_error("Invalid environment cubemap dimensions");
            return false;
        }
        const VkDeviceSize face_pixels = static_cast<VkDeviceSize>(mip.width) * mip.height;
        if (face_pixels > std::numeric_limits<VkDeviceSize>::max() / bytes_per_pixel) {
            vkUnmapMemory(device_, staging_memory);
            release_staging();
            return false;
        }
        const VkDeviceSize face_bytes = face_pixels * bytes_per_pixel;
        if (face_bytes > std::numeric_limits<VkDeviceSize>::max() / 6U ||
            mip.byte_size != static_cast<std::size_t>(face_bytes * 6U) ||
            offset > staging_size || static_cast<VkDeviceSize>(mip.byte_size) > staging_size - offset) {
            vkUnmapMemory(device_, staging_memory);
            release_staging();
            log_error("Environment cubemap face/mip byte count mismatch");
            return false;
        }
        std::memcpy(static_cast<std::uint8_t*>(mapped) + offset, mip.pixels, mip.byte_size);
        for (std::uint32_t face = 0; face < 6U; ++face) {
            VkBufferImageCopy copy{};
            copy.bufferOffset = offset + face_bytes * face;
            copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copy.imageSubresource.mipLevel = level;
            copy.imageSubresource.baseArrayLayer = face;
            copy.imageSubresource.layerCount = 1;
            copy.imageExtent = {mip.width, mip.height, 1};
            copies.push_back(copy);
        }
        offset += face_bytes * 6U;
    }
    vkUnmapMemory(device_, staging_memory);

    VkImageCreateInfo image{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    image.imageType = VK_IMAGE_TYPE_2D;
    image.format = format;
    image.extent = {mips[0].width, mips[0].height, 1};
    image.mipLevels = static_cast<std::uint32_t>(mips.size());
    image.arrayLayers = 6;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    result = vkCreateImage(device_, &image, nullptr, &out_image);
    if (result != VK_SUCCESS) {
        log_error("Create environment cubemap image failed", result);
        release_staging();
        return false;
    }
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, out_image, &requirements);
    const std::uint32_t memory_type = find_memory_type(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memory_type == UINT32_MAX) {
        log_error("No device-local memory for environment cubemap");
        release_staging();
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memory_type;
    result = vkAllocateMemory(device_, &allocation, nullptr, &out_memory);
    if (result == VK_SUCCESS) result = vkBindImageMemory(device_, out_image, out_memory, 0);
    if (result != VK_SUCCESS) {
        log_error("Allocate/bind environment cubemap memory failed", result);
        release_staging();
        return false;
    }
    account_gpu_allocation(requirements.size);

    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_info.queueFamilyIndex = graphics_family_;
    result = vkCreateCommandPool(device_, &pool_info, nullptr, &pool);
    VkCommandBuffer command = VK_NULL_HANDLE;
    if (result == VK_SUCCESS) {
        VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        command_info.commandPool = pool;
        command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_info.commandBufferCount = 1;
        result = vkAllocateCommandBuffers(device_, &command_info, &command);
    }
    if (result == VK_SUCCESS) {
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        result = vkBeginCommandBuffer(command, &begin);
    }
    if (result == VK_SUCCESS) {
        VkImageMemoryBarrier to_transfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.image = out_image;
        to_transfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        to_transfer.subresourceRange.levelCount = static_cast<std::uint32_t>(mips.size());
        to_transfer.subresourceRange.layerCount = 6;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &to_transfer);
        vkCmdCopyBufferToImage(command, staging, out_image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<std::uint32_t>(copies.size()), copies.data());
        VkImageMemoryBarrier to_shader{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        to_shader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_shader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_shader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_shader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_shader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_shader.image = out_image;
        to_shader.subresourceRange = to_transfer.subresourceRange;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &to_shader);
        result = vkEndCommandBuffer(command);
    }
    if (result == VK_SUCCESS) {
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        result = vkQueueSubmit(graphics_queue_, 1, &submit, VK_NULL_HANDLE);
        if (result == VK_SUCCESS) result = vkQueueWaitIdle(graphics_queue_);
    }
    if (pool) vkDestroyCommandPool(device_, pool, nullptr);
    release_staging();
    if (result != VK_SUCCESS) {
        log_error("Upload environment cubemap faces/mips failed", result);
        return false;
    }

    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = out_image;
    view.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    view.format = format;
    view.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view.subresourceRange.levelCount = static_cast<std::uint32_t>(mips.size());
    view.subresourceRange.layerCount = 6;
    result = vkCreateImageView(device_, &view, nullptr, &out_view);
    if (result != VK_SUCCESS) {
        log_error("Create environment cubemap view failed", result);
        return false;
    }
    VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler.magFilter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sampler.minFilter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sampler.mipmapMode = linear_filter ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.maxLod = static_cast<float>(mips.size() - 1U);
    result = vkCreateSampler(device_, &sampler, nullptr, &out_sampler);
    if (result != VK_SUCCESS) {
        log_error("Create environment cubemap sampler failed", result);
        return false;
    }
    out_mip_levels = static_cast<std::uint32_t>(mips.size());
    arut_android_log(ANDROID_LOG_INFO, "Environment cubemap uploaded with all face/mip levels.");
    return true;
}

bool AndroidRenderer::create_brdf_lut() noexcept {
    arut::rendering::Rgba8Image lut{};
    if (!arut::rendering::build_brdf_integration_lut(64U, 128U, lut)) {
        log_error("Split-sum BRDF integration LUT generation failed");
        return false;
    }
    if (!upload_texture_rgba8(lut, VK_FORMAT_R8G8B8A8_UNORM, false,
                              brdf_lut_image_, brdf_lut_memory_, brdf_lut_view_,
                              brdf_lut_sampler_, brdf_lut_mip_levels_)) {
        log_error("Split-sum BRDF integration LUT upload failed");
        return false;
    }
    // A BRDF LUT is a parameter lookup, not a roughness-prefiltered image.
    // Keep it pinned to mip zero while retaining bilinear interpolation.
    if (brdf_lut_sampler_) vkDestroySampler(device_, brdf_lut_sampler_, nullptr);
    brdf_lut_sampler_ = VK_NULL_HANDLE;
    VkFormatProperties lut_format_properties{};
    vkGetPhysicalDeviceFormatProperties(physical_device_, VK_FORMAT_R8G8B8A8_UNORM, &lut_format_properties);
    const bool linear_filter =
        (lut_format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0U;
    VkSamplerCreateInfo lut_sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    lut_sampler.magFilter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    lut_sampler.minFilter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    lut_sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    lut_sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    lut_sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    lut_sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    lut_sampler.maxLod = 0.0F;
    const VkResult sampler_result = vkCreateSampler(device_, &lut_sampler, nullptr, &brdf_lut_sampler_);
    if (sampler_result != VK_SUCCESS) {
        log_error("Create BRDF integration LUT sampler failed", sampler_result);
        return false;
    }
    arut_android_log(ANDROID_LOG_INFO, "64x64 split-sum GGX BRDF integration LUT generated and uploaded.");
    return true;
}

bool AndroidRenderer::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                    VkBuffer& buffer, VkDeviceMemory& memory) noexcept {
    if (size == 0 || device_ == VK_NULL_HANDLE) return false;
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkResult result = vkCreateBuffer(device_, &info, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        log_error("vkCreateBuffer failed", result);
        return false;
    }
    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device_, buffer, &requirements);
    const std::uint32_t type = find_memory_type(
        requirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    if (type == UINT32_MAX) {
        vkDestroyBuffer(device_, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        log_error("No host-visible coherent memory type for buffer");
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = type;
    result = vkAllocateMemory(device_, &allocation, nullptr, &memory);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(device_, buffer, nullptr);
        buffer = VK_NULL_HANDLE;
        log_error("vkAllocateMemory for buffer failed", result);
        return false;
    }
    result = vkBindBufferMemory(device_, buffer, memory, 0);
    if (result != VK_SUCCESS) {
        vkFreeMemory(device_, memory, nullptr);
        vkDestroyBuffer(device_, buffer, nullptr);
        memory = VK_NULL_HANDLE;
        buffer = VK_NULL_HANDLE;
        log_error("vkBindBufferMemory failed", result);
        return false;
    }
    if ((usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) == 0U)
        account_gpu_allocation(requirements.size);
    return true;
}

bool AndroidRenderer::create_triangle_geometry() noexcept {
    struct Vertex final { float position[3]; float color[3]; };
    constexpr std::array<Vertex, 3> vertices{{
        {{ 0.0F, -0.65F, 0.0F}, {1.0F, 0.18F, 0.12F}},
        {{ 0.65F, 0.55F, 0.0F}, {0.12F, 0.85F, 0.28F}},
        {{-0.65F, 0.55F, 0.0F}, {0.15F, 0.38F, 1.0F}}
    }};
    const VkDeviceSize bytes = sizeof(vertices);
    if (!create_buffer(bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       triangle_vertex_buffer_, triangle_vertex_memory_)) return false;
    void* mapped = nullptr;
    const VkResult result = vkMapMemory(device_, triangle_vertex_memory_, 0, bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("vkMapMemory for triangle vertices failed", result);
        return false;
    }
    std::memcpy(mapped, vertices.data(), sizeof(vertices));
    vkUnmapMemory(device_, triangle_vertex_memory_);
    return true;
}

bool AndroidRenderer::create_triangle_pipeline() noexcept {
    if (!assets_) return false;
    const auto load_shader = [this](const char* path, std::vector<std::uint32_t>& words) -> bool {
        AAsset* asset = AAssetManager_open(assets_, path, AASSET_MODE_BUFFER);
        if (!asset) {
            char message[256]{};
            std::snprintf(message, sizeof(message), "Missing packaged shader asset: %s", path);
            log_error(message);
            return false;
        }
        const off_t length = AAsset_getLength(asset);
        if (length <= 0 || (length % 4) != 0) {
            AAsset_close(asset);
            log_error("Invalid SPIR-V shader asset size");
            return false;
        }
        words.resize(static_cast<std::size_t>(length) / sizeof(std::uint32_t));
        const int read = AAsset_read(asset, words.data(), static_cast<std::size_t>(length));
        AAsset_close(asset);
        if (read != length || words.empty() || words.front() != 0x07230203U) {
            log_error("SPIR-V shader asset read or magic validation failed");
            return false;
        }
        return true;
    };

    std::vector<std::uint32_t> vertex_code;
    std::vector<std::uint32_t> fragment_code;
    if (!load_shader("shaders/first_triangle.vert.spv", vertex_code) ||
        !load_shader("shaders/first_triangle.frag.spv", fragment_code)) return false;

    VkShaderModuleCreateInfo vertex_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    vertex_info.codeSize = vertex_code.size() * sizeof(std::uint32_t);
    vertex_info.pCode = vertex_code.data();
    VkShaderModule vertex_module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(device_, &vertex_info, nullptr, &vertex_module);
    if (result != VK_SUCCESS) {
        log_error("vkCreateShaderModule vertex failed", result);
        return false;
    }

    VkShaderModuleCreateInfo fragment_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    fragment_info.codeSize = fragment_code.size() * sizeof(std::uint32_t);
    fragment_info.pCode = fragment_code.data();
    VkShaderModule fragment_module = VK_NULL_HANDLE;
    result = vkCreateShaderModule(device_, &fragment_info, nullptr, &fragment_module);
    if (result != VK_SUCCESS) {
        vkDestroyShaderModule(device_, vertex_module, nullptr);
        log_error("vkCreateShaderModule fragment failed", result);
        return false;
    }

    const std::array<VkPipelineShaderStageCreateInfo, 2> stages{{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertex_module, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_module, "main", nullptr}
    }};
    struct Vertex final { float position[3]; float color[3]; };
    VkVertexInputBindingDescription binding{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
    const std::array<VkVertexInputAttributeDescription, 2> attributes{{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<std::uint32_t>(offsetof(Vertex, position))},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<std::uint32_t>(offsetof(Vertex, color))}
    }};
    VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input.vertexBindingDescriptionCount = 1;
    vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    constexpr std::array<VkDynamicState, 2> dynamic_states{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = static_cast<std::uint32_t>(dynamic_states.size());
    dynamic.pDynamicStates = dynamic_states.data();

    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster.lineWidth = 1.0F;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = VK_TRUE;
    depth.depthWriteEnable = VK_TRUE;
    depth.depthCompareOp = VK_COMPARE_OP_LESS;
    VkPipelineColorBlendAttachmentState blend_attachment{};
    blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blend.attachmentCount = 1;
    blend.pAttachments = &blend_attachment;

    VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    result = vkCreatePipelineLayout(device_, &layout, nullptr, &pipeline_layout_);
    if (result != VK_SUCCESS) {
        vkDestroyShaderModule(device_, fragment_module, nullptr);
        vkDestroyShaderModule(device_, vertex_module, nullptr);
        log_error("vkCreatePipelineLayout failed", result);
        return false;
    }

    VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline.stageCount = static_cast<std::uint32_t>(stages.size());
    pipeline.pStages = stages.data();
    pipeline.pVertexInputState = &vertex_input;
    pipeline.pInputAssemblyState = &assembly;
    pipeline.pViewportState = &viewport;
    pipeline.pRasterizationState = &raster;
    pipeline.pMultisampleState = &multisample;
    pipeline.pDepthStencilState = &depth;
    pipeline.pColorBlendState = &blend;
    pipeline.pDynamicState = &dynamic;
    pipeline.layout = pipeline_layout_;
    pipeline.renderPass = render_pass_;
    pipeline.subpass = 0;
    result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline, nullptr, &graphics_pipeline_);
    vkDestroyShaderModule(device_, fragment_module, nullptr);
    vkDestroyShaderModule(device_, vertex_module, nullptr);
    if (result != VK_SUCCESS) {
        log_error("vkCreateGraphicsPipelines for first triangle failed", result);
        return false;
    }
    arut_android_log(ANDROID_LOG_INFO, "First-triangle Vulkan graphics pipeline created.");
    return true;
}


bool AndroidRenderer::create_static_scene_geometry() noexcept {
    static_scene_instance_count_ =
        static_cast<std::uint32_t>(static_scene_instances_.size());
    if (static_scene_instance_count_ == 0U) return true;
    const VkDeviceSize bytes =
        static_cast<VkDeviceSize>(static_scene_instances_.size() * sizeof(arut::rendering::Mat4));
    if (!create_buffer(bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       static_scene_instance_buffer_, static_scene_instance_memory_)) return false;
    void* mapped = nullptr;
    const VkResult result = vkMapMemory(device_, static_scene_instance_memory_,
        0, bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map static test-scene instance buffer failed", result);
        return false;
    }
    std::memcpy(mapped, static_scene_instances_.data(), static_cast<std::size_t>(bytes));
    vkUnmapMemory(device_, static_scene_instance_memory_);
    char message[160]{};
    std::snprintf(message, sizeof(message),
        "TEST_ARENA_RENDER_INSTANCES ready=true count=%u", static_scene_instance_count_);
    arut_android_log(ANDROID_LOG_INFO, message);
    return true;
}

bool AndroidRenderer::create_mannequin_geometry() noexcept {
    const auto mesh = arut::rendering::make_robot_mannequin_mesh();
    if (mesh.vertices.empty() || mesh.indices.empty() ||
        mesh.indices.size() > UINT32_MAX) {
        log_error("Procedural robot mannequin mesh generation failed");
        return false;
    }
    const VkDeviceSize vertex_bytes =
        static_cast<VkDeviceSize>(mesh.vertices.size() * sizeof(arut::rendering::MeshVertex));
    const VkDeviceSize index_bytes =
        static_cast<VkDeviceSize>(mesh.indices.size() * sizeof(std::uint32_t));
    if (!create_buffer(vertex_bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       mannequin_vertex_buffer_, mannequin_vertex_memory_) ||
        !create_buffer(index_bytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       mannequin_index_buffer_, mannequin_index_memory_)) return false;

    void* mapped = nullptr;
    VkResult result = vkMapMemory(device_, mannequin_vertex_memory_, 0, vertex_bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map robot mannequin vertex buffer failed", result);
        return false;
    }
    std::memcpy(mapped, mesh.vertices.data(), static_cast<std::size_t>(vertex_bytes));
    vkUnmapMemory(device_, mannequin_vertex_memory_);

    mapped = nullptr;
    result = vkMapMemory(device_, mannequin_index_memory_, 0, index_bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map robot mannequin index buffer failed", result);
        return false;
    }
    std::memcpy(mapped, mesh.indices.data(), static_cast<std::size_t>(index_bytes));
    vkUnmapMemory(device_, mannequin_index_memory_);

    arut::rendering::Mat4 model = arut::rendering::Mat4::identity();
    constexpr float scale = 1.0F;
    model.value[0] = scale;
    model.value[5] = scale;
    model.value[10] = scale;
    // In the combined profile, place the mannequin between the stress grid's
    // regular two-metre X/Z rows so it remains visible without hiding cube rows.
    model.value[12] = 0.0F;
    model.value[13] = 0.0F;
    model.value[14] = visibility_source_instance_count_ == 0U ? 0.0F : -5.0F;
    if (!create_buffer(sizeof(model), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       mannequin_instance_buffer_, mannequin_instance_memory_)) return false;
    mapped = nullptr;
    result = vkMapMemory(device_, mannequin_instance_memory_, 0, sizeof(model), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map robot mannequin instance transform failed", result);
        return false;
    }
    std::memcpy(mapped, &model, sizeof(model));
    vkUnmapMemory(device_, mannequin_instance_memory_);

    mannequin_index_count_ = static_cast<std::uint32_t>(mesh.indices.size());
    char message[192]{};
    std::snprintf(message, sizeof(message),
        "HUMANOID_RENDERER mesh_ready=true bones=%zu vertices=%zu indices=%u draw_instances=1",
        arut::core::kHumanoidBoneCount, mesh.vertices.size(), mannequin_index_count_);
    arut_android_log(ANDROID_LOG_INFO, message);
    return true;
}

bool AndroidRenderer::create_pbr_geometry() noexcept {
    const auto mesh = arut::rendering::make_unit_cube();
    if (mesh.vertices.empty() || mesh.indices.empty()) return false;
    const VkDeviceSize vertex_bytes = mesh.vertices.size() * sizeof(arut::rendering::MeshVertex);
    const VkDeviceSize index_bytes = mesh.indices.size() * sizeof(std::uint32_t);
    if (!create_buffer(vertex_bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, cube_vertex_buffer_, cube_vertex_memory_) ||
        !create_buffer(index_bytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, cube_index_buffer_, cube_index_memory_)) return false;
    void* mapped = nullptr;
    VkResult result = vkMapMemory(device_, cube_vertex_memory_, 0, vertex_bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) { log_error("Map PBR cube vertex buffer failed", result); return false; }
    std::memcpy(mapped, mesh.vertices.data(), static_cast<std::size_t>(vertex_bytes));
    vkUnmapMemory(device_, cube_vertex_memory_);
    mapped = nullptr;
    result = vkMapMemory(device_, cube_index_memory_, 0, index_bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) { log_error("Map PBR cube index buffer failed", result); return false; }
    std::memcpy(mapped, mesh.indices.data(), static_cast<std::size_t>(index_bytes));
    vkUnmapMemory(device_, cube_index_memory_);
    cube_index_count_ = static_cast<std::uint32_t>(mesh.indices.size());

    // Instance transforms and bounds are supplied by the ECS render-extraction system.
    // The renderer only uploads/culls/draws this immutable input; it never creates entities.
    visibility_source_instance_count_ = static_cast<std::uint32_t>(visibility_instances_.size());
    if (visibility_source_instance_count_ != 0U &&
        visibility_source_instance_count_ != arut::core::CubeSpawnSystem::target_count) {
        log_error("Stress scene input invalid: expected zero or 5000 ECS-owned cube instances");
        return false;
    }
    const bool humanoid_only =
        mannequin_scene_enabled_ && visibility_source_instance_count_ == 0U;
    const std::array<float, 3> camera = humanoid_only
        ? std::array<float, 3>{0.0F, 1.1F, 7.0F}
        : std::array<float, 3>{0.0F, 0.0F, 18.0F};
    camera_position_ = camera;
    camera_yaw_ = -1.570796327F;
    camera_pitch_ = 0.0F;
    const std::array<float, 3> camera_target = humanoid_only
        ? std::array<float, 3>{0.0F, 1.1F, 6.0F}
        : std::array<float, 3>{0.0F, 0.0F, 17.0F};
    const auto view = arut::rendering::look_at_rh(camera, camera_target, {0.0F, 1.0F, 0.0F});
    const auto projection = arut::rendering::perspective_rh_zo(0.785398163F,
        static_cast<float>(extent_.width) / static_cast<float>(std::max(1U, extent_.height)), 0.1F, 180.0F);
    const auto view_projection = arut::rendering::multiply(projection, view);
    const auto frustum = arut::rendering::extract_vulkan_frustum(view_projection);
    std::vector<arut::rendering::Mat4> cpu_visible_instances;
    cpu_visible_instances.reserve(visibility_instances_.size());
    if (!arut::rendering::compact_visible_instances_cpu(frustum, visibility_instances_, cpu_visible_instances)) {
        log_error("CPU visibility reference compaction failed");
        return false;
    }
    if (visibility_source_instance_count_ != 0U && cpu_visible_instances.empty()) {
        log_error("CPU visibility reference removed every stress-scene instance");
        return false;
    }
    for (std::size_t plane_index = 0; plane_index < frustum.planes.size(); ++plane_index) {
        const auto& plane = frustum.planes[plane_index];
        visibility_frustum_planes_[plane_index * 4U + 0U] = plane.normal[0];
        visibility_frustum_planes_[plane_index * 4U + 1U] = plane.normal[1];
        visibility_frustum_planes_[plane_index * 4U + 2U] = plane.normal[2];
        visibility_frustum_planes_[plane_index * 4U + 3U] = plane.distance;
    }
    const VkDeviceSize input_bytes = visibility_instances_.size() * sizeof(arut::rendering::GpuVisibilityInstance);
    // Humanoid-only profiles intentionally have no ECS visibility input.
    // Avoid a zero-byte Vulkan allocation and preserve queue capability state.
    if (compute_queue_supported_ && visibility_source_instance_count_ != 0U) {
        if (!create_buffer(input_bytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           visibility_input_buffer_, visibility_input_memory_)) {
            compute_queue_supported_ = false;
            arut_android_log(ANDROID_LOG_WARN, "GPU visibility input storage allocation failed; using CPU culling.");
        } else {
            mapped = nullptr;
            result = vkMapMemory(device_, visibility_input_memory_, 0, input_bytes, 0, &mapped);
            if (result != VK_SUCCESS || !mapped) {
                log_error("Map GPU visibility input buffer failed; using CPU culling", result);
                vkDestroyBuffer(device_, visibility_input_buffer_, nullptr);
                vkFreeMemory(device_, visibility_input_memory_, nullptr);
                visibility_input_buffer_ = VK_NULL_HANDLE;
                visibility_input_memory_ = VK_NULL_HANDLE;
                compute_queue_supported_ = false;
            } else {
                std::memcpy(mapped, visibility_instances_.data(), static_cast<std::size_t>(input_bytes));
                vkUnmapMemory(device_, visibility_input_memory_);
            }
        }
    }

    const VkDeviceSize instance_capacity_bytes = std::max<VkDeviceSize>(
        sizeof(arut::rendering::Mat4),
        visibility_instances_.size() * sizeof(arut::rendering::Mat4));
    const VkBufferUsageFlags instance_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        (compute_queue_supported_ ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0U);
    if (!create_buffer(instance_capacity_bytes, instance_usage,
                       instance_buffer_, instance_memory_)) return false;
    mapped = nullptr;
    result = vkMapMemory(device_, instance_memory_, 0, instance_capacity_bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map PBR instance output buffer failed", result);
        return false;
    }
    std::memset(mapped, 0, static_cast<std::size_t>(instance_capacity_bytes));
    if (!cpu_visible_instances.empty()) {
        std::memcpy(mapped, cpu_visible_instances.data(),
                    cpu_visible_instances.size() * sizeof(arut::rendering::Mat4));
    }
    vkUnmapMemory(device_, instance_memory_);
    instance_count_ = static_cast<std::uint32_t>(cpu_visible_instances.size());

    const arut::rendering::IndexedIndirectArguments indirect_command{
        cube_index_count_, instance_count_, 0U, 0, 0U
    };
    const VkBufferUsageFlags indirect_usage = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT |
        (compute_queue_supported_ ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0U);
    if (!create_buffer(sizeof(indirect_command), indirect_usage,
                       indirect_draw_buffer_, indirect_draw_memory_)) return false;
    mapped = nullptr;
    result = vkMapMemory(device_, indirect_draw_memory_, 0, sizeof(indirect_command), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map indexed indirect command buffer failed", result);
        return false;
    }
    std::memcpy(mapped, &indirect_command, sizeof(indirect_command));
    vkUnmapMemory(device_, indirect_draw_memory_);

    char message[192]{};
    if (visibility_source_instance_count_ != 0U) {
        std::snprintf(message, sizeof(message),
                      "STRESS SCENE: source=ECS vertices=%zu indices=%u source_instances=%u CPU_visible=%u culled=%u grid=20x10x25",
                      mesh.vertices.size(), cube_index_count_, visibility_source_instance_count_,
                      instance_count_, visibility_source_instance_count_ - instance_count_);
    } else {
        std::snprintf(message, sizeof(message),
                      "STRESS SCENE disabled=true reason=humanoid_only_profile indirect_instances=0");
    }
    arut_android_log(ANDROID_LOG_INFO, message);
    return true;
}

bool AndroidRenderer::create_terrain_geometry() noexcept {
    if (terrain_meshes_.empty()) return false;
    terrain_triangle_count_ = 0U;
    try { terrain_gpu_meshes_.reserve(terrain_meshes_.size()); }
    catch (...) { log_error("Terrain GPU mesh list allocation failed"); return false; }
    for (const auto& terrain : terrain_meshes_) {
        if (!terrain.valid()) return false;
        std::vector<arut::rendering::MeshVertex> vertices;
        try {
            vertices.reserve(terrain.vertices.size());
            for (const auto& source : terrain.vertices) {
                arut::rendering::MeshVertex vertex{};
                vertex.position = {source.x, source.y, source.z};
                vertex.normal = {source.nx, source.ny, source.nz};
                vertex.tangent = {1.0F, 0.0F, 0.0F, 1.0F};
                vertex.uv = {source.x / terrain.world_size + 0.5F, source.z / terrain.world_size + 0.5F};
                vertices.push_back(vertex);
            }
        } catch (...) { log_error("Terrain vertex conversion allocation failed"); return false; }
        TerrainGpuMesh gpu{};
        const std::uint64_t allocation_before = tracked_gpu_memory_bytes_;
        const VkDeviceSize vertex_bytes = vertices.size() * sizeof(arut::rendering::MeshVertex);
        const VkDeviceSize index_bytes = terrain.indices.size() * sizeof(std::uint32_t);
        if (vertex_bytes == 0 || index_bytes == 0 ||
            !create_buffer(vertex_bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, gpu.vertex_buffer, gpu.vertex_memory) ||
            !create_buffer(index_bytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, gpu.index_buffer, gpu.index_memory) ||
            !create_buffer(sizeof(arut::rendering::Mat4), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           gpu.instance_buffer, gpu.instance_memory)) {
            log_error("Terrain cell GPU buffer allocation failed");
            if (gpu.vertex_buffer) vkDestroyBuffer(device_, gpu.vertex_buffer, nullptr);
            if (gpu.vertex_memory) vkFreeMemory(device_, gpu.vertex_memory, nullptr);
            if (gpu.index_buffer) vkDestroyBuffer(device_, gpu.index_buffer, nullptr);
            if (gpu.index_memory) vkFreeMemory(device_, gpu.index_memory, nullptr);
            if (gpu.instance_buffer) vkDestroyBuffer(device_, gpu.instance_buffer, nullptr);
            if (gpu.instance_memory) vkFreeMemory(device_, gpu.instance_memory, nullptr);
            return false;
        }
        void* mapped = nullptr;
        VkResult result = vkMapMemory(device_, gpu.vertex_memory, 0, vertex_bytes, 0, &mapped);
        if (result != VK_SUCCESS || !mapped) { log_error("Map terrain cell vertex buffer failed", result); return false; }
        std::memcpy(mapped, vertices.data(), static_cast<std::size_t>(vertex_bytes));
        vkUnmapMemory(device_, gpu.vertex_memory);
        mapped = nullptr;
        result = vkMapMemory(device_, gpu.index_memory, 0, index_bytes, 0, &mapped);
        if (result != VK_SUCCESS || !mapped) { log_error("Map terrain cell index buffer failed", result); return false; }
        std::memcpy(mapped, terrain.indices.data(), static_cast<std::size_t>(index_bytes));
        vkUnmapMemory(device_, gpu.index_memory);
        arut::rendering::Mat4 model = arut::rendering::Mat4::identity();
        model.value[13] = -4.5F;
        mapped = nullptr;
        result = vkMapMemory(device_, gpu.instance_memory, 0, sizeof(model), 0, &mapped);
        if (result != VK_SUCCESS || !mapped) { log_error("Map terrain cell transform buffer failed", result); return false; }
        std::memcpy(mapped, &model, sizeof(model));
        vkUnmapMemory(device_, gpu.instance_memory);
        gpu.index_count = static_cast<std::uint32_t>(terrain.indices.size());
        gpu.allocation_bytes = tracked_gpu_memory_bytes_ - allocation_before;
        terrain_triangle_count_ += gpu.index_count / 3U;
        terrain_gpu_meshes_.push_back(gpu);
    }
    char message[192]{};
    std::snprintf(message, sizeof(message), "TERRAIN_GPU cells=%zu uploaded=true triangles=%u",
                  terrain_gpu_meshes_.size(), terrain_triangle_count_);
    arut_android_log(ANDROID_LOG_INFO, message);
    return true;
}

bool AndroidRenderer::upload_texture_rgba8(
    const arut::rendering::Rgba8Image& pixels, VkFormat format, bool source_is_srgb,
    VkImage& image, VkDeviceMemory& memory, VkImageView& view, VkSampler& sampler,
    std::uint32_t& mip_levels) noexcept {
    if (!pixels.valid()) return false;
    VkFormatProperties format_properties{};
    vkGetPhysicalDeviceFormatProperties(physical_device_, format, &format_properties);
    if ((format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) == 0U) {
        log_error("Texture format does not support optimal sampled images");
        return false;
    }

    std::vector<arut::rendering::Rgba8Mip> mip_chain;
    if (!arut::rendering::build_rgba8_mip_chain(pixels, source_is_srgb, mip_chain)) {
        log_error("CPU mip-chain generation failed");
        return false;
    }
    VkDeviceSize staging_size = 0;
    for (const auto& level : mip_chain) {
        const VkDeviceSize bytes = static_cast<VkDeviceSize>(level.pixels.size());
        if (bytes > std::numeric_limits<VkDeviceSize>::max() - staging_size) return false;
        staging_size += bytes;
    }
    if (staging_size == 0 || mip_chain.size() > std::numeric_limits<std::uint32_t>::max()) return false;

    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory staging_memory = VK_NULL_HANDLE;
    auto release_staging = [&]() {
        if (staging) vkDestroyBuffer(device_, staging, nullptr);
        if (staging_memory) vkFreeMemory(device_, staging_memory, nullptr);
        staging = VK_NULL_HANDLE;
        staging_memory = VK_NULL_HANDLE;
    };
    if (!create_buffer(staging_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, staging, staging_memory)) {
        release_staging();
        return false;
    }
    void* mapped = nullptr;
    VkResult result = vkMapMemory(device_, staging_memory, 0, staging_size, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map texture staging buffer failed", result);
        release_staging();
        return false;
    }
    std::vector<VkBufferImageCopy> copies;
    copies.reserve(mip_chain.size());
    VkDeviceSize offset = 0;
    for (std::uint32_t level = 0; level < mip_chain.size(); ++level) {
        const auto& mip = mip_chain[level];
        std::memcpy(static_cast<std::uint8_t*>(mapped) + offset,
                    mip.pixels.data(), mip.pixels.size());
        VkBufferImageCopy copy{};
        copy.bufferOffset = offset;
        copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copy.imageSubresource.mipLevel = level;
        copy.imageSubresource.layerCount = 1;
        copy.imageExtent = {mip.width, mip.height, 1};
        copies.push_back(copy);
        offset += static_cast<VkDeviceSize>(mip.pixels.size());
    }
    vkUnmapMemory(device_, staging_memory);

    VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = format;
    image_info.extent = {pixels.width, pixels.height, 1};
    image_info.mipLevels = static_cast<std::uint32_t>(mip_chain.size());
    image_info.arrayLayers = 1;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    result = vkCreateImage(device_, &image_info, nullptr, &image);
    if (result != VK_SUCCESS) {
        log_error("Create sampled texture image failed", result);
        release_staging();
        return false;
    }
    auto release_texture = [&]() {
        if (sampler) vkDestroySampler(device_, sampler, nullptr);
        if (view) vkDestroyImageView(device_, view, nullptr);
        if (image) vkDestroyImage(device_, image, nullptr);
        if (memory) vkFreeMemory(device_, memory, nullptr);
        sampler = VK_NULL_HANDLE;
        view = VK_NULL_HANDLE;
        image = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
        mip_levels = 0;
    };
    VkMemoryRequirements requirements{};
    vkGetImageMemoryRequirements(device_, image, &requirements);
    const std::uint32_t memory_type = find_memory_type(
        requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (memory_type == UINT32_MAX) {
        log_error("No device-local memory for sampled texture");
        release_texture();
        release_staging();
        return false;
    }
    VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocation.allocationSize = requirements.size;
    allocation.memoryTypeIndex = memory_type;
    result = vkAllocateMemory(device_, &allocation, nullptr, &memory);
    if (result == VK_SUCCESS) result = vkBindImageMemory(device_, image, memory, 0);
    if (result != VK_SUCCESS) {
        log_error("Allocate/bind sampled texture memory failed", result);
        release_texture();
        release_staging();
        return false;
    }
    account_gpu_allocation(requirements.size);

    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    pool_info.queueFamilyIndex = graphics_family_;
    result = vkCreateCommandPool(device_, &pool_info, nullptr, &pool);
    VkCommandBuffer command = VK_NULL_HANDLE;
    if (result == VK_SUCCESS) {
        VkCommandBufferAllocateInfo command_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        command_info.commandPool = pool;
        command_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_info.commandBufferCount = 1;
        result = vkAllocateCommandBuffers(device_, &command_info, &command);
    }
    if (result == VK_SUCCESS) {
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        result = vkBeginCommandBuffer(command, &begin);
    }
    if (result == VK_SUCCESS) {
        VkImageMemoryBarrier to_transfer{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_transfer.image = image;
        to_transfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        to_transfer.subresourceRange.levelCount = static_cast<std::uint32_t>(mip_chain.size());
        to_transfer.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &to_transfer);
        vkCmdCopyBufferToImage(command, staging, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<std::uint32_t>(copies.size()), copies.data());
        VkImageMemoryBarrier to_shader{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
        to_shader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        to_shader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        to_shader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        to_shader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_shader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        to_shader.image = image;
        to_shader.subresourceRange = to_transfer.subresourceRange;
        vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &to_shader);
        result = vkEndCommandBuffer(command);
    }
    if (result == VK_SUCCESS) {
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1;
        submit.pCommandBuffers = &command;
        result = vkQueueSubmit(graphics_queue_, 1, &submit, VK_NULL_HANDLE);
        if (result == VK_SUCCESS) result = vkQueueWaitIdle(graphics_queue_);
    }
    if (pool) vkDestroyCommandPool(device_, pool, nullptr);
    release_staging();
    if (result != VK_SUCCESS) {
        log_error("Upload sampled texture mip chain failed", result);
        release_texture();
        return false;
    }

    VkImageViewCreateInfo view_info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.levelCount = static_cast<std::uint32_t>(mip_chain.size());
    view_info.subresourceRange.layerCount = 1;
    result = vkCreateImageView(device_, &view_info, nullptr, &view);
    if (result != VK_SUCCESS) {
        log_error("Create sampled texture view failed", result);
        release_texture();
        return false;
    }

    const bool linear_filter =
        (format_properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) != 0U;
    VkSamplerCreateInfo sampler_info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    sampler_info.magFilter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sampler_info.minFilter = linear_filter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    sampler_info.mipmapMode = linear_filter ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.maxLod = static_cast<float>(mip_chain.size() - 1U);
    result = vkCreateSampler(device_, &sampler_info, nullptr, &sampler);
    if (result != VK_SUCCESS) {
        log_error("Create sampled texture sampler failed", result);
        release_texture();
        return false;
    }
    mip_levels = static_cast<std::uint32_t>(mip_chain.size());
    return true;
}

bool AndroidRenderer::load_texture_asset(
    const char* path, VkFormat format, bool source_is_srgb,
    VkImage& image, VkDeviceMemory& memory, VkImageView& view, VkSampler& sampler,
    std::uint32_t& mip_levels) noexcept {
    if (!assets_ || !path || !*path) return false;
    AAsset* asset = AAssetManager_open(assets_, path, AASSET_MODE_BUFFER);
    if (!asset) {
        char message[256]{};
        std::snprintf(message, sizeof(message), "Texture asset not found: %s", path);
        log_error(message);
        return false;
    }
    const off_t length = AAsset_getLength(asset);
    if (length <= 0 || static_cast<std::uint64_t>(length) > 64ULL * 1024ULL * 1024ULL) {
        AAsset_close(asset);
        log_error("Texture asset has invalid or excessive size");
        return false;
    }
    std::vector<std::uint8_t> encoded(static_cast<std::size_t>(length));
    const int read = AAsset_read(asset, encoded.data(), encoded.size());
    AAsset_close(asset);
    if (read != length) {
        log_error("Texture asset read was incomplete");
        return false;
    }
    arut::rendering::Rgba8Image decoded{};
    const auto decoded_result = arut::rendering::decode_image_rgba8(encoded, decoded);
    if (!decoded_result) {
        log_error("Texture image decode failed (unsupported, corrupt or over budget)");
        return false;
    }
    return upload_texture_rgba8(decoded, format, source_is_srgb,
                                image, memory, view, sampler, mip_levels);
}

bool AndroidRenderer::create_fallback_texture() noexcept {
    arut::rendering::Rgba8Image white{};
    white.width = 1;
    white.height = 1;
    white.pixels = {255U, 255U, 255U, 255U};
    return upload_texture_rgba8(white, VK_FORMAT_R8G8B8A8_UNORM, false,
                               fallback_image_, fallback_memory_, fallback_view_,
                               fallback_sampler_, fallback_mip_levels_);
}

bool AndroidRenderer::create_pbr_pipeline() noexcept {
    const auto load_shader = [this](const char* path, std::vector<std::uint32_t>& words) -> bool {
        AAsset* asset = AAssetManager_open(assets_, path, AASSET_MODE_BUFFER);
        if (!asset) { char message[256]{}; std::snprintf(message, sizeof(message), "Missing PBR shader asset: %s", path); log_error(message); return false; }
        const off_t length = AAsset_getLength(asset);
        if (length <= 0 || (length % 4) != 0) { AAsset_close(asset); log_error("Invalid PBR SPIR-V asset size"); return false; }
        words.resize(static_cast<std::size_t>(length) / sizeof(std::uint32_t));
        const int read = AAsset_read(asset, words.data(), static_cast<std::size_t>(length));
        AAsset_close(asset);
        if (read != length || words.empty() || words.front() != 0x07230203U) { log_error("PBR SPIR-V read or magic validation failed"); return false; }
        return true;
    };
    std::vector<std::uint32_t> vertex_code, fragment_code;
    if (!load_shader("shaders/mobile_pbr_forward.vert.spv", vertex_code) ||
        !load_shader("shaders/mobile_pbr_forward.frag.spv", fragment_code)) return false;
    VkShaderModuleCreateInfo module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = vertex_code.size() * sizeof(std::uint32_t);
    module_info.pCode = vertex_code.data();
    VkShaderModule vertex_module = VK_NULL_HANDLE, fragment_module = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(device_, &module_info, nullptr, &vertex_module);
    if (result != VK_SUCCESS) { log_error("Create PBR vertex shader module failed", result); return false; }
    module_info.codeSize = fragment_code.size() * sizeof(std::uint32_t);
    module_info.pCode = fragment_code.data();
    result = vkCreateShaderModule(device_, &module_info, nullptr, &fragment_module);
    if (result != VK_SUCCESS) { vkDestroyShaderModule(device_, vertex_module, nullptr); log_error("Create PBR fragment shader module failed", result); return false; }

    const std::array<VkDescriptorSetLayoutBinding, 9> frame_bindings{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {7, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {8, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
    }};
    VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set_info.bindingCount = static_cast<std::uint32_t>(frame_bindings.size());
    set_info.pBindings = frame_bindings.data();
    result = vkCreateDescriptorSetLayout(device_, &set_info, nullptr, &frame_set_layout_);
    if (result != VK_SUCCESS) { vkDestroyShaderModule(device_, fragment_module, nullptr); vkDestroyShaderModule(device_, vertex_module, nullptr); log_error("Create frame descriptor layout failed", result); return false; }

    const std::array<VkDescriptorSetLayoutBinding, 4> material_bindings{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    }};
    set_info.bindingCount = static_cast<std::uint32_t>(material_bindings.size());
    set_info.pBindings = material_bindings.data();
    result = vkCreateDescriptorSetLayout(device_, &set_info, nullptr, &material_set_layout_);
    if (result != VK_SUCCESS) { vkDestroyShaderModule(device_, fragment_module, nullptr); vkDestroyShaderModule(device_, vertex_module, nullptr); log_error("Create material descriptor layout failed", result); return false; }

    const std::array<VkDescriptorSetLayout, 2> layouts{frame_set_layout_, material_set_layout_};
    VkPushConstantRange shadow_cascade_push{};
    shadow_cascade_push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    shadow_cascade_push.offset = 0;
    shadow_cascade_push.size = sizeof(std::uint32_t);
    VkPipelineLayoutCreateInfo pipeline_layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
    pipeline_layout.pSetLayouts = layouts.data();
    pipeline_layout.pushConstantRangeCount = 1;
    pipeline_layout.pPushConstantRanges = &shadow_cascade_push;
    result = vkCreatePipelineLayout(device_, &pipeline_layout, nullptr, &pbr_pipeline_layout_);
    if (result != VK_SUCCESS) { vkDestroyShaderModule(device_, fragment_module, nullptr); vkDestroyShaderModule(device_, vertex_module, nullptr); log_error("Create PBR pipeline layout failed", result); return false; }

    const std::array<VkPipelineShaderStageCreateInfo, 2> stages{{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertex_module, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_module, "main", nullptr}
    }};
    const std::array<VkVertexInputBindingDescription, 2> bindings{{
        {0, sizeof(arut::rendering::MeshVertex), VK_VERTEX_INPUT_RATE_VERTEX},
        {1, sizeof(arut::rendering::Mat4), VK_VERTEX_INPUT_RATE_INSTANCE}
    }};
    const std::array<VkVertexInputAttributeDescription, 10> attributes{{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, position))},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, normal))},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, tangent))},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, uv))},
        {4, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 0},
        {5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 16},
        {6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 32},
        {7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, 48},
        {8, 0, VK_FORMAT_R32G32B32A32_UINT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, joints))},
        {9, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, weights))}
    }};
    VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input.vertexBindingDescriptionCount = static_cast<std::uint32_t>(bindings.size());
    vertex_input.pVertexBindingDescriptions = bindings.data();
    vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size());
    vertex_input.pVertexAttributeDescriptions = attributes.data();
    VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO}; viewport.viewportCount = 1; viewport.scissorCount = 1;
    constexpr std::array<VkDynamicState, 2> dynamics{VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO}; dynamic.dynamicStateCount = 2; dynamic.pDynamicStates = dynamics.data();
    VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL; raster.cullMode = VK_CULL_MODE_BACK_BIT; raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; raster.lineWidth = 1.0F;
    VkPipelineMultisampleStateCreateInfo multisample{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO}; multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    depth.depthTestEnable = VK_TRUE; depth.depthWriteEnable = VK_TRUE; depth.depthCompareOp = VK_COMPARE_OP_LESS;
    VkPipelineColorBlendAttachmentState blend_attachment{}; blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blend{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO}; blend.attachmentCount = 1; blend.pAttachments = &blend_attachment;
    VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipeline.stageCount = 2; pipeline.pStages = stages.data(); pipeline.pVertexInputState = &vertex_input; pipeline.pInputAssemblyState = &assembly;
    pipeline.pViewportState = &viewport; pipeline.pRasterizationState = &raster; pipeline.pMultisampleState = &multisample;
    pipeline.pDepthStencilState = &depth; pipeline.pColorBlendState = &blend; pipeline.pDynamicState = &dynamic;
    pipeline.layout = pbr_pipeline_layout_; pipeline.renderPass = render_pass_; pipeline.subpass = 0;
    result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipeline, nullptr, &pbr_pipeline_);
    vkDestroyShaderModule(device_, fragment_module, nullptr);
    vkDestroyShaderModule(device_, vertex_module, nullptr);
    if (result != VK_SUCCESS) { log_error("Create mobile PBR graphics pipeline failed", result); return false; }
    arut_android_log(ANDROID_LOG_INFO, "Mobile forward PBR Vulkan pipeline created.");
    return true;
}

bool AndroidRenderer::create_pbr_descriptors() noexcept {
    struct alignas(16) FrameData final {
        float view_projection[16];
        float camera_position[4];
        float camera_forward[4];
        float light_direction[4];
        float light_radiance[4];
        float light_view_projection[4][16];
        float cascade_splits[4];
    };
    struct alignas(16) MaterialData final {
        float base_color[4];
        float emissive_metallic[4];
        float roughness_normal_occlusion_alpha[4];
        std::uint32_t flags[4];
    };
    static_assert(sizeof(FrameData) == 400);
    static_assert(sizeof(MaterialData) == 64);
    if (!load_texture_asset("textures/demo_base_color.png", VK_FORMAT_R8G8B8A8_SRGB, true,
                            base_color_image_, base_color_memory_, base_color_view_,
                            base_color_sampler_, base_color_mip_levels_)) return false;
    if (!load_texture_asset("textures/demo_normal.tga", VK_FORMAT_R8G8B8A8_UNORM, false,
                            normal_image_, normal_memory_, normal_view_,
                            normal_sampler_, normal_mip_levels_)) return false;
    if (!load_texture_asset("textures/demo_metallic_roughness.tga", VK_FORMAT_R8G8B8A8_UNORM, false,
                            metallic_roughness_image_, metallic_roughness_memory_, metallic_roughness_view_,
                            metallic_roughness_sampler_, metallic_roughness_mip_levels_)) return false;
    if (!create_buffer(sizeof(FrameData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, frame_uniform_buffer_, frame_uniform_memory_) ||
        !create_buffer(sizeof(MaterialData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, material_uniform_buffer_, material_uniform_memory_) ||
        !create_buffer(sizeof(arut::rendering::EnvironmentProbeGpuData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       environment_probe_uniform_buffer_, environment_probe_uniform_memory_) ||
        !create_buffer(sizeof(arut::rendering::HumanoidSkinningPalette), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       bone_palette_uniform_buffer_, bone_palette_uniform_memory_)) return false;

    const auto rest_pose = arut::rendering::make_rest_humanoid_pose();
    arut::rendering::HumanoidSkinningPalette initial_bone_palette{};
    if (!arut::rendering::build_humanoid_skinning_palette(
            arut::core::make_robot_mannequin_skeleton(), rest_pose, initial_bone_palette)) {
        log_error("Build initial humanoid inverse-bind palette failed");
        return false;
    }
    void* palette_mapped = nullptr;
    VkResult palette_result = vkMapMemory(device_, bone_palette_uniform_memory_, 0,
        sizeof(initial_bone_palette), 0, &palette_mapped);
    if (palette_result != VK_SUCCESS || !palette_mapped) {
        log_error("Map initial humanoid bone palette failed", palette_result);
        return false;
    }
    std::memcpy(palette_mapped, &initial_bone_palette, sizeof(initial_bone_palette));
    vkUnmapMemory(device_, bone_palette_uniform_memory_);

    FrameData frame{};
    const std::span<const arut::rendering::EnvironmentProbeInfluence> scene_probes(
        environment_probe_influences_.data(), environment_probe_count_);
    arut::rendering::EnvironmentProbeGpuData environment_probe_data{};
    if (!arut::rendering::build_environment_probe_gpu_data(
            scene_probes, environment_probe_data)) {
        log_error("Build GPU environment-probe data failed");
        return false;
    }
    const std::array<float, 3> camera{2.2F, 1.8F, 2.8F};
    frame.camera_position[0] = camera[0];
    frame.camera_position[1] = camera[1];
    frame.camera_position[2] = camera[2];
    frame.camera_position[3] = 1.0F;
    frame.light_direction[0] = 0.45F;
    frame.light_direction[1] = -1.0F;
    frame.light_direction[2] = 0.35F;
    frame.light_radiance[0] = 3.0F;
    frame.light_radiance[1] = 2.8F;
    frame.light_radiance[2] = 2.5F;

    const auto view = arut::rendering::look_at_rh(camera, {0.0F, 0.0F, 0.0F},
                                                  {0.0F, 1.0F, 0.0F});
    const float camera_aspect = static_cast<float>(extent_.width) /
                                static_cast<float>(std::max(1U, extent_.height));
    constexpr float camera_fov_y = 0.785398163F;
    const float camera_tan_half_fov = std::tan(camera_fov_y * 0.5F);
    const auto projection = arut::rendering::perspective_rh_zo(camera_fov_y,
        camera_aspect, 0.1F, 100.0F);
    const auto view_projection = arut::rendering::multiply(projection, view);
    std::copy(view_projection.value.begin(), view_projection.value.end(), frame.view_projection);

    const std::array<float, 3> forward = arut::rendering::normalize3(
        {-camera[0], -camera[1], -camera[2]});
    frame.camera_forward[0] = forward[0];
    frame.camera_forward[1] = forward[1];
    frame.camera_forward[2] = forward[2];
    frame.camera_forward[3] = 0.0F;
    const std::array<float, 3> direction_to_light =
        arut::rendering::normalize3({-0.45F, 1.0F, -0.35F});
    const std::array<float, 4> cascade_ends{8.0F, 20.0F, 45.0F, 80.0F};
    for (std::uint32_t cascade = 0; cascade < 4U; ++cascade) {
        float start_distance = cascade == 0U ? 0.0F : cascade_ends[cascade - 1U];
        float end_distance = cascade_ends[cascade];
        if (shadow_cascade_count_ == 1U) {
            start_distance = 0.0F;
            end_distance = 80.0F;
        } else if (shadow_cascade_count_ == 2U && cascade == 1U) {
            end_distance = 80.0F;
        }
        const float midpoint = (start_distance + end_distance) * 0.5F;
        const std::array<float, 3> center{
            camera[0] + forward[0] * midpoint,
            camera[1] + forward[1] * midpoint,
            camera[2] + forward[2] * midpoint
        };
        const float half_width = end_distance * camera_tan_half_fov * camera_aspect;
        const float half_height = end_distance * camera_tan_half_fov;
        const float half_depth = (end_distance - start_distance) * 0.5F;
        // Enclose the far-plane corners and the longitudinal extent of this camera slice.
        const float radius = std::max(2.0F,
            std::sqrt(half_width * half_width + half_height * half_height +
                      half_depth * half_depth) + 0.5F);
        const std::array<float, 3> light_eye{
            center[0] + direction_to_light[0] * radius * 3.0F,
            center[1] + direction_to_light[1] * radius * 3.0F,
            center[2] + direction_to_light[2] * radius * 3.0F
        };
        const auto light_view = arut::rendering::look_at_rh(
            light_eye, center, {0.0F, 1.0F, 0.0F});
        const auto light_projection = arut::rendering::orthographic_rh_zo(
            -radius, radius, -radius, radius, 0.1F, radius * 8.0F);
        const auto light_view_projection = arut::rendering::stabilize_shadow_matrix(
            arut::rendering::multiply(light_projection, light_view), shadow_extent_.width);
        std::copy(light_view_projection.value.begin(), light_view_projection.value.end(),
                  frame.light_view_projection[cascade]);
    }
    frame.cascade_splits[0] = shadow_cascade_count_ > 1U ? cascade_ends[0] : 100000.0F;
    frame.cascade_splits[1] = shadow_cascade_count_ > 2U ? cascade_ends[1] : 100000.0F;
    frame.cascade_splits[2] = shadow_cascade_count_ > 3U ? cascade_ends[2] : 100000.0F;
    frame.cascade_splits[3] = 100000.0F;
    FrameData frame_upload = frame;
    void* mapped = nullptr;
    VkResult result = vkMapMemory(device_, frame_uniform_memory_, 0, sizeof(FrameData), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) { log_error("Map frame uniform buffer failed", result); return false; }
    std::memcpy(mapped, &frame_upload, sizeof(FrameData)); vkUnmapMemory(device_, frame_uniform_memory_);

    const MaterialData material{{1.0F, 1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F, 0.25F},
                                {0.68F, 1.0F, 1.0F, 0.5F}, {7U, 0U, 0U, 0U}};
    mapped = nullptr;
    result = vkMapMemory(device_, material_uniform_memory_, 0, sizeof(MaterialData), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) { log_error("Map material uniform buffer failed", result); return false; }
    std::memcpy(mapped, &material, sizeof(MaterialData)); vkUnmapMemory(device_, material_uniform_memory_);

    mapped = nullptr;
    result = vkMapMemory(device_, environment_probe_uniform_memory_, 0,
                         sizeof(environment_probe_data), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map environment-probe uniform buffer failed", result);
        return false;
    }
    std::memcpy(mapped, &environment_probe_data, sizeof(environment_probe_data));
    vkUnmapMemory(device_, environment_probe_uniform_memory_);

    const std::array<VkDescriptorPoolSize, 2> sizes{{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 15}
    }};
    VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool.maxSets = 2; pool.poolSizeCount = static_cast<std::uint32_t>(sizes.size()); pool.pPoolSizes = sizes.data();
    result = vkCreateDescriptorPool(device_, &pool, nullptr, &pbr_descriptor_pool_);
    if (result != VK_SUCCESS) { log_error("Create PBR descriptor pool failed", result); return false; }
    const std::array<VkDescriptorSetLayout, 2> layouts{frame_set_layout_, material_set_layout_};
    std::array<VkDescriptorSet, 2> sets{};
    VkDescriptorSetAllocateInfo allocate{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocate.descriptorPool = pbr_descriptor_pool_; allocate.descriptorSetCount = 2; allocate.pSetLayouts = layouts.data();
    result = vkAllocateDescriptorSets(device_, &allocate, sets.data());
    if (result != VK_SUCCESS) { log_error("Allocate PBR descriptor sets failed", result); return false; }
    frame_descriptor_set_ = sets[0]; material_descriptor_set_ = sets[1];

    VkDescriptorBufferInfo frame_info{frame_uniform_buffer_, 0, sizeof(FrameData)};
    VkDescriptorBufferInfo material_info{material_uniform_buffer_, 0, sizeof(MaterialData)};
    VkDescriptorBufferInfo environment_probe_info{
        environment_probe_uniform_buffer_, 0, sizeof(arut::rendering::EnvironmentProbeGpuData)};
    VkDescriptorBufferInfo bone_palette_info{
        bone_palette_uniform_buffer_, 0, sizeof(arut::rendering::HumanoidSkinningPalette)};
    const VkDescriptorImageInfo shadow_info{shadow_sampler_, shadow_view_, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo environment_info{environment_sampler_, environment_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo brdf_lut_info{brdf_lut_sampler_, brdf_lut_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo irradiance_info{irradiance_sampler_, irradiance_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo base_color_info{base_color_sampler_, base_color_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo normal_info{normal_sampler_, normal_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo metallic_roughness_info{metallic_roughness_sampler_, metallic_roughness_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    std::array<VkDescriptorImageInfo, arut::rendering::kMaxEnvironmentProbes> probe_specular_infos{};
    std::array<VkDescriptorImageInfo, arut::rendering::kMaxEnvironmentProbes> probe_diffuse_infos{};
    for (std::uint32_t probe = 0; probe < arut::rendering::kMaxEnvironmentProbes; ++probe) {
        probe_specular_infos[probe] = {
            environment_probe_specular_samplers_[probe]
                ? environment_probe_specular_samplers_[probe] : environment_sampler_,
            environment_probe_specular_views_[probe]
                ? environment_probe_specular_views_[probe] : environment_view_,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
        probe_diffuse_infos[probe] = {
            environment_probe_diffuse_samplers_[probe]
                ? environment_probe_diffuse_samplers_[probe] : irradiance_sampler_,
            environment_probe_diffuse_views_[probe]
                ? environment_probe_diffuse_views_[probe] : irradiance_view_,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    }
    std::array<VkWriteDescriptorSet, 13> writes{};
    writes[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &frame_info, nullptr};
    writes[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &shadow_info, nullptr, nullptr};
    writes[2] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &environment_info, nullptr, nullptr};
    writes[3] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 3, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &brdf_lut_info, nullptr, nullptr};
    writes[4] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 4, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &irradiance_info, nullptr, nullptr};
    writes[5] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 5, 0,
        static_cast<std::uint32_t>(probe_specular_infos.size()), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        probe_specular_infos.data(), nullptr, nullptr};
    writes[6] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 6, 0,
        static_cast<std::uint32_t>(probe_diffuse_infos.size()), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        probe_diffuse_infos.data(), nullptr, nullptr};
    writes[7] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 7, 0, 1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &environment_probe_info, nullptr};
    writes[8] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, material_descriptor_set_, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &material_info, nullptr};
    writes[9] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, material_descriptor_set_, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &base_color_info, nullptr, nullptr};
    writes[10] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, material_descriptor_set_, 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &normal_info, nullptr, nullptr};
    writes[11] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, material_descriptor_set_, 3, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &metallic_roughness_info, nullptr, nullptr};
    writes[12] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 8, 0, 1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &bone_palette_info, nullptr};
    vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
    arut_android_log(ANDROID_LOG_INFO, "PBR frame/material descriptors and fallback texture bound.");
    return true;
}

bool AndroidRenderer::create_commands_and_sync() noexcept {
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = graphics_family_;
    if (vkCreateCommandPool(device_, &pool, nullptr, &command_pool_) != VK_SUCCESS) return false;
    command_buffers_.resize(framebuffers_.size(), VK_NULL_HANDLE);
    VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc.commandPool = command_pool_;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = static_cast<std::uint32_t>(command_buffers_.size());
    if (vkAllocateCommandBuffers(device_, &alloc, command_buffers_.data()) != VK_SUCCESS) return false;
    VkSemaphoreCreateInfo semaphore{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    if (vkCreateSemaphore(device_, &semaphore, nullptr, &image_available_) != VK_SUCCESS) return false;
    render_finished_.assign(images_.size(), VK_NULL_HANDLE);
    for (auto& finished : render_finished_) {
        if (vkCreateSemaphore(device_, &semaphore, nullptr, &finished) != VK_SUCCESS) return false;
    }
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (vkCreateFence(device_, &fence, nullptr, &in_flight_) != VK_SUCCESS) return false;
    if (timestamp_queries_available_) {
        VkQueryPoolCreateInfo queries{VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        queries.queryType = VK_QUERY_TYPE_TIMESTAMP;
        queries.queryCount = 4;
        const VkResult query_result = vkCreateQueryPool(device_, &queries, nullptr, &timestamp_query_pool_);
        if (query_result != VK_SUCCESS) {
            timestamp_queries_available_ = false;
            timestamp_query_pool_ = VK_NULL_HANDLE;
            log_error("Timestamp query pool unavailable; GPU timing disabled", query_result);
        } else {
            arut_android_log(ANDROID_LOG_INFO, "Vulkan GPU timestamp profiling enabled.");
        }
    }
    return true;
}

void AndroidRenderer::set_gamepad_movement_input(float x, float y) noexcept {
    gamepad_move_x_ = std::isfinite(x) ? std::clamp(x, -1.0F, 1.0F) : 0.0F;
    gamepad_move_y_ = std::isfinite(y) ? std::clamp(y, -1.0F, 1.0F) : 0.0F;
}

void AndroidRenderer::clear_input() noexcept {
    joystick_x_ = joystick_y_ = 0.0F;
    gamepad_move_x_ = gamepad_move_y_ = 0.0F;
    gamepad_look_x_ = gamepad_look_y_ = 0.0F;
    joystick_pointer_ = look_pointer_ = jump_pointer_ = -1;
    jump_pressed_ = false;
}

void AndroidRenderer::set_gamepad_look_input(float x, float y) noexcept {
    gamepad_look_x_ = std::isfinite(x) ? std::clamp(x, -1.0F, 1.0F) : 0.0F;
    gamepad_look_y_ = std::isfinite(y) ? std::clamp(y, -1.0F, 1.0F) : 0.0F;
}

void AndroidRenderer::set_touch_input(std::int32_t action, std::int32_t pointer_id,
                                     float x, float y, float width, float height) noexcept {
    if (action == 0) {
        if (x < width * 0.48F && joystick_pointer_ < 0) {
            joystick_pointer_ = pointer_id;
            joystick_origin_x_ = x;
            joystick_origin_y_ = y;
            joystick_x_ = 0.0F;
            joystick_y_ = 0.0F;
        } else if (mannequin_scene_enabled_ && x >= width * 0.48F &&
                   y >= height * 0.68F && jump_pointer_ < 0) {
            // Tap/hold in the lower-right quadrant is the humanoid jump action.
            jump_pointer_ = pointer_id;
            jump_pressed_ = true;
        } else if (x >= width * 0.48F && look_pointer_ < 0) {
            look_pointer_ = pointer_id;
            look_last_x_ = x;
            look_last_y_ = y;
        }
        return;
    }
    if (action == 2) {
        if (pointer_id == joystick_pointer_) {
            joystick_pointer_ = -1;
            joystick_x_ = joystick_y_ = 0.0F;
        }
        if (pointer_id == look_pointer_) look_pointer_ = -1;
        if (pointer_id == jump_pointer_) jump_pointer_ = -1;
        return;
    }
    if (action != 1) return;
    if (pointer_id == joystick_pointer_) {
        const float dx = x - joystick_origin_x_;
        const float dy = y - joystick_origin_y_;
        const float distance = std::max(1.0F, std::sqrt(dx * dx + dy * dy));
        const float magnitude = std::min(1.0F, distance / joystick_radius_);
        joystick_x_ = (dx / distance) * magnitude;
        joystick_y_ = (-dy / distance) * magnitude;
    } else if (pointer_id == look_pointer_) {
        camera_yaw_ += (x - look_last_x_) * 0.0045F;
        camera_pitch_ = std::clamp(camera_pitch_ - (y - look_last_y_) * 0.0045F, -1.45F, 1.45F);
        look_last_x_ = x;
        look_last_y_ = y;
    }
}

void AndroidRenderer::update_stress_camera(float delta_seconds) noexcept {
    const float dt = std::clamp(delta_seconds, 0.0F, 0.05F);
    camera_yaw_ += gamepad_look_x_ * 2.2F * dt;
    camera_pitch_ = std::clamp(
        camera_pitch_ - gamepad_look_y_ * 1.8F * dt, -1.45F, 1.45F);
    const float cosine_pitch = std::cos(camera_pitch_);
    const std::array<float, 3> forward{
        std::cos(camera_yaw_) * cosine_pitch,
        std::sin(camera_pitch_),
        std::sin(camera_yaw_) * cosine_pitch
    };
    const std::array<float, 3> right{-std::sin(camera_yaw_), 0.0F, std::cos(camera_yaw_)};
    if (!mannequin_scene_enabled_) {
        constexpr float speed = 9.0F;
        for (std::size_t i = 0; i < 3U; ++i) {
            camera_position_[i] += (forward[i] * (joystick_y_ + gamepad_move_y_) +
                                    right[i] * (joystick_x_ + gamepad_move_x_)) * speed * dt;
        }
    }
    const std::array<float, 3> target{
        camera_position_[0] + forward[0],
        camera_position_[1] + forward[1],
        camera_position_[2] + forward[2]
    };
    const auto view = arut::rendering::look_at_rh(camera_position_, target, {0.0F, 1.0F, 0.0F});
    const auto projection = arut::rendering::perspective_rh_zo(0.785398163F,
        static_cast<float>(extent_.width) / static_cast<float>(std::max(1U, extent_.height)), 0.1F, 180.0F);
    const auto view_projection = arut::rendering::multiply(projection, view);
    const auto frustum = arut::rendering::extract_vulkan_frustum(view_projection);
    std::vector<arut::rendering::Mat4> cpu_visible;
    cpu_visible.reserve(visibility_instances_.size());
    if (!arut::rendering::compact_visible_instances_cpu(frustum, visibility_instances_, cpu_visible)) {
        log_error("Stress camera CPU visibility reference failed");
        return;
    }
    cpu_expected_visible_ = static_cast<std::uint32_t>(cpu_visible.size());
    for (std::size_t plane = 0; plane < frustum.planes.size(); ++plane) {
        const auto& p = frustum.planes[plane];
        visibility_frustum_planes_[plane * 4U + 0U] = p.normal[0];
        visibility_frustum_planes_[plane * 4U + 1U] = p.normal[1];
        visibility_frustum_planes_[plane * 4U + 2U] = p.normal[2];
        visibility_frustum_planes_[plane * 4U + 3U] = p.distance;
    }
    // FrameData layout: viewProjection[0..63], cameraPosition[64..79], cameraForward[80..95].
    void* mapped = nullptr;
    const VkResult map_result = vkMapMemory(device_, frame_uniform_memory_, 0, 96U, 0, &mapped);
    if (map_result != VK_SUCCESS || !mapped) {
        log_error("Map camera frame data failed", map_result);
        return;
    }
    std::memcpy(mapped, view_projection.value.data(), sizeof(float) * 16U);
    std::memcpy(static_cast<std::byte*>(mapped) + 64U, camera_position_.data(), sizeof(float) * 3U);
    static_cast<float*>(mapped)[19] = 1.0F;
    std::memcpy(static_cast<std::byte*>(mapped) + 80U, forward.data(), sizeof(float) * 3U);
    static_cast<float*>(mapped)[23] = 0.0F;
    vkUnmapMemory(device_, frame_uniform_memory_);
}

bool AndroidRenderer::load_imported_character_glb(
    const char* asset_path, arut::animation::SkeletalAsset& out_asset,
    const arut::rendering::Mat4& model) noexcept {
    if (!assets_ || !asset_path || asset_path[0] == '\0') {
        log_error("GLB load rejected: Android asset manager or path is missing");
        return false;
    }
    AAsset* file = AAssetManager_open(assets_, asset_path, AASSET_MODE_BUFFER);
    if (!file) {
        log_error("GLB load failed: packaged character asset was not found");
        return false;
    }
    const off_t length = AAsset_getLength(file);
    if (length <= 0 || length > static_cast<off_t>(256 * 1024 * 1024)) {
        AAsset_close(file);
        log_error("GLB load rejected: file size is empty or exceeds the 256 MiB safety limit");
        return false;
    }

    std::vector<std::uint8_t> bytes;
    try {
        bytes.resize(static_cast<std::size_t>(length));
    } catch (...) {
        AAsset_close(file);
        log_error("GLB load failed: could not allocate file buffer");
        return false;
    }
    std::size_t bytes_read = 0U;
    while (bytes_read < bytes.size()) {
        const int chunk = AAsset_read(file, bytes.data() + bytes_read, bytes.size() - bytes_read);
        if (chunk <= 0) break;
        bytes_read += static_cast<std::size_t>(chunk);
    }
    AAsset_close(file);
    if (bytes_read != bytes.size()) {
        log_error("GLB load failed: packaged asset read was incomplete");
        return false;
    }

    arut::animation::SkeletalAsset asset{};
    arut::animation::GltfImportReport report{};
    bool imported = false;
    try {
        imported = arut::animation::import_glb(bytes.data(), bytes.size(), asset, report);
    } catch (...) {
        log_error("GLB import failed: allocation or parser exception");
        return false;
    }
    if (!imported) {
        char message[256]{};
        std::snprintf(message, sizeof(message), "GLB import failed: %s", report.message.c_str());
        log_error(message);
        return false;
    }

    arut::animation::Pose rest_pose{};
    rest_pose.joint_count = asset.joint_count;
    for (std::size_t joint = 0; joint < asset.joint_count; ++joint)
        rest_pose.joints[joint] = asset.joints[joint].rest_local;
    if (!set_imported_character(asset, rest_pose, model)) {
        log_error("GLB parsed successfully but Vulkan character installation failed");
        return false;
    }
    try {
        out_asset = std::move(asset);
    } catch (...) {
        log_error("GLB was uploaded, but returning its animation data failed");
        return false;
    }
    arut_android_log(ANDROID_LOG_INFO, "GLB_CHARACTER_LOAD success=true initial_pose=rest animations_available=true");
    return true;
}

bool AndroidRenderer::set_imported_character(
    const arut::animation::SkeletalAsset& asset,
    const arut::animation::Pose& pose,
    const arut::rendering::Mat4& model) noexcept {
    const bool model_is_finite = std::all_of(model.value.begin(), model.value.end(),
        [](float value) { return std::isfinite(value); });
    if (!ready() || !mannequin_scene_enabled_ || !mannequin_instance_memory_ ||
        !bone_palette_uniform_memory_ || !model_is_finite || !asset.valid() || asset.meshes.empty() ||
        asset.meshes.front().indices.size() > UINT32_MAX) {
        log_error("Imported character rejected: renderer, asset, or mannequin resources are not ready");
        return false;
    }

    arut::rendering::MeshData mesh{};
    try {
        if (!arut::rendering::convert_imported_skinned_mesh(asset, asset.meshes.front(), mesh) ||
            mesh.vertices.empty() || mesh.indices.empty()) {
            log_error("Imported character vertex conversion failed");
            return false;
        }
    } catch (...) {
        log_error("Imported character vertex conversion failed: allocation exception");
        return false;
    }

    arut::rendering::ImportedSkinningPalette imported_palette{};
    if (!arut::rendering::build_imported_skinning_palette(asset, pose, imported_palette)) {
        log_error("Imported character skinning palette build failed");
        return false;
    }
    arut::rendering::HumanoidSkinningPalette gpu_palette{};
    for (std::size_t joint = 0; joint < imported_palette.joint_count; ++joint)
        gpu_palette.matrices[joint] = imported_palette.matrices[joint];
    for (std::size_t joint = imported_palette.joint_count;
         joint < gpu_palette.matrices.size(); ++joint)
        gpu_palette.matrices[joint] = arut::rendering::Mat4::identity();

    VkBuffer new_vertex_buffer = VK_NULL_HANDLE;
    VkDeviceMemory new_vertex_memory = VK_NULL_HANDLE;
    VkBuffer new_index_buffer = VK_NULL_HANDLE;
    VkDeviceMemory new_index_memory = VK_NULL_HANDLE;
    const auto discard_buffer = [this](VkBuffer& buffer, VkDeviceMemory& memory) {
        if (buffer != VK_NULL_HANDLE) {
            VkMemoryRequirements requirements{};
            vkGetBufferMemoryRequirements(device_, buffer, &requirements);
            if (requirements.size <= tracked_gpu_memory_bytes_)
                tracked_gpu_memory_bytes_ -= requirements.size;
            vkDestroyBuffer(device_, buffer, nullptr);
            buffer = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device_, memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    };
    const VkDeviceSize vertex_bytes = static_cast<VkDeviceSize>(
        mesh.vertices.size() * sizeof(arut::rendering::MeshVertex));
    const VkDeviceSize index_bytes = static_cast<VkDeviceSize>(
        mesh.indices.size() * sizeof(std::uint32_t));
    if (!create_buffer(vertex_bytes, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       new_vertex_buffer, new_vertex_memory) ||
        !create_buffer(index_bytes, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       new_index_buffer, new_index_memory)) {
        discard_buffer(new_vertex_buffer, new_vertex_memory);
        discard_buffer(new_index_buffer, new_index_memory);
        log_error("Imported character GPU buffer allocation failed");
        return false;
    }

    void* mapped = nullptr;
    VkResult result = vkMapMemory(device_, new_vertex_memory, 0, vertex_bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        discard_buffer(new_vertex_buffer, new_vertex_memory);
        discard_buffer(new_index_buffer, new_index_memory);
        log_error("Map imported character vertex buffer failed", result);
        return false;
    }
    std::memcpy(mapped, mesh.vertices.data(), static_cast<std::size_t>(vertex_bytes));
    vkUnmapMemory(device_, new_vertex_memory);

    mapped = nullptr;
    result = vkMapMemory(device_, new_index_memory, 0, index_bytes, 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        discard_buffer(new_vertex_buffer, new_vertex_memory);
        discard_buffer(new_index_buffer, new_index_memory);
        log_error("Map imported character index buffer failed", result);
        return false;
    }
    std::memcpy(mapped, mesh.indices.data(), static_cast<std::size_t>(index_bytes));
    vkUnmapMemory(device_, new_index_memory);

    result = vkDeviceWaitIdle(device_);
    if (result != VK_SUCCESS) {
        discard_buffer(new_vertex_buffer, new_vertex_memory);
        discard_buffer(new_index_buffer, new_index_memory);
        log_error("Wait before replacing imported character buffers failed", result);
        return false;
    }

    discard_buffer(imported_character_vertex_buffer_, imported_character_vertex_memory_);
    discard_buffer(imported_character_index_buffer_, imported_character_index_memory_);
    imported_character_vertex_buffer_ = new_vertex_buffer;
    imported_character_vertex_memory_ = new_vertex_memory;
    imported_character_index_buffer_ = new_index_buffer;
    imported_character_index_memory_ = new_index_memory;
    imported_character_index_count_ = static_cast<std::uint32_t>(mesh.indices.size());
    imported_character_skeleton_signature_ = imported_skeleton_signature(asset);
    imported_character_enabled_ = true;

    pending_mannequin_model_ = model;
    pending_humanoid_palette_ = gpu_palette;
    humanoid_camera_position_ = camera_position_;
    has_external_humanoid_state_ = true;
    arut_android_log(ANDROID_LOG_INFO, "IMPORTED_CHARACTER uploaded=true mesh=first_skinned_mesh skinning=128_joint_palette");
    return true;
}

bool AndroidRenderer::set_imported_character_pose(
    const arut::animation::SkeletalAsset& asset,
    const arut::animation::Pose& pose,
    const arut::rendering::Mat4& model) noexcept {
    if (!ready() || !imported_character_enabled_ || !mannequin_scene_enabled_ ||
        !bone_palette_uniform_memory_ ||
        imported_skeleton_signature(asset) != imported_character_skeleton_signature_ ||
        !std::all_of(model.value.begin(), model.value.end(),
            [](float value) { return std::isfinite(value); })) return false;

    arut::rendering::ImportedSkinningPalette imported_palette{};
    if (!arut::rendering::build_imported_skinning_palette(asset, pose, imported_palette))
        return false;

    arut::rendering::HumanoidSkinningPalette gpu_palette{};
    for (std::size_t joint = 0; joint < imported_palette.joint_count; ++joint)
        gpu_palette.matrices[joint] = imported_palette.matrices[joint];
    for (std::size_t joint = imported_palette.joint_count;
         joint < gpu_palette.matrices.size(); ++joint)
        gpu_palette.matrices[joint] = arut::rendering::Mat4::identity();

    pending_mannequin_model_ = model;
    pending_humanoid_palette_ = gpu_palette;
    has_external_humanoid_state_ = true;
    return true;
}

void AndroidRenderer::set_humanoid_state(
    const arut::rendering::Mat4& model,
    const arut::rendering::HumanoidSkinningPalette& palette,
    const std::array<float, 3>& camera_target,
    const std::array<float, 3>& camera_position) noexcept {
    if (!mannequin_scene_enabled_) return;
    pending_mannequin_model_ = model;
    pending_humanoid_palette_ = palette;
    humanoid_camera_target_ = camera_target;
    humanoid_camera_position_ = camera_position;
    has_external_humanoid_state_ = true;
}

void AndroidRenderer::update_humanoid_animation(float delta_seconds) noexcept {
    if (!mannequin_scene_enabled_ || !bone_palette_uniform_memory_) return;
    if (has_external_humanoid_state_) {
        void* mapped = nullptr;
        VkResult result = vkMapMemory(device_, mannequin_instance_memory_, 0,
            sizeof(pending_mannequin_model_), 0, &mapped);
        if (result != VK_SUCCESS || !mapped) {
            log_error("Map humanoid model transform failed", result);
            return;
        }
        std::memcpy(mapped, &pending_mannequin_model_, sizeof(pending_mannequin_model_));
        vkUnmapMemory(device_, mannequin_instance_memory_);

        mapped = nullptr;
        result = vkMapMemory(device_, bone_palette_uniform_memory_, 0,
            sizeof(pending_humanoid_palette_), 0, &mapped);
        if (result != VK_SUCCESS || !mapped) {
            log_error("Map externally animated humanoid palette failed", result);
            return;
        }
        std::memcpy(mapped, &pending_humanoid_palette_, sizeof(pending_humanoid_palette_));
        vkUnmapMemory(device_, bone_palette_uniform_memory_);

        camera_position_ = humanoid_camera_position_;
        return;
    }
    humanoid_animation_time_seconds_ += std::clamp(delta_seconds, 0.0F, 0.05F);
    auto pose = arut::rendering::make_rest_humanoid_pose();
    const float phase = humanoid_animation_time_seconds_;
    const auto chest = static_cast<std::size_t>(arut::core::HumanoidBone::Chest);
    const auto left_arm = static_cast<std::size_t>(arut::core::HumanoidBone::LeftUpperArm);
    const auto right_arm = static_cast<std::size_t>(arut::core::HumanoidBone::RightUpperArm);
    // A small procedural idle cycle validates bone deformation before the
    // full locomotion clip/blend system is added in the next slice.
    pose.local_rotations[chest] = arut::rendering::quaternion_from_axis_angle(
        {0.0F, 0.0F, 1.0F}, 0.025F * std::sin(phase * 1.7F));
    pose.local_rotations[left_arm] = arut::rendering::quaternion_from_axis_angle(
        {1.0F, 0.0F, 0.0F}, 0.018F * std::sin(phase * 1.2F));
    pose.local_rotations[right_arm] = arut::rendering::quaternion_from_axis_angle(
        {1.0F, 0.0F, 0.0F}, -0.018F * std::sin(phase * 1.2F));

    arut::rendering::HumanoidSkinningPalette palette{};
    if (!arut::rendering::build_humanoid_skinning_palette(
            arut::core::make_robot_mannequin_skeleton(), pose, palette)) {
        log_error("Build animated humanoid skinning palette failed");
        return;
    }
    void* mapped = nullptr;
    const VkResult result = vkMapMemory(device_, bone_palette_uniform_memory_, 0,
        sizeof(palette), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) {
        log_error("Map animated humanoid bone palette failed", result);
        return;
    }
    std::memcpy(mapped, &palette, sizeof(palette));
    vkUnmapMemory(device_, bone_palette_uniform_memory_);
    ++humanoid_animation_updates_;
    if (humanoid_animation_updates_ == 1U || (humanoid_animation_updates_ % 300U) == 0U) {
        char message[192]{};
        std::snprintf(message, sizeof(message),
            "HUMANOID_ANIMATION mode=procedural_idle updates=%llu time_seconds=%.2f skinning=enabled",
            static_cast<unsigned long long>(humanoid_animation_updates_),
            humanoid_animation_time_seconds_);
        arut_android_log(ANDROID_LOG_INFO, message);
    }
}

bool AndroidRenderer::draw_frame() noexcept {
    if (!ready()) return false;
    const auto cpu_frame_start = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    const float delta_seconds = last_camera_update_.time_since_epoch().count() == 0
        ? 0.016F : std::chrono::duration<float>(now - last_camera_update_).count();
    last_camera_update_ = now;
    VkResult result = vkWaitForFences(device_, 1, &in_flight_, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        log_error("vkWaitForFences before frame failed", result);
        return false;
    }
    update_humanoid_animation(delta_seconds);
    update_stress_camera(delta_seconds);
    if (gpu_visibility_enabled_ && gpu_visibility_result_pending_) {
        void* mapped = nullptr;
        const VkResult map_result = vkMapMemory(device_, indirect_draw_memory_, 0,
            sizeof(arut::rendering::IndexedIndirectArguments), 0, &mapped);
        if (map_result == VK_SUCCESS && mapped) {
            arut::rendering::IndexedIndirectArguments completed{};
            std::memcpy(&completed, mapped, sizeof(completed));
            vkUnmapMemory(device_, indirect_draw_memory_);
            last_visible_objects_ = completed.instance_count;
            gpu_visibility_result_pending_ = false;
            ++culling_parity_checks_;
            if (completed.instance_count != submitted_cpu_expected_visible_) {
                ++culling_parity_mismatches_;
                if (culling_parity_mismatches_ < 5U || (culling_parity_mismatches_ % 120U) == 0U) {
                    char parity[192]{};
                    std::snprintf(parity, sizeof(parity), "GPU_CULL_PARITY mismatch gpu_visible=%u cpu_visible=%u source=%u",
                        completed.instance_count, submitted_cpu_expected_visible_, visibility_source_instance_count_);
                    arut_android_log(ANDROID_LOG_WARN, parity);
                }
            } else if (culling_parity_checks_ == 1U || (culling_parity_checks_ % 120U) == 0U) {
                char parity[192]{};
                std::snprintf(parity, sizeof(parity), "GPU_CULL_PARITY pass gpu_visible=%u cpu_visible=%u source=%u checks=%llu",
                    completed.instance_count, submitted_cpu_expected_visible_, visibility_source_instance_count_,
                    static_cast<unsigned long long>(culling_parity_checks_));
                arut_android_log(ANDROID_LOG_INFO, parity);
            }
            last_triangles_ = (cube_index_count_ / 3U) * completed.instance_count +
                terrain_triangle_count_ +
                ((!imported_character_enabled_ ? mannequin_index_count_ : 0U) +
                 (imported_character_enabled_ ? imported_character_index_count_ : 0U)) / 3U +
                (cube_index_count_ / 3U) * static_scene_instance_count_;
            last_visible_objects_ += static_scene_instance_count_;
            if (mannequin_index_count_ != 0U && !imported_character_enabled_) ++last_visible_objects_;
            if (imported_character_enabled_ && imported_character_index_count_ != 0U) ++last_visible_objects_;
        } else {
            gpu_visibility_result_pending_ = false;
            log_error("Read completed GPU visibility count failed; retaining last profile counters", map_result);
        }
    } else if (!gpu_visibility_enabled_) {
        last_visible_objects_ = instance_count_;
        last_triangles_ = (cube_index_count_ / 3U) * instance_count_ +
            terrain_triangle_count_ +
            ((!imported_character_enabled_ ? mannequin_index_count_ : 0U) +
             (imported_character_enabled_ ? imported_character_index_count_ : 0U)) / 3U +
            (cube_index_count_ / 3U) * static_scene_instance_count_;
        last_visible_objects_ = instance_count_ + static_scene_instance_count_ +
            ((mannequin_index_count_ != 0U && !imported_character_enabled_) ? 1U : 0U) +
            ((imported_character_enabled_ && imported_character_index_count_ != 0U) ? 1U : 0U);
    }
    if (timestamp_queries_available_ && timestamp_data_pending_) {
        std::array<std::uint64_t, 4> timestamps{};
        const VkResult query_result = vkGetQueryPoolResults(
            device_, timestamp_query_pool_, 0, 4, sizeof(timestamps), timestamps.data(),
            sizeof(std::uint64_t), VK_QUERY_RESULT_64_BIT);
        if (query_result == VK_SUCCESS) {
            const std::uint64_t mask = timestamp_valid_bits_ >= 64U
                ? UINT64_MAX : ((std::uint64_t{1} << timestamp_valid_bits_) - 1U);
            const std::uint64_t visibility_ticks = (timestamps[1] - timestamps[0]) & mask;
            const std::uint64_t shadow_ticks = (timestamps[2] - timestamps[1]) & mask;
            const std::uint64_t main_pass_ticks = (timestamps[3] - timestamps[2]) & mask;
            const std::uint64_t frame_ticks = (timestamps[3] - timestamps[0]) & mask;
            last_visibility_pass_ms_ = static_cast<double>(visibility_ticks) * timestamp_period_ns_ / 1000000.0;
            last_shadow_pass_ms_ = static_cast<double>(shadow_ticks) * timestamp_period_ns_ / 1000000.0;
            last_main_pass_ms_ = static_cast<double>(main_pass_ticks) * timestamp_period_ns_ / 1000000.0;
            last_gpu_frame_ms_ = static_cast<double>(frame_ticks) * timestamp_period_ns_ / 1000000.0;
        }
        timestamp_data_pending_ = false;
    }
    std::uint32_t image_index = 0;
    result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_, VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) return recreate_swapchain();
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        log_error("vkAcquireNextImageKHR failed", result);
        return false;
    }
    result = vkResetCommandBuffer(command_buffers_[image_index], 0);
    if (result != VK_SUCCESS) {
        log_error("vkResetCommandBuffer failed", result);
        return false;
    }
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    result = vkBeginCommandBuffer(command_buffers_[image_index], &begin);
    if (result != VK_SUCCESS) {
        log_error("vkBeginCommandBuffer failed", result);
        return false;
    }
    if (timestamp_queries_available_) {
        vkCmdResetQueryPool(command_buffers_[image_index], timestamp_query_pool_, 0, 4);
        vkCmdWriteTimestamp(command_buffers_[image_index], VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             timestamp_query_pool_, 0);
    }
    if (!record_gpu_visibility(command_buffers_[image_index])) return false;
    if (timestamp_queries_available_) {
        const VkPipelineStageFlagBits visibility_timestamp_stage = gpu_visibility_enabled_
            ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        vkCmdWriteTimestamp(command_buffers_[image_index], visibility_timestamp_stage,
                             timestamp_query_pool_, 1);
    }
    if (shadow_enabled_) {
        VkClearValue shadow_clear{};
        shadow_clear.depthStencil = {1.0F, 0};
        vkCmdBindPipeline(command_buffers_[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, shadow_pipeline_);
        vkCmdBindDescriptorSets(command_buffers_[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pbr_pipeline_layout_, 0, 1, &frame_descriptor_set_, 0, nullptr);
        VkViewport shadow_viewport{};
        shadow_viewport.width = static_cast<float>(shadow_extent_.width);
        shadow_viewport.height = static_cast<float>(shadow_extent_.height);
        shadow_viewport.minDepth = 0.0F;
        shadow_viewport.maxDepth = 1.0F;
        VkRect2D shadow_scissor{{0, 0}, shadow_extent_};
        vkCmdSetViewport(command_buffers_[image_index], 0, 1, &shadow_viewport);
        vkCmdSetScissor(command_buffers_[image_index], 0, 1, &shadow_scissor);
        const std::array<VkBuffer, 2> shadow_vertex_buffers{cube_vertex_buffer_, instance_buffer_};
        const std::array<VkDeviceSize, 2> shadow_vertex_offsets{0, 0};
        vkCmdBindVertexBuffers(command_buffers_[image_index], 0,
                               static_cast<std::uint32_t>(shadow_vertex_buffers.size()),
                               shadow_vertex_buffers.data(), shadow_vertex_offsets.data());
        vkCmdBindIndexBuffer(command_buffers_[image_index], cube_index_buffer_, 0, VK_INDEX_TYPE_UINT32);
        for (std::uint32_t cascade = 0; cascade < shadow_cascade_count_; ++cascade) {
            VkRenderPassBeginInfo shadow_pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
            shadow_pass.renderPass = shadow_render_pass_;
            shadow_pass.framebuffer = shadow_framebuffers_[cascade];
            shadow_pass.renderArea.extent = shadow_extent_;
            shadow_pass.clearValueCount = 1;
            shadow_pass.pClearValues = &shadow_clear;
            vkCmdBeginRenderPass(command_buffers_[image_index], &shadow_pass, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdPushConstants(command_buffers_[image_index], pbr_pipeline_layout_,
                               VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(cascade), &cascade);
            vkCmdDrawIndexedIndirect(command_buffers_[image_index], indirect_draw_buffer_, 0, 1,
                                     sizeof(VkDrawIndexedIndirectCommand));
            if (mannequin_index_count_ != 0U && !imported_character_enabled_) {
                const std::array<VkBuffer, 2> mannequin_buffers{
                    mannequin_vertex_buffer_, mannequin_instance_buffer_};
                const std::array<VkDeviceSize, 2> mannequin_offsets{0, 0};
                vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 2,
                    mannequin_buffers.data(), mannequin_offsets.data());
                vkCmdBindIndexBuffer(command_buffers_[image_index], mannequin_index_buffer_,
                    0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(command_buffers_[image_index], mannequin_index_count_, 1, 0, 0, 0);
            }
            if (imported_character_enabled_ && imported_character_index_count_ != 0U) {
                const std::array<VkBuffer, 2> imported_buffers{
                    imported_character_vertex_buffer_, mannequin_instance_buffer_};
                const std::array<VkDeviceSize, 2> imported_offsets{0, 0};
                vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 2,
                    imported_buffers.data(), imported_offsets.data());
                vkCmdBindIndexBuffer(command_buffers_[image_index], imported_character_index_buffer_,
                    0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(command_buffers_[image_index], imported_character_index_count_, 1, 0, 0, 0);
            }
            if (static_scene_instance_count_ != 0U) {
                const std::array<VkBuffer, 2> static_buffers{
                    cube_vertex_buffer_, static_scene_instance_buffer_};
                const std::array<VkDeviceSize, 2> static_offsets{0, 0};
                vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 2,
                    static_buffers.data(), static_offsets.data());
                vkCmdBindIndexBuffer(command_buffers_[image_index], cube_index_buffer_,
                    0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(command_buffers_[image_index], cube_index_count_,
                    static_scene_instance_count_, 0, 0, 0);
            }
            for (const auto& terrain : terrain_gpu_meshes_) {
                const std::array<VkBuffer, 2> terrain_buffers{terrain.vertex_buffer, terrain.instance_buffer};
                const std::array<VkDeviceSize, 2> terrain_offsets{0, 0};
                vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 2, terrain_buffers.data(), terrain_offsets.data());
                vkCmdBindIndexBuffer(command_buffers_[image_index], terrain.index_buffer, 0, VK_INDEX_TYPE_UINT32);
                vkCmdDrawIndexed(command_buffers_[image_index], terrain.index_count, 1, 0, 0, 0);
            }
            vkCmdEndRenderPass(command_buffers_[image_index]);
        }
    }
    if (timestamp_queries_available_) {
        vkCmdWriteTimestamp(command_buffers_[image_index], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             timestamp_query_pool_, 2);
    }
    std::array<VkClearValue, 2> clears{};
    clears[0].color = {{0.025F, 0.045F, 0.075F, 1.0F}};
    clears[1].depthStencil = {1.0F, 0};
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass = render_pass_;
    pass.framebuffer = framebuffers_[image_index];
    pass.renderArea.extent = extent_;
    pass.clearValueCount = static_cast<std::uint32_t>(clears.size());
    pass.pClearValues = clears.data();
    vkCmdBeginRenderPass(command_buffers_[image_index], &pass, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffers_[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS, pbr_pipeline_);
    const std::array<VkDescriptorSet, 2> pbr_sets{frame_descriptor_set_, material_descriptor_set_};
    vkCmdBindDescriptorSets(command_buffers_[image_index], VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pbr_pipeline_layout_, 0, static_cast<std::uint32_t>(pbr_sets.size()),
                            pbr_sets.data(), 0, nullptr);
    VkViewport viewport{};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width = static_cast<float>(extent_.width);
    viewport.height = static_cast<float>(extent_.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    VkRect2D scissor{{0, 0}, extent_};
    vkCmdSetViewport(command_buffers_[image_index], 0, 1, &viewport);
    vkCmdSetScissor(command_buffers_[image_index], 0, 1, &scissor);
    constexpr VkDeviceSize vertex_offset = 0;
    const std::array<VkBuffer, 2> vertex_buffers{cube_vertex_buffer_, instance_buffer_};
    const std::array<VkDeviceSize, 2> vertex_offsets{0, 0};
    vkCmdBindVertexBuffers(command_buffers_[image_index], 0,
                           static_cast<std::uint32_t>(vertex_buffers.size()),
                           vertex_buffers.data(), vertex_offsets.data());
    vkCmdBindIndexBuffer(command_buffers_[image_index], cube_index_buffer_, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexedIndirect(command_buffers_[image_index], indirect_draw_buffer_, 0, 1,
                             sizeof(VkDrawIndexedIndirectCommand));
    if (mannequin_index_count_ != 0U && !imported_character_enabled_) {
        const std::array<VkBuffer, 2> mannequin_buffers{
            mannequin_vertex_buffer_, mannequin_instance_buffer_};
        const std::array<VkDeviceSize, 2> mannequin_offsets{0, 0};
        vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 2,
            mannequin_buffers.data(), mannequin_offsets.data());
        vkCmdBindIndexBuffer(command_buffers_[image_index], mannequin_index_buffer_,
            0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffers_[image_index], mannequin_index_count_, 1, 0, 0, 0);
    }
    if (imported_character_enabled_ && imported_character_index_count_ != 0U) {
        const std::array<VkBuffer, 2> imported_buffers{
            imported_character_vertex_buffer_, mannequin_instance_buffer_};
        const std::array<VkDeviceSize, 2> imported_offsets{0, 0};
        vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 2,
            imported_buffers.data(), imported_offsets.data());
        vkCmdBindIndexBuffer(command_buffers_[image_index], imported_character_index_buffer_,
            0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffers_[image_index], imported_character_index_count_, 1, 0, 0, 0);
    }
    if (static_scene_instance_count_ != 0U) {
        const std::array<VkBuffer, 2> static_buffers{
            cube_vertex_buffer_, static_scene_instance_buffer_};
        const std::array<VkDeviceSize, 2> static_offsets{0, 0};
        vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 2,
            static_buffers.data(), static_offsets.data());
        vkCmdBindIndexBuffer(command_buffers_[image_index], cube_index_buffer_,
            0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffers_[image_index], cube_index_count_,
            static_scene_instance_count_, 0, 0, 0);
    }
    for (const auto& terrain : terrain_gpu_meshes_) {
        const std::array<VkBuffer, 2> terrain_buffers{terrain.vertex_buffer, terrain.instance_buffer};
        const std::array<VkDeviceSize, 2> terrain_offsets{0, 0};
        vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 2, terrain_buffers.data(), terrain_offsets.data());
        vkCmdBindIndexBuffer(command_buffers_[image_index], terrain.index_buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(command_buffers_[image_index], terrain.index_count, 1, 0, 0, 0);
    }
    vkCmdEndRenderPass(command_buffers_[image_index]);
    if (timestamp_queries_available_) {
        vkCmdWriteTimestamp(command_buffers_[image_index], VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                             timestamp_query_pool_, 3);
    }
    result = vkEndCommandBuffer(command_buffers_[image_index]);
    if (result != VK_SUCCESS) {
        log_error("vkEndCommandBuffer failed", result);
        return false;
    }
    // Reset only after command recording succeeds; otherwise the next frame could wait
    // forever on a fence that was reset but never submitted.
    result = vkResetFences(device_, 1, &in_flight_);
    if (result != VK_SUCCESS) {
        log_error("vkResetFences failed", result);
        return false;
    }
    const VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &image_available_;
    submit.pWaitDstStageMask = &wait_stage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command_buffers_[image_index];
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &render_finished_[image_index];
    result = vkQueueSubmit(graphics_queue_, 1, &submit, in_flight_);
    if (result != VK_SUCCESS) {
        log_error("vkQueueSubmit failed", result);
        // A failed submit will not signal the reset fence. Restore a signaled fence
        // so the runtime cannot deadlock on the next frame while reporting the failure.
        vkDestroyFence(device_, in_flight_, nullptr);
        VkFenceCreateInfo recovery{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        recovery.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        in_flight_ = VK_NULL_HANDLE;
        if (vkCreateFence(device_, &recovery, nullptr, &in_flight_) != VK_SUCCESS)
            log_error("Failed to recreate frame fence after submit failure");
        return false;
    }
    timestamp_data_pending_ = timestamp_queries_available_;
    if (gpu_visibility_enabled_) {
        submitted_cpu_expected_visible_ = cpu_expected_visible_;
        gpu_visibility_result_pending_ = true;
    }
    const auto terrain_draws = static_cast<std::uint32_t>(terrain_gpu_meshes_.size());
    const std::uint32_t mannequin_draws =
        (mannequin_index_count_ != 0U && !imported_character_enabled_) ? 1U : 0U;
    const std::uint32_t imported_draws =
        (imported_character_enabled_ && imported_character_index_count_ != 0U) ? 1U : 0U;
    const std::uint32_t static_scene_draws = static_scene_instance_count_ != 0U ? 1U : 0U;
    const std::uint32_t scene_draws =
        terrain_draws + 1U + mannequin_draws + imported_draws + static_scene_draws;
    last_draw_calls_ = (shadow_enabled_ ? shadow_cascade_count_ * scene_draws : 0U) + scene_draws;
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &render_finished_[image_index];
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &image_index;
    result = vkQueuePresentKHR(present_queue_, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return recreate_swapchain();
    if (result != VK_SUCCESS) { log_error("vkQueuePresentKHR failed", result); return false; }
    last_cpu_frame_ms_ = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - cpu_frame_start).count();
    return true;
}

arut::rendering::RenderProfileSnapshot AndroidRenderer::profile_snapshot() const noexcept {
    arut::rendering::RenderProfileSnapshot snapshot{};
    snapshot.cpu_frame_ms = last_cpu_frame_ms_;
    snapshot.gpu_frame_ms = last_gpu_frame_ms_;
    snapshot.visibility_pass_ms = last_visibility_pass_ms_;
    snapshot.shadow_pass_ms = last_shadow_pass_ms_;
    snapshot.main_pass_ms = last_main_pass_ms_;
    snapshot.draw_calls = last_draw_calls_;
    snapshot.visible_objects = last_visible_objects_;
    snapshot.triangles = last_triangles_;
    snapshot.gpu_timing_available = timestamp_queries_available_;
    snapshot.gpu_visibility_enabled = gpu_visibility_enabled_;
    snapshot.gpu_memory_bytes = tracked_gpu_memory_bytes_;
    if (memory_budget_extension_enabled_ && instance_ != VK_NULL_HANDLE) {
        const auto get_memory_properties = reinterpret_cast<
            PFN_vkGetPhysicalDeviceMemoryProperties2KHR>(
                vkGetInstanceProcAddr(instance_, "vkGetPhysicalDeviceMemoryProperties2KHR"));
        if (get_memory_properties) {
            VkPhysicalDeviceMemoryBudgetPropertiesEXT budget{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};
            VkPhysicalDeviceMemoryProperties2 properties{
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
            properties.pNext = &budget;
            get_memory_properties(physical_device_, &properties);
            for (std::uint32_t heap = 0; heap < properties.memoryProperties.memoryHeapCount; ++heap) {
                if ((properties.memoryProperties.memoryHeaps[heap].flags &
                     VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == 0U) continue;
                snapshot.gpu_memory_budget_bytes += budget.heapBudget[heap];
                snapshot.gpu_memory_usage_bytes += budget.heapUsage[heap];
            }
            snapshot.gpu_memory_budget_available = true;
        }
    }
    return snapshot;
}

void AndroidRenderer::destroy_swapchain() noexcept {
    if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
    if (device_ != VK_NULL_HANDLE) {
        if (in_flight_) vkDestroyFence(device_, in_flight_, nullptr);
        if (timestamp_query_pool_) vkDestroyQueryPool(device_, timestamp_query_pool_, nullptr);
        if (visibility_compute_pipeline_) vkDestroyPipeline(device_, visibility_compute_pipeline_, nullptr);
        if (visibility_pipeline_layout_) vkDestroyPipelineLayout(device_, visibility_pipeline_layout_, nullptr);
        if (visibility_descriptor_pool_) vkDestroyDescriptorPool(device_, visibility_descriptor_pool_, nullptr);
        if (visibility_set_layout_) vkDestroyDescriptorSetLayout(device_, visibility_set_layout_, nullptr);
        if (visibility_input_buffer_) vkDestroyBuffer(device_, visibility_input_buffer_, nullptr);
        if (visibility_input_memory_) vkFreeMemory(device_, visibility_input_memory_, nullptr);
        if (shadow_pipeline_) vkDestroyPipeline(device_, shadow_pipeline_, nullptr);
        if (pbr_pipeline_) vkDestroyPipeline(device_, pbr_pipeline_, nullptr);
        if (pbr_descriptor_pool_) vkDestroyDescriptorPool(device_, pbr_descriptor_pool_, nullptr);
        if (pbr_pipeline_layout_) vkDestroyPipelineLayout(device_, pbr_pipeline_layout_, nullptr);
        if (frame_set_layout_) vkDestroyDescriptorSetLayout(device_, frame_set_layout_, nullptr);
        if (material_set_layout_) vkDestroyDescriptorSetLayout(device_, material_set_layout_, nullptr);
        if (frame_uniform_buffer_) vkDestroyBuffer(device_, frame_uniform_buffer_, nullptr);
        if (frame_uniform_memory_) vkFreeMemory(device_, frame_uniform_memory_, nullptr);
        if (material_uniform_buffer_) vkDestroyBuffer(device_, material_uniform_buffer_, nullptr);
        if (material_uniform_memory_) vkFreeMemory(device_, material_uniform_memory_, nullptr);
        if (environment_probe_uniform_buffer_) vkDestroyBuffer(device_, environment_probe_uniform_buffer_, nullptr);
        if (environment_probe_uniform_memory_) vkFreeMemory(device_, environment_probe_uniform_memory_, nullptr);
        if (bone_palette_uniform_buffer_) vkDestroyBuffer(device_, bone_palette_uniform_buffer_, nullptr);
        if (bone_palette_uniform_memory_) vkFreeMemory(device_, bone_palette_uniform_memory_, nullptr);
        bone_palette_uniform_buffer_ = VK_NULL_HANDLE;
        bone_palette_uniform_memory_ = VK_NULL_HANDLE;
        for (std::size_t probe = 0; probe < arut::rendering::kMaxEnvironmentProbes; ++probe) {
            if (environment_probe_specular_samplers_[probe]) vkDestroySampler(device_, environment_probe_specular_samplers_[probe], nullptr);
            if (environment_probe_specular_views_[probe]) vkDestroyImageView(device_, environment_probe_specular_views_[probe], nullptr);
            if (environment_probe_specular_images_[probe]) vkDestroyImage(device_, environment_probe_specular_images_[probe], nullptr);
            if (environment_probe_specular_memories_[probe]) vkFreeMemory(device_, environment_probe_specular_memories_[probe], nullptr);
            if (environment_probe_diffuse_samplers_[probe]) vkDestroySampler(device_, environment_probe_diffuse_samplers_[probe], nullptr);
            if (environment_probe_diffuse_views_[probe]) vkDestroyImageView(device_, environment_probe_diffuse_views_[probe], nullptr);
            if (environment_probe_diffuse_images_[probe]) vkDestroyImage(device_, environment_probe_diffuse_images_[probe], nullptr);
            if (environment_probe_diffuse_memories_[probe]) vkFreeMemory(device_, environment_probe_diffuse_memories_[probe], nullptr);
        }
        for (const auto& terrain : terrain_gpu_meshes_) {
            if (terrain.vertex_buffer) vkDestroyBuffer(device_, terrain.vertex_buffer, nullptr);
            if (terrain.vertex_memory) vkFreeMemory(device_, terrain.vertex_memory, nullptr);
            if (terrain.index_buffer) vkDestroyBuffer(device_, terrain.index_buffer, nullptr);
            if (terrain.index_memory) vkFreeMemory(device_, terrain.index_memory, nullptr);
            if (terrain.instance_buffer) vkDestroyBuffer(device_, terrain.instance_buffer, nullptr);
            if (terrain.instance_memory) vkFreeMemory(device_, terrain.instance_memory, nullptr);
            if (terrain.allocation_bytes <= tracked_gpu_memory_bytes_)
                tracked_gpu_memory_bytes_ -= terrain.allocation_bytes;
        }
        if (static_scene_instance_buffer_) vkDestroyBuffer(device_, static_scene_instance_buffer_, nullptr);
        if (static_scene_instance_memory_) vkFreeMemory(device_, static_scene_instance_memory_, nullptr);
        static_scene_instance_buffer_ = VK_NULL_HANDLE;
        static_scene_instance_memory_ = VK_NULL_HANDLE;
        static_scene_instance_count_ = 0U;
        if (mannequin_vertex_buffer_) vkDestroyBuffer(device_, mannequin_vertex_buffer_, nullptr);
        if (mannequin_vertex_memory_) vkFreeMemory(device_, mannequin_vertex_memory_, nullptr);
        if (mannequin_index_buffer_) vkDestroyBuffer(device_, mannequin_index_buffer_, nullptr);
        if (mannequin_index_memory_) vkFreeMemory(device_, mannequin_index_memory_, nullptr);
        if (mannequin_instance_buffer_) vkDestroyBuffer(device_, mannequin_instance_buffer_, nullptr);
        if (mannequin_instance_memory_) vkFreeMemory(device_, mannequin_instance_memory_, nullptr);
        if (imported_character_vertex_buffer_) vkDestroyBuffer(device_, imported_character_vertex_buffer_, nullptr);
        if (imported_character_vertex_memory_) vkFreeMemory(device_, imported_character_vertex_memory_, nullptr);
        if (imported_character_index_buffer_) vkDestroyBuffer(device_, imported_character_index_buffer_, nullptr);
        if (imported_character_index_memory_) vkFreeMemory(device_, imported_character_index_memory_, nullptr);
        mannequin_vertex_buffer_ = mannequin_index_buffer_ = mannequin_instance_buffer_ = VK_NULL_HANDLE;
        imported_character_vertex_buffer_ = imported_character_index_buffer_ = VK_NULL_HANDLE;
        imported_character_vertex_memory_ = imported_character_index_memory_ = VK_NULL_HANDLE;
        imported_character_index_count_ = 0U;
        imported_character_skeleton_signature_ = 0U;
        imported_character_enabled_ = false;
        mannequin_vertex_memory_ = mannequin_index_memory_ = mannequin_instance_memory_ = VK_NULL_HANDLE;
        mannequin_index_count_ = 0U;
        if (cube_vertex_buffer_) vkDestroyBuffer(device_, cube_vertex_buffer_, nullptr);
        if (cube_vertex_memory_) vkFreeMemory(device_, cube_vertex_memory_, nullptr);
        if (cube_index_buffer_) vkDestroyBuffer(device_, cube_index_buffer_, nullptr);
        if (cube_index_memory_) vkFreeMemory(device_, cube_index_memory_, nullptr);
        if (instance_buffer_) vkDestroyBuffer(device_, instance_buffer_, nullptr);
        if (instance_memory_) vkFreeMemory(device_, instance_memory_, nullptr);
        if (indirect_draw_buffer_) vkDestroyBuffer(device_, indirect_draw_buffer_, nullptr);
        if (indirect_draw_memory_) vkFreeMemory(device_, indirect_draw_memory_, nullptr);
        if (brdf_lut_sampler_) vkDestroySampler(device_, brdf_lut_sampler_, nullptr);
        if (brdf_lut_view_) vkDestroyImageView(device_, brdf_lut_view_, nullptr);
        if (brdf_lut_image_) vkDestroyImage(device_, brdf_lut_image_, nullptr);
        if (brdf_lut_memory_) vkFreeMemory(device_, brdf_lut_memory_, nullptr);
        if (irradiance_sampler_) vkDestroySampler(device_, irradiance_sampler_, nullptr);
        if (irradiance_view_) vkDestroyImageView(device_, irradiance_view_, nullptr);
        if (irradiance_image_) vkDestroyImage(device_, irradiance_image_, nullptr);
        if (irradiance_memory_) vkFreeMemory(device_, irradiance_memory_, nullptr);
        if (environment_sampler_) vkDestroySampler(device_, environment_sampler_, nullptr);
        if (environment_view_) vkDestroyImageView(device_, environment_view_, nullptr);
        if (environment_image_) vkDestroyImage(device_, environment_image_, nullptr);
        if (environment_memory_) vkFreeMemory(device_, environment_memory_, nullptr);
        if (normal_sampler_) vkDestroySampler(device_, normal_sampler_, nullptr);
        if (normal_view_) vkDestroyImageView(device_, normal_view_, nullptr);
        if (normal_image_) vkDestroyImage(device_, normal_image_, nullptr);
        if (normal_memory_) vkFreeMemory(device_, normal_memory_, nullptr);
        if (metallic_roughness_sampler_) vkDestroySampler(device_, metallic_roughness_sampler_, nullptr);
        if (metallic_roughness_view_) vkDestroyImageView(device_, metallic_roughness_view_, nullptr);
        if (metallic_roughness_image_) vkDestroyImage(device_, metallic_roughness_image_, nullptr);
        if (metallic_roughness_memory_) vkFreeMemory(device_, metallic_roughness_memory_, nullptr);
        if (base_color_sampler_) vkDestroySampler(device_, base_color_sampler_, nullptr);
        if (base_color_view_) vkDestroyImageView(device_, base_color_view_, nullptr);
        if (base_color_image_) vkDestroyImage(device_, base_color_image_, nullptr);
        if (base_color_memory_) vkFreeMemory(device_, base_color_memory_, nullptr);
        if (fallback_sampler_) vkDestroySampler(device_, fallback_sampler_, nullptr);
        if (fallback_view_) vkDestroyImageView(device_, fallback_view_, nullptr);
        if (fallback_image_) vkDestroyImage(device_, fallback_image_, nullptr);
        if (fallback_memory_) vkFreeMemory(device_, fallback_memory_, nullptr);
        if (graphics_pipeline_) vkDestroyPipeline(device_, graphics_pipeline_, nullptr);
        if (pipeline_layout_) vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
        if (triangle_vertex_buffer_) vkDestroyBuffer(device_, triangle_vertex_buffer_, nullptr);
        if (triangle_vertex_memory_) vkFreeMemory(device_, triangle_vertex_memory_, nullptr);
        if (image_available_) vkDestroySemaphore(device_, image_available_, nullptr);
        for (auto semaphore : render_finished_) if (semaphore) vkDestroySemaphore(device_, semaphore, nullptr);
        if (command_pool_) vkDestroyCommandPool(device_, command_pool_, nullptr);
        for (auto fb : framebuffers_) if (fb) vkDestroyFramebuffer(device_, fb, nullptr);
        if (render_pass_) vkDestroyRenderPass(device_, render_pass_, nullptr);
        for (auto view : image_views_) if (view) vkDestroyImageView(device_, view, nullptr);
        if (depth_view_) vkDestroyImageView(device_, depth_view_, nullptr);
        if (depth_image_) vkDestroyImage(device_, depth_image_, nullptr);
        if (depth_memory_) vkFreeMemory(device_, depth_memory_, nullptr);
        for (auto framebuffer : shadow_framebuffers_)
            if (framebuffer) vkDestroyFramebuffer(device_, framebuffer, nullptr);
        for (auto layer_view : shadow_layer_views_)
            if (layer_view) vkDestroyImageView(device_, layer_view, nullptr);
        if (shadow_render_pass_) vkDestroyRenderPass(device_, shadow_render_pass_, nullptr);
        if (shadow_sampler_) vkDestroySampler(device_, shadow_sampler_, nullptr);
        if (shadow_view_) vkDestroyImageView(device_, shadow_view_, nullptr);
        if (shadow_image_) vkDestroyImage(device_, shadow_image_, nullptr);
        if (shadow_memory_) vkFreeMemory(device_, shadow_memory_, nullptr);
        if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }
    in_flight_ = VK_NULL_HANDLE;
    timestamp_query_pool_ = VK_NULL_HANDLE;
    timestamp_data_pending_ = false;
    gpu_visibility_result_pending_ = false;
    submitted_cpu_expected_visible_ = 0U;
    image_available_ = VK_NULL_HANDLE;
    render_finished_.clear();
    command_pool_ = VK_NULL_HANDLE;
    render_pass_ = VK_NULL_HANDLE;
    graphics_pipeline_ = VK_NULL_HANDLE;
    pipeline_layout_ = VK_NULL_HANDLE;
    pbr_pipeline_ = VK_NULL_HANDLE;
    pbr_pipeline_layout_ = VK_NULL_HANDLE;
    pbr_descriptor_pool_ = VK_NULL_HANDLE;
    frame_set_layout_ = material_set_layout_ = VK_NULL_HANDLE;
    frame_descriptor_set_ = material_descriptor_set_ = VK_NULL_HANDLE;
    frame_uniform_buffer_ = material_uniform_buffer_ = VK_NULL_HANDLE;
    frame_uniform_memory_ = material_uniform_memory_ = VK_NULL_HANDLE;
    environment_probe_uniform_buffer_ = VK_NULL_HANDLE;
    environment_probe_uniform_memory_ = VK_NULL_HANDLE;
    environment_probe_specular_images_.fill(VK_NULL_HANDLE);
    environment_probe_specular_memories_.fill(VK_NULL_HANDLE);
    environment_probe_specular_views_.fill(VK_NULL_HANDLE);
    environment_probe_specular_samplers_.fill(VK_NULL_HANDLE);
    environment_probe_specular_mip_levels_.fill(0U);
    environment_probe_diffuse_images_.fill(VK_NULL_HANDLE);
    environment_probe_diffuse_memories_.fill(VK_NULL_HANDLE);
    environment_probe_diffuse_views_.fill(VK_NULL_HANDLE);
    environment_probe_diffuse_samplers_.fill(VK_NULL_HANDLE);
    environment_probe_diffuse_mip_levels_.fill(0U);
    terrain_gpu_meshes_.clear();
    terrain_meshes_.clear();
    terrain_triangle_count_ = 0U;
    cube_vertex_buffer_ = cube_index_buffer_ = VK_NULL_HANDLE;
    cube_vertex_memory_ = cube_index_memory_ = VK_NULL_HANDLE;
    instance_buffer_ = VK_NULL_HANDLE;
    instance_memory_ = VK_NULL_HANDLE;
    indirect_draw_buffer_ = VK_NULL_HANDLE;
    indirect_draw_memory_ = VK_NULL_HANDLE;
    cube_index_count_ = 0;
    instance_count_ = 0;
    brdf_lut_sampler_ = VK_NULL_HANDLE;
    brdf_lut_view_ = VK_NULL_HANDLE;
    brdf_lut_image_ = VK_NULL_HANDLE;
    brdf_lut_memory_ = VK_NULL_HANDLE;
    brdf_lut_mip_levels_ = 0;
    irradiance_sampler_ = VK_NULL_HANDLE;
    irradiance_view_ = VK_NULL_HANDLE;
    irradiance_image_ = VK_NULL_HANDLE;
    irradiance_memory_ = VK_NULL_HANDLE;
    irradiance_mip_levels_ = 0;
    environment_sampler_ = VK_NULL_HANDLE;
    environment_view_ = VK_NULL_HANDLE;
    environment_image_ = VK_NULL_HANDLE;
    environment_memory_ = VK_NULL_HANDLE;
    environment_mip_levels_ = 0;
    normal_sampler_ = VK_NULL_HANDLE;
    normal_view_ = VK_NULL_HANDLE;
    normal_image_ = VK_NULL_HANDLE;
    normal_memory_ = VK_NULL_HANDLE;
    normal_mip_levels_ = 0;
    metallic_roughness_sampler_ = VK_NULL_HANDLE;
    metallic_roughness_view_ = VK_NULL_HANDLE;
    metallic_roughness_image_ = VK_NULL_HANDLE;
    metallic_roughness_memory_ = VK_NULL_HANDLE;
    metallic_roughness_mip_levels_ = 0;
    base_color_sampler_ = VK_NULL_HANDLE;
    base_color_view_ = VK_NULL_HANDLE;
    base_color_image_ = VK_NULL_HANDLE;
    base_color_memory_ = VK_NULL_HANDLE;
    base_color_mip_levels_ = 0;
    fallback_sampler_ = VK_NULL_HANDLE;
    fallback_mip_levels_ = 0;
    fallback_view_ = VK_NULL_HANDLE;
    fallback_image_ = VK_NULL_HANDLE;
    fallback_memory_ = VK_NULL_HANDLE;
    triangle_vertex_buffer_ = VK_NULL_HANDLE;
    triangle_vertex_memory_ = VK_NULL_HANDLE;
    depth_view_ = VK_NULL_HANDLE;
    depth_image_ = VK_NULL_HANDLE;
    depth_memory_ = VK_NULL_HANDLE;
    depth_format_ = VK_FORMAT_UNDEFINED;
    shadow_format_ = VK_FORMAT_UNDEFINED;
    shadow_image_ = VK_NULL_HANDLE;
    shadow_memory_ = VK_NULL_HANDLE;
    shadow_view_ = VK_NULL_HANDLE;
    shadow_sampler_ = VK_NULL_HANDLE;
    shadow_render_pass_ = VK_NULL_HANDLE;
    shadow_framebuffers_.fill(VK_NULL_HANDLE);
    shadow_layer_views_.fill(VK_NULL_HANDLE);
    shadow_pipeline_ = VK_NULL_HANDLE;
    visibility_compute_pipeline_ = VK_NULL_HANDLE;
    visibility_pipeline_layout_ = VK_NULL_HANDLE;
    visibility_descriptor_pool_ = VK_NULL_HANDLE;
    visibility_descriptor_set_ = VK_NULL_HANDLE;
    visibility_set_layout_ = VK_NULL_HANDLE;
    visibility_input_buffer_ = VK_NULL_HANDLE;
    visibility_input_memory_ = VK_NULL_HANDLE;
    visibility_input_info_ = {};
    visibility_source_instance_count_ = 0;
    visibility_frustum_planes_.fill(0.0F);
    gpu_visibility_enabled_ = false;
    shadow_enabled_ = false;
    swapchain_ = VK_NULL_HANDLE;
    framebuffers_.clear();
    image_views_.clear();
    command_buffers_.clear();
    images_.clear();
    tracked_gpu_memory_bytes_ = 0;
    mannequin_index_count_ = 0U;
    mannequin_vertex_buffer_ = mannequin_index_buffer_ = mannequin_instance_buffer_ = VK_NULL_HANDLE;
    mannequin_vertex_memory_ = mannequin_index_memory_ = mannequin_instance_memory_ = VK_NULL_HANDLE;
}

bool AndroidRenderer::recreate_swapchain() noexcept {
    if (!device_) return false;
    destroy_swapchain();
    return create_swapchain() && create_frame_resources();
}

bool AndroidRenderer::set_environment_probes(
    std::span<const arut::rendering::EnvironmentProbeInfluence> probes) noexcept {
    if (probes.size() > arut::rendering::kMaxEnvironmentProbes ||
        device_ == VK_NULL_HANDLE ||
        environment_probe_uniform_buffer_ == VK_NULL_HANDLE ||
        environment_probe_uniform_memory_ == VK_NULL_HANDLE) {
        return false;
    }

    arut::rendering::EnvironmentProbeGpuData gpu_data{};
    if (!arut::rendering::build_environment_probe_gpu_data(probes, gpu_data)) {
        log_error("Rejected invalid environment-probe registration data");
        return false;
    }

    const VkResult idle = vkDeviceWaitIdle(device_);
    if (idle != VK_SUCCESS) {
        log_error("Wait for device idle before updating environment probes failed", idle);
        return false;
    }
    void* mapped = nullptr;
    const VkResult map_result = vkMapMemory(
        device_, environment_probe_uniform_memory_, 0, sizeof(gpu_data), 0, &mapped);
    if (map_result != VK_SUCCESS || !mapped) {
        log_error("Map environment-probe uniform buffer for update failed", map_result);
        return false;
    }
    std::memcpy(mapped, &gpu_data, sizeof(gpu_data));
    vkUnmapMemory(device_, environment_probe_uniform_memory_);

    environment_probe_influences_.fill({});
    std::copy(probes.begin(), probes.end(), environment_probe_influences_.begin());
    environment_probe_count_ = static_cast<std::uint32_t>(probes.size());
    arut_android_log(ANDROID_LOG_INFO, "Runtime environment-probe registration updated.");
    return true;
}

bool AndroidRenderer::set_quality_tier(arut::rendering::RenderQualityTier tier) noexcept {
    quality_ = arut::rendering::quality_config(tier);
    if (device_ == VK_NULL_HANDLE || window_ == nullptr || assets_ == nullptr) return true;
    const VkResult idle = vkDeviceWaitIdle(device_);
    if (idle != VK_SUCCESS) {
        log_error("Wait for device idle before changing quality tier failed", idle);
        return false;
    }
    destroy_swapchain();
    if (create_swapchain() && create_frame_resources()) {
        arut_android_log(ANDROID_LOG_INFO, "Renderer quality tier changed and GPU resources recreated.");
        return true;
    }
    log_error("Renderer resource recreation after quality-tier change failed");
    shutdown();
    return false;
}

void AndroidRenderer::shutdown() noexcept {
    destroy_swapchain();
    if (device_) vkDestroyDevice(device_, nullptr);
    if (surface_ && instance_) vkDestroySurfaceKHR(instance_, surface_, nullptr);
    if (instance_) vkDestroyInstance(instance_, nullptr);
    device_ = VK_NULL_HANDLE;
    surface_ = VK_NULL_HANDLE;
    instance_ = VK_NULL_HANDLE;
    window_ = nullptr;
    assets_ = nullptr;
    physical_device_ = VK_NULL_HANDLE;
    graphics_queue_ = VK_NULL_HANDLE;
    present_queue_ = VK_NULL_HANDLE;
    graphics_family_ = present_family_ = UINT32_MAX;
}

} // namespace arut::rendering::vulkan
