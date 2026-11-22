#include "arut/rendering/pbr_brdf.hpp"

#include <vulkan/vulkan.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
struct GpuResult { float x, y, z, w; };

bool check(VkResult result, const char* operation) {
    if (result == VK_SUCCESS) return true;
    std::cerr << operation << " failed: VkResult=" << result << '\n';
    return false;
}

std::vector<std::uint32_t> read_spirv(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return {};
    const auto size = file.tellg();
    if (size <= 0 || (static_cast<std::uint64_t>(size) % 4U) != 0U) return {};
    std::vector<std::uint32_t> words(static_cast<std::size_t>(size) / 4U);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(words.data()), size);
    return file ? words : std::vector<std::uint32_t>{};
}

std::uint32_t find_memory_type(VkPhysicalDevice physical, std::uint32_t bits,
                               VkMemoryPropertyFlags required) {
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physical, &properties);
    for (std::uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
        if ((bits & (1U << i)) != 0U &&
            (properties.memoryTypes[i].propertyFlags & required) == required) return i;
    }
    return UINT32_MAX;
}
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: arut_vulkan_pbr_reference <shader.spv>\n";
        return 2;
    }
    const auto spirv = read_spirv(argv[1]);
    if (spirv.empty()) {
        std::cerr << "Could not read valid SPIR-V module\n";
        return 2;
    }

    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkShaderModule shader = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    int exit_code = 1;

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "ARUT-HARDT PBR GPU reference";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app;
    if (!check(vkCreateInstance(&instance_info, nullptr, &instance), "vkCreateInstance")) goto cleanup;

    {
        std::uint32_t count = 0;
        if (!check(vkEnumeratePhysicalDevices(instance, &count, nullptr), "enumerate physical devices") || count == 0) goto cleanup;
        std::vector<VkPhysicalDevice> physicals(count);
        if (!check(vkEnumeratePhysicalDevices(instance, &count, physicals.data()), "read physical devices")) goto cleanup;
        VkPhysicalDevice physical = VK_NULL_HANDLE;
        std::uint32_t queue_family = UINT32_MAX;
        for (auto candidate : physicals) {
            std::uint32_t family_count = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &family_count, nullptr);
            std::vector<VkQueueFamilyProperties> families(family_count);
            vkGetPhysicalDeviceQueueFamilyProperties(candidate, &family_count, families.data());
            for (std::uint32_t i = 0; i < family_count; ++i) {
                if ((families[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0U) {
                    physical = candidate; queue_family = i; break;
                }
            }
            if (physical != VK_NULL_HANDLE) break;
        }
        if (physical == VK_NULL_HANDLE) { std::cerr << "No Vulkan compute queue found\n"; goto cleanup; }

        VkPhysicalDeviceProperties device_properties{};
        vkGetPhysicalDeviceProperties(physical, &device_properties);
        std::cout << "GPU device: " << device_properties.deviceName << '\n';

        const float priority = 1.0F;
        VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        queue_info.queueFamilyIndex = queue_family;
        queue_info.queueCount = 1;
        queue_info.pQueuePriorities = &priority;
        VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        device_info.queueCreateInfoCount = 1;
        device_info.pQueueCreateInfos = &queue_info;
        if (!check(vkCreateDevice(physical, &device_info, nullptr, &device), "vkCreateDevice")) goto cleanup;
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, queue_family, 0, &queue);

        VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = sizeof(GpuResult);
        buffer_info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (!check(vkCreateBuffer(device, &buffer_info, nullptr, &buffer), "vkCreateBuffer")) goto cleanup;
        VkMemoryRequirements requirements{};
        vkGetBufferMemoryRequirements(device, buffer, &requirements);
        const auto memory_type = find_memory_type(physical, requirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (memory_type == UINT32_MAX) { std::cerr << "No host-visible coherent memory type\n"; goto cleanup; }
        VkMemoryAllocateInfo allocation{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        allocation.allocationSize = requirements.size;
        allocation.memoryTypeIndex = memory_type;
        if (!check(vkAllocateMemory(device, &allocation, nullptr, &memory), "vkAllocateMemory") ||
            !check(vkBindBufferMemory(device, buffer, memory, 0), "vkBindBufferMemory")) goto cleanup;

        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        set_info.bindingCount = 1;
        set_info.pBindings = &binding;
        if (!check(vkCreateDescriptorSetLayout(device, &set_info, nullptr, &set_layout), "vkCreateDescriptorSetLayout")) goto cleanup;
        VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1};
        VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        pool_info.maxSets = 1; pool_info.poolSizeCount = 1; pool_info.pPoolSizes = &pool_size;
        if (!check(vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptor_pool), "vkCreateDescriptorPool")) goto cleanup;
        VkDescriptorSetAllocateInfo set_alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        set_alloc.descriptorPool = descriptor_pool; set_alloc.descriptorSetCount = 1; set_alloc.pSetLayouts = &set_layout;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        if (!check(vkAllocateDescriptorSets(device, &set_alloc, &descriptor_set), "vkAllocateDescriptorSets")) goto cleanup;
        VkDescriptorBufferInfo buffer_descriptor{buffer, 0, sizeof(GpuResult)};
        VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write.dstSet = descriptor_set; write.dstBinding = 0; write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; write.pBufferInfo = &buffer_descriptor;
        vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);

        VkPushConstantRange unused_push{};
        VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
        layout_info.setLayoutCount = 1; layout_info.pSetLayouts = &set_layout;
        (void)unused_push;
        if (!check(vkCreatePipelineLayout(device, &layout_info, nullptr, &pipeline_layout), "vkCreatePipelineLayout")) goto cleanup;
        VkShaderModuleCreateInfo shader_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
        shader_info.codeSize = spirv.size() * sizeof(std::uint32_t);
        shader_info.pCode = spirv.data();
        if (!check(vkCreateShaderModule(device, &shader_info, nullptr, &shader), "vkCreateShaderModule")) goto cleanup;
        VkPipelineShaderStageCreateInfo stage{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT; stage.module = shader; stage.pName = "main";
        VkComputePipelineCreateInfo pipeline_info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
        pipeline_info.stage = stage; pipeline_info.layout = pipeline_layout;
        if (!check(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline), "vkCreateComputePipelines")) goto cleanup;

        VkCommandPoolCreateInfo command_pool_info{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
        command_pool_info.queueFamilyIndex = queue_family;
        if (!check(vkCreateCommandPool(device, &command_pool_info, nullptr, &command_pool), "vkCreateCommandPool")) goto cleanup;
        VkCommandBufferAllocateInfo command_alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        command_alloc.commandPool = command_pool; command_alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; command_alloc.commandBufferCount = 1;
        VkCommandBuffer command = VK_NULL_HANDLE;
        if (!check(vkAllocateCommandBuffers(device, &command_alloc, &command), "vkAllocateCommandBuffers")) goto cleanup;
        VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (!check(vkBeginCommandBuffer(command, &begin), "vkBeginCommandBuffer")) goto cleanup;
        vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
        vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
        vkCmdDispatch(command, 1, 1, 1);
        if (!check(vkEndCommandBuffer(command), "vkEndCommandBuffer")) goto cleanup;
        VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        if (!check(vkCreateFence(device, &fence_info, nullptr, &fence), "vkCreateFence")) goto cleanup;
        VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
        submit.commandBufferCount = 1; submit.pCommandBuffers = &command;
        if (!check(vkQueueSubmit(queue, 1, &submit, fence), "vkQueueSubmit") ||
            !check(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "vkWaitForFences")) goto cleanup;

        void* mapped = nullptr;
        if (!check(vkMapMemory(device, memory, 0, sizeof(GpuResult), 0, &mapped), "vkMapMemory")) goto cleanup;
        const auto gpu = *static_cast<const GpuResult*>(mapped);
        vkUnmapMemory(device, memory);
        const auto cpu = arut::rendering::pbr::evaluate_direct_brdf(
            {0.0F, 0.0F, 1.0F}, {0.0F, 0.0F, 1.0F}, {0.6F, 0.0F, 0.8F},
            {0.8F, 0.2F, 0.1F}, 0.2F, 0.35F, {3.0F, 2.0F, 1.0F});
        const std::array<float, 3> gpu_values{gpu.x, gpu.y, gpu.z};
        const std::array<float, 3> cpu_values{cpu.x, cpu.y, cpu.z};
        for (std::size_t i = 0; i < gpu_values.size(); ++i) {
            const float tolerance = 2.0e-4F * std::max(1.0F, std::abs(cpu_values[i]));
            if (!std::isfinite(gpu_values[i]) || std::abs(gpu_values[i] - cpu_values[i]) > tolerance) {
                std::cerr << "GPU/CPU BRDF mismatch channel " << i << ": GPU="
                          << gpu_values[i] << " CPU=" << cpu_values[i]
                          << " tolerance=" << tolerance << '\n';
                goto cleanup;
            }
        }
        if (std::abs(gpu.w - 1.0F) > 1.0e-6F) { std::cerr << "GPU result sentinel mismatch\n"; goto cleanup; }
        std::cout << "GPU BRDF: " << gpu.x << ", " << gpu.y << ", " << gpu.z << '\n';
        std::cout << "CPU BRDF: " << cpu.x << ", " << cpu.y << ", " << cpu.z << '\n';
        std::cout << "PASS: GPU shader readback matches C++ reference within tolerance\n";
        exit_code = 0;
    }

cleanup:
    if (device != VK_NULL_HANDLE) vkDeviceWaitIdle(device);
    if (fence) vkDestroyFence(device, fence, nullptr);
    if (command_pool) vkDestroyCommandPool(device, command_pool, nullptr);
    if (pipeline) vkDestroyPipeline(device, pipeline, nullptr);
    if (shader) vkDestroyShaderModule(device, shader, nullptr);
    if (pipeline_layout) vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
    if (descriptor_pool) vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
    if (set_layout) vkDestroyDescriptorSetLayout(device, set_layout, nullptr);
    if (buffer) vkDestroyBuffer(device, buffer, nullptr);
    if (memory) vkFreeMemory(device, memory, nullptr);
    if (device) vkDestroyDevice(device, nullptr);
    if (instance) vkDestroyInstance(instance, nullptr);
    return exit_code;
}
