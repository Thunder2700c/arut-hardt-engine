#include "arut/rendering/vulkan/android_renderer.hpp"

#include <android/log.h>
#include <vulkan/vulkan_android.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <cstdio>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <android/asset_manager.h>
#include "arut/rendering/mesh.hpp"
#include "arut/rendering/image.hpp"

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

bool AndroidRenderer::initialize(ANativeWindow* window, AAssetManager* assets) noexcept {
    shutdown();
    window_ = window;
    assets_ = assets;
    if (!window || !assets_ || !create_instance() || !create_surface(window) ||
        !select_device() || !create_device() || !create_swapchain() ||
        !create_render_targets() || !create_triangle_geometry() || !create_triangle_pipeline() ||
        !create_pbr_geometry() || !create_fallback_texture() || !create_pbr_pipeline() ||
        !create_pbr_descriptors() || !create_commands_and_sync()) {
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

    const std::array<VkDescriptorSetLayoutBinding, 1> frame_bindings{{
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
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
    VkPushConstantRange push{};
    push.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    push.offset = 0;
    push.size = 64;
    VkPipelineLayoutCreateInfo pipeline_layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    pipeline_layout.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
    pipeline_layout.pSetLayouts = layouts.data();
    pipeline_layout.pushConstantRangeCount = 1;
    pipeline_layout.pPushConstantRanges = &push;
    result = vkCreatePipelineLayout(device_, &pipeline_layout, nullptr, &pbr_pipeline_layout_);
    if (result != VK_SUCCESS) { vkDestroyShaderModule(device_, fragment_module, nullptr); vkDestroyShaderModule(device_, vertex_module, nullptr); log_error("Create PBR pipeline layout failed", result); return false; }

    const std::array<VkPipelineShaderStageCreateInfo, 2> stages{{
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_VERTEX_BIT, vertex_module, "main", nullptr},
        {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, nullptr, 0, VK_SHADER_STAGE_FRAGMENT_BIT, fragment_module, "main", nullptr}
    }};
    const VkVertexInputBindingDescription binding{0, sizeof(arut::rendering::MeshVertex), VK_VERTEX_INPUT_RATE_VERTEX};
    const std::array<VkVertexInputAttributeDescription, 4> attributes{{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, position))},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, normal))},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, tangent))},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<std::uint32_t>(offsetof(arut::rendering::MeshVertex, uv))}
    }};
    VkPipelineVertexInputStateCreateInfo vertex_input{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertex_input.vertexBindingDescriptionCount = 1; vertex_input.pVertexBindingDescriptions = &binding;
    vertex_input.vertexAttributeDescriptionCount = static_cast<std::uint32_t>(attributes.size()); vertex_input.pVertexAttributeDescriptions = attributes.data();
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
        float light_direction[4];
        float light_radiance[4];
    };
    struct alignas(16) MaterialData final {
        float base_color[4];
        float emissive_metallic[4];
        float roughness_normal_occlusion_alpha[4];
        std::uint32_t flags[4];
    };
    static_assert(sizeof(FrameData) == 112);
    static_assert(sizeof(MaterialData) == 64);
    if (!load_texture_asset("textures/demo_base_color.png", VK_FORMAT_R8G8B8A8_SRGB, true,
                            base_color_image_, base_color_memory_, base_color_view_,
                            base_color_sampler_, base_color_mip_levels_)) return false;
    if (!create_buffer(sizeof(FrameData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, frame_uniform_buffer_, frame_uniform_memory_) ||
        !create_buffer(sizeof(MaterialData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, material_uniform_buffer_, material_uniform_memory_)) return false;

    const FrameData frame{
        {0}, {2.2F, 1.8F, 2.8F, 1.0F}, {0.45F, -1.0F, 0.35F, 0.0F}, {3.0F, 2.8F, 2.5F, 0.0F}
    };
    const auto view = arut::rendering::look_at_rh({frame.camera_position[0], frame.camera_position[1], frame.camera_position[2]},
                                                  {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    const auto projection = arut::rendering::perspective_rh_zo(0.785398163F,
        static_cast<float>(extent_.width) / static_cast<float>(std::max(1U, extent_.height)), 0.1F, 100.0F);
    const auto vp = arut::rendering::multiply(projection, view);
    FrameData frame_upload = frame;
    std::copy(vp.value.begin(), vp.value.end(), frame_upload.view_projection);
    void* mapped = nullptr;
    VkResult result = vkMapMemory(device_, frame_uniform_memory_, 0, sizeof(FrameData), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) { log_error("Map frame uniform buffer failed", result); return false; }
    std::memcpy(mapped, &frame_upload, sizeof(FrameData)); vkUnmapMemory(device_, frame_uniform_memory_);

    const MaterialData material{{1.0F, 1.0F, 1.0F, 1.0F}, {0.0F, 0.0F, 0.0F, 0.05F},
                                {0.48F, 1.0F, 1.0F, 0.5F}, {1U, 0U, 0U, 0U}};
    mapped = nullptr;
    result = vkMapMemory(device_, material_uniform_memory_, 0, sizeof(MaterialData), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) { log_error("Map material uniform buffer failed", result); return false; }
    std::memcpy(mapped, &material, sizeof(MaterialData)); vkUnmapMemory(device_, material_uniform_memory_);

    const std::array<VkDescriptorPoolSize, 2> sizes{{
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3}
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
    const VkDescriptorImageInfo base_color_info{base_color_sampler_, base_color_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    const VkDescriptorImageInfo fallback_info{fallback_sampler_, fallback_view_, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    std::array<VkWriteDescriptorSet, 5> writes{};
    writes[0] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, frame_descriptor_set_, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &frame_info, nullptr};
    writes[1] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, material_descriptor_set_, 0, 0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, nullptr, &material_info, nullptr};
    writes[2] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, material_descriptor_set_, 1, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &base_color_info, nullptr, nullptr};
    writes[3] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, material_descriptor_set_, 2, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &fallback_info, nullptr, nullptr};
    writes[4] = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr, material_descriptor_set_, 3, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, &fallback_info, nullptr, nullptr};
    vkUpdateDescriptorSets(device_, static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
    arut_android_log(ANDROID_LOG_INFO, "PBR frame/material descriptors and fallback texture bound.");
    return true;
}

bool AndroidRenderer::update_pbr_frame_data() noexcept {
    struct alignas(16) FrameData final {
        float view_projection[16];
        float camera_position[4];
        float light_direction[4];
        float light_radiance[4];
    };
    const std::array<float, 3> camera{2.2F, 1.8F, 2.8F};
    const auto view = arut::rendering::look_at_rh(camera, {0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F});
    const auto projection = arut::rendering::perspective_rh_zo(0.785398163F,
        static_cast<float>(extent_.width) / static_cast<float>(std::max(1U, extent_.height)), 0.1F, 100.0F);
    const auto vp = arut::rendering::multiply(projection, view);
    FrameData frame{};
    std::copy(vp.value.begin(), vp.value.end(), frame.view_projection);
    frame.camera_position[0] = camera[0]; frame.camera_position[1] = camera[1]; frame.camera_position[2] = camera[2]; frame.camera_position[3] = 1.0F;
    frame.light_direction[0] = 0.45F; frame.light_direction[1] = -1.0F; frame.light_direction[2] = 0.35F;
    frame.light_radiance[0] = 3.0F; frame.light_radiance[1] = 2.8F; frame.light_radiance[2] = 2.5F;
    void* mapped = nullptr;
    const VkResult result = vkMapMemory(device_, frame_uniform_memory_, 0, sizeof(FrameData), 0, &mapped);
    if (result != VK_SUCCESS || !mapped) { log_error("Update PBR frame uniform failed", result); return false; }
    std::memcpy(mapped, &frame, sizeof(FrameData));
    vkUnmapMemory(device_, frame_uniform_memory_);
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
    const auto model = arut::rendering::Mat4::identity();
    vkCmdPushConstants(command_buffers_[image_index], pbr_pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT,
                       0, sizeof(model), model.value.data());
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
    vkCmdBindVertexBuffers(command_buffers_[image_index], 0, 1, &cube_vertex_buffer_, &vertex_offset);
    vkCmdBindIndexBuffer(command_buffers_[image_index], cube_index_buffer_, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(command_buffers_[image_index], cube_index_count_, 1, 0, 0, 0);
    vkCmdEndRenderPass(command_buffers_[image_index]);
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
        if (pbr_pipeline_) vkDestroyPipeline(device_, pbr_pipeline_, nullptr);
        if (pbr_pipeline_layout_) vkDestroyPipelineLayout(device_, pbr_pipeline_layout_, nullptr);
        if (pbr_descriptor_pool_) vkDestroyDescriptorPool(device_, pbr_descriptor_pool_, nullptr);
        if (frame_set_layout_) vkDestroyDescriptorSetLayout(device_, frame_set_layout_, nullptr);
        if (material_set_layout_) vkDestroyDescriptorSetLayout(device_, material_set_layout_, nullptr);
        if (frame_uniform_buffer_) vkDestroyBuffer(device_, frame_uniform_buffer_, nullptr);
        if (frame_uniform_memory_) vkFreeMemory(device_, frame_uniform_memory_, nullptr);
        if (material_uniform_buffer_) vkDestroyBuffer(device_, material_uniform_buffer_, nullptr);
        if (material_uniform_memory_) vkFreeMemory(device_, material_uniform_memory_, nullptr);
        if (cube_vertex_buffer_) vkDestroyBuffer(device_, cube_vertex_buffer_, nullptr);
        if (cube_vertex_memory_) vkFreeMemory(device_, cube_vertex_memory_, nullptr);
        if (cube_index_buffer_) vkDestroyBuffer(device_, cube_index_buffer_, nullptr);
        if (cube_index_memory_) vkFreeMemory(device_, cube_index_memory_, nullptr);
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
        if (swapchain_) vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }
    in_flight_ = VK_NULL_HANDLE;
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
    cube_vertex_buffer_ = cube_index_buffer_ = VK_NULL_HANDLE;
    cube_vertex_memory_ = cube_index_memory_ = VK_NULL_HANDLE;
    cube_index_count_ = 0;
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
    swapchain_ = VK_NULL_HANDLE;
    framebuffers_.clear();
    image_views_.clear();
    command_buffers_.clear();
    images_.clear();
}

bool AndroidRenderer::recreate_swapchain() noexcept {
    if (!device_) return false;
    destroy_swapchain();
    return create_swapchain() && create_render_targets() && create_triangle_geometry() &&
           create_triangle_pipeline() && create_pbr_geometry() && create_fallback_texture() &&
           create_pbr_pipeline() && create_pbr_descriptors() && create_commands_and_sync();
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
