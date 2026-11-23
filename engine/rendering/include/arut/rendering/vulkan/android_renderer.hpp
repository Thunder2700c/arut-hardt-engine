#pragma once

#include <android/native_window.h>
#include <android/asset_manager.h>
#include <vulkan/vulkan.h>
#include "arut/rendering/image.hpp"
#include "arut/rendering/render_quality.hpp"
#include "arut/rendering/environment.hpp"
#include "arut/rendering/gpu_visibility.hpp"
#include "arut/rendering/environment_probe.hpp"
#include "arut/core/cube_stress_scene.hpp"
#include "arut/core/world_systems.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <cstddef>
#include <chrono>
#include <vector>

namespace arut::rendering::vulkan {

class AndroidRenderer final {
public:
    AndroidRenderer() = default;
    ~AndroidRenderer();
    AndroidRenderer(const AndroidRenderer&) = delete;
    AndroidRenderer& operator=(const AndroidRenderer&) = delete;

    bool initialize(ANativeWindow* window, AAssetManager* assets,
                    std::span<const arut::core::CubeRenderInstance> ecs_instances,
                    std::span<const arut::core::TerrainMesh> terrain_meshes,
                    bool render_mannequin = true) noexcept;
    void shutdown() noexcept;
    bool set_quality_tier(arut::rendering::RenderQualityTier tier) noexcept;
    bool set_environment_probes(std::span<const arut::rendering::EnvironmentProbeInfluence> probes) noexcept;
    bool draw_frame() noexcept;
    bool set_terrain_cells(std::span<const arut::core::TerrainMesh> terrain_meshes) noexcept;
    [[nodiscard]] std::array<float, 3> camera_position() const noexcept { return camera_position_; }
    // action: 0=down, 1=move, 2=up. Coordinates are in window pixels.
    void set_touch_input(std::int32_t action, std::int32_t pointer_id,
                         float x, float y, float width, float height) noexcept;
    [[nodiscard]] arut::rendering::RenderProfileSnapshot profile_snapshot() const noexcept;
    bool ready() const noexcept { return device_ != VK_NULL_HANDLE && swapchain_ != VK_NULL_HANDLE; }

private:
    struct CubeUploadMip final {
        std::uint32_t width{0};
        std::uint32_t height{0};
        const void* pixels{nullptr};
        std::size_t byte_size{0};
    };
    struct TerrainGpuMesh final {
        VkBuffer vertex_buffer{VK_NULL_HANDLE};
        VkDeviceMemory vertex_memory{VK_NULL_HANDLE};
        VkBuffer index_buffer{VK_NULL_HANDLE};
        VkDeviceMemory index_memory{VK_NULL_HANDLE};
        VkBuffer instance_buffer{VK_NULL_HANDLE};
        VkDeviceMemory instance_memory{VK_NULL_HANDLE};
        std::uint32_t index_count{0};
        VkDeviceSize allocation_bytes{0};
    };

