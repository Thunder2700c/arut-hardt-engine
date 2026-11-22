#include "arut/rendering/vulkan/android_renderer.hpp"

#include <android/log.h>
#include <vulkan/vulkan_android.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <cstdio>
#include <vector>

extern "C" void arut_android_log(int priority, const char* message) noexcept;

namespace arut::rendering::vulkan {
namespace {
constexpr char kTag[] = "ARUT-HARDT";
void log_error(const char* message, VkResult result = VK_SUCCESS) {
    char buffer[512]{}; std::snprintf(buffer, sizeof(buffer), "%s (VkResult=%d)", message, static_cast<int>(result));
    arut_android_log(ANDROID_LOG_ERROR, buffer);
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

bool AndroidRenderer::initialize(ANativeWindow* window) noexcept {
    shutdown();
    window_ = window;
    if (!window || !create_instance() || !create_surface(window) ||
        !select_device() || !create_device() || !create_swapchain() ||
        !create_render_targets() || !create_commands_and_sync()) {
        shutdown();
        return false;
    }
    char ready_message[256]{}; std::snprintf(ready_message, sizeof(ready_message), "Vulkan swapchain ready: %ux%u, images=%zu", extent_.width, extent_.height, images_.size());
    arut_android_log(ANDROID_LOG_INFO, ready_message);
    return true;
}

bool AndroidRenderer::create_instance() noexcept {
    const std::array<const char*, 2> extensions{
        VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME
    };
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
        std::uint32_t present = UINT32_MAX;
        for (std::uint32_t i = 0; i < family_count; ++i) {
            if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && graphics == UINT32_MAX) graphics = i;
            VkBool32 supported = VK_FALSE;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(candidate, i, surface_, &supported) == VK_SUCCESS &&
                supported && present == UINT32_MAX) present = i;
        }
        if (graphics != UINT32_MAX && present != UINT32_MAX) {
            physical_device_ = candidate;
            graphics_family_ = graphics;
            present_family_ = present;
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(candidate, &properties);
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
    const char* extension = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    info.queueCreateInfoCount = queue_count;
    info.pQueueCreateInfos = queues.data();
    info.enabledExtensionCount = 1;
    info.ppEnabledExtensionNames = &extension;
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
    return vkCreateFence(device_, &fence, nullptr, &in_flight_) == VK_SUCCESS;
}

bool AndroidRenderer::draw_frame() noexcept {
    if (!ready()) return false;
    VkResult result = vkWaitForFences(device_, 1, &in_flight_, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) return false;
    std::uint32_t image_index = 0;
    result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, image_available_, VK_NULL_HANDLE, &image_index);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) return recreate_swapchain();
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        log_error("vkAcquireNextImageKHR failed", result);
        return false;
    }
    vkResetFences(device_, 1, &in_flight_);
    vkResetCommandBuffer(command_buffers_[image_index], 0);
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    if (vkBeginCommandBuffer(command_buffers_[image_index], &begin) != VK_SUCCESS) return false;
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
    vkCmdEndRenderPass(command_buffers_[image_index]);
    if (vkEndCommandBuffer(command_buffers_[image_index]) != VK_SUCCESS) return false;
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
    if (result != VK_SUCCESS) { log_error("vkQueueSubmit failed", result); return false; }
    VkPresentInfoKHR present{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &render_finished_[image_index];
    present.swapchainCount = 1;
    present.pSwapchains = &swapchain_;
    present.pImageIndices = &image_index;
    result = vkQueuePresentKHR(present_queue_, &present);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) return recreate_swapchain();
    if (result != VK_SUCCESS) { log_error("vkQueuePresentKHR failed", result); return false; }
    return true;
}

void AndroidRenderer::destroy_swapchain() noexcept {
    if (device_ != VK_NULL_HANDLE) vkDeviceWaitIdle(device_);
    if (device_ != VK_NULL_HANDLE) {
        if (in_flight_) vkDestroyFence(device_, in_flight_, nullptr);
        if (image_available_) vkDestroySemaphore(device_, image_available_, nullptr);
        for (auto semaphore : render_finished_) if (semaphore) vkDestroySemaphore(device_, semaphore, nullptr);
        if (command_pool_) vkDestroyCommandPool(device_, command_pool_, nullptr);
        for (auto fb : framebuffers_) if (fb) vkDestroyFramebuffer(device_, fb, nullptr);
        if (render_pass_) vkDestroyRenderPass(device_, render_pass_, nullptr);
        for (auto view : image_views_) if (view) vkDestroyImageView(device_, view, nullptr);
        if (depth_view_) vkDestroyImageView(device_, depth_view_, nullptr);
        if (depth_image_) vkDestroyImage(device_, depth_image_, nullptr);
        if (depth_memory_) vkFreeMemory(device_, depth_memory_, nullptr);
        if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }
    in_flight_ = VK_NULL_HANDLE;
    image_available_ = VK_NULL_HANDLE;
    render_finished_.clear();
    command_pool_ = VK_NULL_HANDLE;
    render_pass_ = VK_NULL_HANDLE;
    depth_view_ = VK_NULL_HANDLE;
    depth_image_ = VK_NULL_HANDLE;
    depth_memory_ = VK_NULL_HANDLE;
    depth_format_ = VK_FORMAT_UNDEFINED;
    swapchain_ = VK_NULL_HANDLE;
    framebuffers_.clear();
    image_views_.clear();
    command_buffers_.clear();
    images_.clear();
}

bool AndroidRenderer::recreate_swapchain() noexcept {
    if (!device_) return false;
    destroy_swapchain();
    return create_swapchain() && create_render_targets() && create_commands_and_sync();
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
    physical_device_ = VK_NULL_HANDLE;
    graphics_queue_ = VK_NULL_HANDLE;
    present_queue_ = VK_NULL_HANDLE;
    graphics_family_ = present_family_ = UINT32_MAX;
}

} // namespace arut::rendering::vulkan
