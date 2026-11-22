#pragma once

#include <android/native_window.h>
#include <android/asset_manager.h>
#include <vulkan/vulkan.h>
#include "arut/rendering/image.hpp"
#include "arut/rendering/render_quality.hpp"
#include "arut/rendering/environment.hpp"
#include <cstdint>
#include <vector>

namespace arut::rendering::vulkan {

class AndroidRenderer final {
public:
    AndroidRenderer() = default;
    ~AndroidRenderer();
    AndroidRenderer(const AndroidRenderer&) = delete;
    AndroidRenderer& operator=(const AndroidRenderer&) = delete;

    bool initialize(ANativeWindow* window, AAssetManager* assets) noexcept;
    void shutdown() noexcept;
    bool draw_frame() noexcept;
    [[nodiscard]] arut::rendering::RenderProfileSnapshot profile_snapshot() const noexcept;
    bool ready() const noexcept { return device_ != VK_NULL_HANDLE && swapchain_ != VK_NULL_HANDLE; }

private:
    bool create_instance() noexcept;
    bool create_surface(ANativeWindow* window) noexcept;
    bool select_device() noexcept;
    bool create_device() noexcept;
    bool create_swapchain() noexcept;
    bool create_render_targets() noexcept;
    bool create_depth_resources() noexcept;
    std::uint32_t find_memory_type(std::uint32_t type_bits, VkMemoryPropertyFlags properties) const noexcept;
    bool create_commands_and_sync() noexcept;
    bool create_triangle_pipeline() noexcept;
    bool create_triangle_geometry() noexcept;
    bool create_pbr_pipeline() noexcept;
    bool create_pbr_geometry() noexcept;
    bool create_pbr_descriptors() noexcept;
    bool create_shadow_resources() noexcept;
    bool create_shadow_pipeline() noexcept;
    bool create_environment_cubemap() noexcept;
    bool create_fallback_texture() noexcept;
    bool load_texture_asset(const char* path, VkFormat format, bool source_is_srgb,
                            VkImage& image, VkDeviceMemory& memory, VkImageView& view,
                            VkSampler& sampler, std::uint32_t& mip_levels) noexcept;
    bool upload_texture_rgba8(const Rgba8Image& pixels, VkFormat format, bool source_is_srgb,
                              VkImage& image, VkDeviceMemory& memory, VkImageView& view,
                              VkSampler& sampler, std::uint32_t& mip_levels) noexcept;
    bool create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer& buffer, VkDeviceMemory& memory) noexcept;
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
    VkBuffer cube_vertex_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory cube_vertex_memory_{VK_NULL_HANDLE};
    VkBuffer cube_index_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory cube_index_memory_{VK_NULL_HANDLE};
    VkBuffer instance_buffer_{VK_NULL_HANDLE};
    VkDeviceMemory instance_memory_{VK_NULL_HANDLE};
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
    std::uint32_t environment_mip_levels_{0};
    std::uint32_t base_color_mip_levels_{0};
    VkImage depth_image_{VK_NULL_HANDLE};
    VkDeviceMemory depth_memory_{VK_NULL_HANDLE};
    VkImageView depth_view_{VK_NULL_HANDLE};
    VkFormat depth_format_{VK_FORMAT_UNDEFINED};
    VkFormat shadow_format_{VK_FORMAT_UNDEFINED};
    VkExtent2D shadow_extent_{1024U, 1024U};
    VkImage shadow_image_{VK_NULL_HANDLE};
    VkDeviceMemory shadow_memory_{VK_NULL_HANDLE};
    VkImageView shadow_view_{VK_NULL_HANDLE};
    VkSampler shadow_sampler_{VK_NULL_HANDLE};
    VkRenderPass shadow_render_pass_{VK_NULL_HANDLE};
    VkFramebuffer shadow_framebuffer_{VK_NULL_HANDLE};
    VkPipeline shadow_pipeline_{VK_NULL_HANDLE};
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
    double last_shadow_pass_ms_{0.0};
    std::uint32_t last_draw_calls_{0};
    std::uint32_t last_visible_objects_{0};
    std::uint32_t last_triangles_{0};
};

} // namespace arut::rendering::vulkan
