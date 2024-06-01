#pragma once

#include "images.hpp"

namespace mv {

struct vulkan_memory_t {
    VkDeviceMemory memory;
    VkDeviceSize size;
    VkDeviceSize bind_offset{0};

    const vulkan_device_t *device;

    vulkan_memory_t(
        VkDeviceMemory p_memory,
        VkDeviceSize p_size,
        const vulkan_device_t &p_device
    )
        : memory(p_memory), size(p_size), device(&p_device) {}

    static auto allocate(
        const vulkan_device_t &device,
        std::span<const VkMemoryRequirements> requirements,
        VkMemoryPropertyFlags property_flags
    ) -> vulkan_memory_t;

    inline auto
    bind_image(const vulkan_image_t &image, VkMemoryRequirements requirements)
        -> void {
        const auto offset_factor = bind_offset / requirements.alignment;
        bind_offset = (offset_factor + 1) * requirements.alignment;
        vkBindImageMemory(device->logical, image.image, memory, bind_offset);
        bind_offset += requirements.size;
    }

    NO_COPY(vulkan_memory_t);

    inline vulkan_memory_t(vulkan_memory_t &&other) {
        memory = other.memory;
        size = other.size;
        device = other.device;

        other.memory = VK_NULL_HANDLE;
        other.size = 0;
        other.device = nullptr;
    }

    inline auto operator=(vulkan_memory_t&& other) -> vulkan_memory_t& {
        std::swap(memory, other.memory);
        std::swap(size, other.size);
        std::swap(device, other.device);

        return *this;
    }

    inline ~vulkan_memory_t() {
        if (device != nullptr && memory != VK_NULL_HANDLE)
            vkFreeMemory(device->logical, memory, nullptr);
    }
};

auto get_memory_type_index(
    const vulkan_device_t &device,
    uint32_t type_bits,
    VkMemoryPropertyFlags property_flags
) -> uint32_t;

} // namespace mv
