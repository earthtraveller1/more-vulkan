#include "memory.hpp"

auto mv::vulkan_memory_t::allocate(
    const vulkan_device_t &device,
    std::span<const VkMemoryRequirements> requirements,
    VkMemoryPropertyFlags property_flags
) -> vulkan_memory_t {
    VkDeviceSize total_size = 0;
    for (const auto &req : requirements) {
        const auto offset = (total_size / req.alignment + 1) * req.alignment;
        total_size = req.size + offset;
    }

    const VkMemoryAllocateInfo allocate_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = total_size,
        .memoryTypeIndex = get_memory_type_index(
            device, requirements[0].memoryTypeBits, property_flags
        ),
    };

    VkDeviceMemory memory;
    const auto result =
        vkAllocateMemory(device.logical, &allocate_info, nullptr, &memory);

    if (result != VK_SUCCESS) {
        throw vulkan_exception{result};
    }

    return {memory, total_size, device};
}

auto mv::get_memory_type_index(
    const vulkan_device_t &device,
    uint32_t type_bits,
    VkMemoryPropertyFlags property_flags
) -> uint32_t {
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(device.physical, &memory_properties);

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        const auto has_type_bit = (type_bits & (1 << i)) != 0;

        const auto has_property_flags =
            (property_flags & property_flags) == property_flags;

        if (has_type_bit && has_property_flags) {
            return i;
        }
    }

    throw no_adequate_memory_type_exception{};
}
