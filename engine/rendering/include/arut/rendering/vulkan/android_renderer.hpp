#pragma once

#include <android/native_window.h>
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

namespace arut::rendering::vulkan {

class AndroidRenderer final {
public:
    AndroidRenderer() = default;
    ~AndroidRenderer();
    AndroidRenderer(const AndroidRenderer&) = delete;
    AndroidRenderer& operator=(const AndroidRenderer&) = delete;

    bool initialize(ANativeWindow* window) noexcept;
    void shutdown() noexcept;
    bool draw_frame() noexcept;
    bool ready() const noexcept { return device_ != VK_NULL_HANDLE && swapchain_ != VK_NULL_HANDLE; }

private:
    bool create_instance() noexcept;
    bool create_surface(ANativeWindow* window) noexcept;
    bool select_device() noexcept;
    bool create_device() noexcept;
    bool create_swapchain() noexcept;
    bool create_render_targets() noexcept;
    bool create_commands_and_sync() noexcept;
    void destroy_swapchain() noexcept;
    bool recreate_swapchain() noexcept;

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
    std::vector<VkFramebuffer> framebuffers_;
    VkCommandPool command_pool_{VK_NULL_HANDLE};
    std::vector<VkCommandBuffer> command_buffers_;
    VkSemaphore image_available_{VK_NULL_HANDLE};
    VkSemaphore render_finished_{VK_NULL_HANDLE};
    VkFence in_flight_{VK_NULL_HANDLE};
};

} // namespace arut::rendering::vulkan
