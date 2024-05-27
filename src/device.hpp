#pragma once

#include <vulkan/vulkan.h>

#include "common.hpp"

namespace mv {
struct vulkan_instance_t {
    VkInstance instance;

    // May throw vulkan_exception
    static auto create(bool p_enable_validation) -> vulkan_instance_t;

    inline operator VkInstance() const noexcept {
        return instance;
    }

    inline ~vulkan_instance_t() {
        vkDestroyInstance(instance, nullptr);
    }
};

struct vulkan_device_t {
    VkPhysicalDevice physical;
    VkDevice logical;

    uint32_t graphics_family;
    uint32_t present_family;

    VkQueue graphics_queue;
    VkQueue present_queue;

    // May throw vulkan_exception
    static auto create(VkInstance p_instance, VkSurfaceKHR p_surface)
        -> vulkan_device_t;

    inline ~vulkan_device_t() {
        vkDestroyDevice(logical, nullptr);
    }
};
} // namespace mv

#include "images.hpp"

namespace mv {

struct vulkan_memory_t {
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkDeviceSize bind_offset{0};

    const vulkan_device_t &device;

    vulkan_memory_t(
        VkDeviceMemory p_memory,
        VkDeviceSize p_size,
        const vulkan_device_t &p_device
    )
        : memory(p_memory), size(p_size), device(p_device) {}

    static auto allocate(
        const vulkan_device_t &device,
        std::span<const VkMemoryRequirements> requirements,
        VkMemoryPropertyFlags property_flags
    ) -> vulkan_memory_t;

    inline auto bind_image(const vulkan_texture_t &image, VkDeviceSize size)
        -> void {
        vkBindImageMemory(device.logical, image.image, memory, bind_offset + size);
        bind_offset += size;
    }

    NO_COPY(vulkan_memory_t);

    inline ~vulkan_memory_t() {
        vkFreeMemory(device.logical, memory, nullptr);
    }
};

auto get_memory_type_index(
    const vulkan_device_t &device,
    uint32_t type_bits,
    VkMemoryPropertyFlags property_flags
) -> uint32_t;

} // namespace mv