    bool create_instance() noexcept;
    bool create_surface(ANativeWindow* window) noexcept;
    bool select_device() noexcept;
    bool create_device() noexcept;
    bool create_swapchain() noexcept;
    bool create_render_targets() noexcept;
    bool create_depth_resources() noexcept;
    std::uint32_t find_memory_type(std::uint32_t type_bits, VkMemoryPropertyFlags properties) const noexcept;
    bool create_commands_and_sync() noexcept;
    bool create_frame_resources() noexcept;
    bool create_gpu_visibility_resources() noexcept;
    bool record_gpu_visibility(VkCommandBuffer command_buffer) noexcept;
    void update_stress_camera(float delta_seconds) noexcept;
    bool create_triangle_pipeline() noexcept;
    bool create_triangle_geometry() noexcept;
    bool create_pbr_pipeline() noexcept;
    bool create_pbr_geometry() noexcept;
    bool create_mannequin_geometry() noexcept;
    bool create_terrain_geometry() noexcept;
    bool create_pbr_descriptors() noexcept;
    bool create_shadow_resources() noexcept;
    bool create_shadow_pipeline() noexcept;
    bool create_environment_cubemap() noexcept;
    bool create_environment_probe_cubemaps() noexcept;
    bool load_environment_probe_cubemap(const char* path,
        VkImage& specular_image, VkDeviceMemory& specular_memory,
        VkImageView& specular_view, VkSampler& specular_sampler,
        std::uint32_t& specular_mip_levels,
        VkImage& diffuse_image, VkDeviceMemory& diffuse_memory,
        VkImageView& diffuse_view, VkSampler& diffuse_sampler,
        std::uint32_t& diffuse_mip_levels) noexcept;
    bool load_environment_hdr_asset(const char* path, arut::rendering::Rgba32fImage& decoded) noexcept;
    bool upload_environment_cube(const std::vector<arut::rendering::EnvironmentCubeMip>& mips, VkImage& image, VkDeviceMemory& memory, VkImageView& view, VkSampler& sampler, std::uint32_t& mip_levels) noexcept;
    bool upload_environment_cube_rgba16f(const std::vector<arut::rendering::EnvironmentCubeF16Mip>& mips, VkImage& image, VkDeviceMemory& memory, VkImageView& view, VkSampler& sampler, std::uint32_t& mip_levels) noexcept;
    bool upload_environment_cube_raw(const std::vector<CubeUploadMip>& mips, VkFormat format, VkDeviceSize bytes_per_pixel,
                                     VkImage& image, VkDeviceMemory& memory, VkImageView& view,
                                     VkSampler& sampler, std::uint32_t& mip_levels) noexcept;
    bool create_brdf_lut() noexcept;
    bool create_fallback_texture() noexcept;
    bool load_texture_asset(const char* path, VkFormat format, bool source_is_srgb,
                            VkImage& image, VkDeviceMemory& memory, VkImageView& view,
                            VkSampler& sampler, std::uint32_t& mip_levels) noexcept;
    bool upload_texture_rgba8(const Rgba8Image& pixels, VkFormat format, bool source_is_srgb,
                              VkImage& image, VkDeviceMemory& memory, VkImageView& view,
                              VkSampler& sampler, std::uint32_t& mip_levels) noexcept;
    bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory) noexcept;
    void account_gpu_allocation(VkDeviceSize bytes) noexcept;
    void destroy_swapchain() noexcept;
    bool recreate_swapchain() noexcept;

    ANativeWindow* window_{nullptr};
    AAssetManager* assets_{nullptr};
    VkInstance instance_{VK_NULL_HANDLE};
    VkSurfaceKHR surface_{VK_NULL_HANDLE};
    VkPhysicalDevice physical_device_{VK_NULL_HANDLE};
    VkDevice device_{VK_NULL_HANDLE};
    VkQueue graphics_queue_{VK_NULL_HANDLE};
    VkQueue present_queue_{VK_NULL_HANDLE};
    std::uint32_t graphics_family_{UINT32_MAX};
    std::uint32_t present_family_{UINT32_MAX};
    VkSwapchainKHR swapchain_{VK_NULL_HANDLE};
    VkFormat color_format_{VK_FORMAT_UNDEFINED};
    VkExtent2D extent_{};
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    VkRenderPass render_pass_{VK_NULL_HANDLE};
    VkPipelineLayout pipeline_layout_{VK_NULL_HANDLE};
    VkPipeline graphics_pipeline_{VK_NULL_HANDLE};
    VkBuffer triangle_vertex_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory triangle_vertex_memory_{VK_NULL_HANDLE};
    VkPipeline pbr_pipeline_{VK_NULL_HANDLE};
    VkPipelineLayout pbr_pipeline_layout_{VK_NULL_HANDLE};
    VkDescriptorSetLayout frame_set_layout_{VK_NULL_HANDLE};
    VkDescriptorSetLayout material_set_layout_{VK_NULL_HANDLE};
    VkDescriptorPool pbr_descriptor_pool_{VK_NULL_HANDLE};
    VkDescriptorSet frame_descriptor_set_{VK_NULL_HANDLE};
    VkDescriptorSet material_descriptor_set_{VK_NULL_HANDLE};
    VkBuffer frame_uniform_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory frame_uniform_memory_{VK_NULL_HANDLE};
    VkBuffer material_uniform_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory material_uniform_memory_{VK_NULL_HANDLE};
    VkBuffer environment_probe_uniform_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory environment_probe_uniform_memory_{VK_NULL_HANDLE};
    std::array<arut::rendering::EnvironmentProbeInfluence,
               arut::rendering::kMaxEnvironmentProbes> environment_probe_influences_{{
        arut::rendering::EnvironmentProbeInfluence{
            arut::rendering::EnvironmentProbeAabb{-4.0F, -4.0F, -4.0F, 4.0F, 4.0F, 4.0F},
            4.0F, 0},
        arut::rendering::EnvironmentProbeInfluence{
            arut::rendering::EnvironmentProbeAabb{-1.6F, -2.0F, -1.6F, 1.6F, 2.0F, 1.6F},
            0.65F, 1}
    }};
    std::uint32_t environment_probe_count_{2U};
    std::vector<arut::core::TerrainMesh> terrain_meshes_{};
    std::vector<TerrainGpuMesh> terrain_gpu_meshes_{};
    std::uint32_t terrain_triangle_count_{0};
    VkBuffer mannequin_vertex_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory mannequin_vertex_memory_{VK_NULL_HANDLE};
    VkBuffer mannequin_index_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory mannequin_index_memory_{VK_NULL_HANDLE};
    VkBuffer mannequin_instance_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory mannequin_instance_memory_{VK_NULL_HANDLE};
    std::uint32_t mannequin_index_count_{0};
    bool mannequin_scene_enabled_{true};
    VkBuffer cube_vertex_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory cube_vertex_memory_{VK_NULL_HANDLE};
    VkBuffer cube_index_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory cube_index_memory_{VK_NULL_HANDLE};
    VkBuffer instance_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory instance_memory_{VK_NULL_HANDLE};
    VkBuffer indirect_draw_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory indirect_draw_memory_{VK_NULL_HANDLE};
    std::uint32_t cube_index_count_{0};
    std::uint32_t instance_count_{0};
    VkImage fallback_image_{VK_NULL_HANDLE};
    VkDeviceMemory fallback_memory_{VK_NULL_HANDLE};
    VkImageView fallback_view_{VK_NULL_HANDLE};
    VkSampler fallback_sampler_{VK_NULL_HANDLE};
    std::uint32_t fallback_mip_levels_{0};
    VkImage base_color_image_{VK_NULL_HANDLE};
    VkDeviceMemory base_color_memory_{VK_NULL_HANDLE};
    VkImageView base_color_view_{VK_NULL_HANDLE};
    VkSampler base_color_sampler_{VK_NULL_HANDLE};
    VkImage environment_image_{VK_NULL_HANDLE};
    VkDeviceMemory environment_memory_{VK_NULL_HANDLE};
    VkImageView environment_view_{VK_NULL_HANDLE};
    VkSampler environment_sampler_{VK_NULL_HANDLE};
    VkImage brdf_lut_image_{VK_NULL_HANDLE};
    VkDeviceMemory brdf_lut_memory_{VK_NULL_HANDLE};
    VkImageView brdf_lut_view_{VK_NULL_HANDLE};
    VkSampler brdf_lut_sampler_{VK_NULL_HANDLE};
    std::uint32_t brdf_lut_mip_levels_{0};
    std::uint32_t environment_mip_levels_{0};
    std::array<VkImage, arut::rendering::kMaxEnvironmentProbes> environment_probe_specular_images_{};
    std::array<VkDeviceMemory, arut::rendering::kMaxEnvironmentProbes> environment_probe_specular_memories_{};
    std::array<VkImageView, arut::rendering::kMaxEnvironmentProbes> environment_probe_specular_views_{};
    std::array<VkSampler, arut::rendering::kMaxEnvironmentProbes> environment_probe_specular_samplers_{};
    std::array<std::uint32_t, arut::rendering::kMaxEnvironmentProbes> environment_probe_specular_mip_levels_{};
    std::array<VkImage, arut::rendering::kMaxEnvironmentProbes> environment_probe_diffuse_images_{};
    std::array<VkDeviceMemory, arut::rendering::kMaxEnvironmentProbes> environment_probe_diffuse_memories_{};
    std::array<VkImageView, arut::rendering::kMaxEnvironmentProbes> environment_probe_diffuse_views_{};
    std::array<VkSampler, arut::rendering::kMaxEnvironmentProbes> environment_probe_diffuse_samplers_{};
    std::array<std::uint32_t, arut::rendering::kMaxEnvironmentProbes> environment_probe_diffuse_mip_levels_{};
    VkImage irradiance_image_{VK_NULL_HANDLE};
    VkDeviceMemory irradiance_memory_{VK_NULL_HANDLE};
    VkImageView irradiance_view_{VK_NULL_HANDLE};
    VkSampler irradiance_sampler_{VK_NULL_HANDLE};
    std::uint32_t irradiance_mip_levels_{0};
    std::uint32_t base_color_mip_levels_{0};
    VkImage normal_image_{VK_NULL_HANDLE};
    VkDeviceMemory normal_memory_{VK_NULL_HANDLE};
    VkImageView normal_view_{VK_NULL_HANDLE};
    VkSampler normal_sampler_{VK_NULL_HANDLE};
    std::uint32_t normal_mip_levels_{0};
    VkImage metallic_roughness_image_{VK_NULL_HANDLE};
    VkDeviceMemory metallic_roughness_memory_{VK_NULL_HANDLE};
    VkImageView metallic_roughness_view_{VK_NULL_HANDLE};
    VkSampler metallic_roughness_sampler_{VK_NULL_HANDLE};
    std::uint32_t metallic_roughness_mip_levels_{0};
    VkImage depth_image_{VK_NULL_HANDLE};
    VkDeviceMemory depth_memory_{VK_NULL_HANDLE};
    VkImageView depth_view_{VK_NULL_HANDLE};
    VkFormat depth_format_{VK_FORMAT_UNDEFINED};
    VkFormat shadow_format_{VK_FORMAT_UNDEFINED};
    arut::rendering::RenderQualityConfig quality_{arut::rendering::quality_config(arut::rendering::RenderQualityTier::Medium)};
    VkExtent2D shadow_extent_{1024U, 1024U};
    std::uint32_t shadow_cascade_count_{1U};
    VkImage shadow_image_{VK_NULL_HANDLE};
    VkDeviceMemory shadow_memory_{VK_NULL_HANDLE};
    VkImageView shadow_view_{VK_NULL_HANDLE};
    VkSampler shadow_sampler_{VK_NULL_HANDLE};
    VkRenderPass shadow_render_pass_{VK_NULL_HANDLE};
    std::array<VkImageView, 4> shadow_layer_views_{};
    std::array<VkFramebuffer, 4> shadow_framebuffers_{};
    VkPipeline shadow_pipeline_{VK_NULL_HANDLE};
    VkDescriptorSetLayout visibility_set_layout_{VK_NULL_HANDLE};
    VkDescriptorPool visibility_descriptor_pool_{VK_NULL_HANDLE};
    VkDescriptorSet visibility_descriptor_set_{VK_NULL_HANDLE};
    VkPipelineLayout visibility_pipeline_layout_{VK_NULL_HANDLE};
    VkPipeline visibility_compute_pipeline_{VK_NULL_HANDLE};
    VkBuffer visibility_input_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory visibility_input_memory_{VK_NULL_HANDLE};
    VkDescriptorBufferInfo visibility_input_info_{};
    std::uint32_t visibility_source_instance_count_{0};
    std::vector<arut::rendering::GpuVisibilityInstance> visibility_instances_{};
    std::array<float, 3> camera_position_{0.0F, 0.0F, 18.0F};
    float camera_yaw_{-1.570796327F};
    float camera_pitch_{0.0F};
    float joystick_x_{0.0F};
    float joystick_y_{0.0F};
    float joystick_origin_x_{0.0F};
    float joystick_origin_y_{0.0F};
    float joystick_radius_{72.0F};
    std::int32_t joystick_pointer_{-1};
    std::int32_t look_pointer_{-1};
    float look_last_x_{0.0F};
    float look_last_y_{0.0F};
    std::uint32_t cpu_expected_visible_{0};
    // CPU count captured for the exact frame whose GPU indirect count is read back next frame.
    std::uint32_t submitted_cpu_expected_visible_{0};
    bool gpu_visibility_result_pending_{false};
    std::uint64_t culling_parity_checks_{0};
    std::uint64_t culling_parity_mismatches_{0};
    std::chrono::steady_clock::time_point last_camera_update_{};
    std::array<float, 24> visibility_frustum_planes_{};
    bool compute_queue_supported_{false};
    bool properties2_extension_enabled_{false};
    bool memory_budget_extension_enabled_{false};
    bool gpu_visibility_enabled_{false};
    bool shadow_enabled_{false};
    std::vector<VkFramebuffer> framebuffers_;
    VkCommandPool command_pool_{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> command_buffers_;
    VkSemaphore image_available_{VK_NULL_HANDLE};
    std::vector<VkSemaphore> render_finished_;
    VkFence in_flight_{VK_NULL_HANDLE};
    VkQueryPool timestamp_query_pool_{VK_NULL_HANDLE};
    bool timestamp_queries_available_{false};
    bool timestamp_data_pending_{false};
    std::uint32_t timestamp_valid_bits_{0};
    double timestamp_period_ns_{0.0};
    double last_cpu_frame_ms_{0.0};
    double last_gpu_frame_ms_{0.0};
    double last_visibility_pass_ms_{0.0};
    double last_shadow_pass_ms_{0.0};
    double last_main_pass_ms_{0.0};
    std::uint64_t tracked_gpu_memory_bytes_{0};
    std::uint32_t last_draw_calls_{0};
    std::uint32_t last_visible_objects_{0};
    std::uint32_t last_triangles_{0};
};

} // namespace arut::rendering::vulkan
