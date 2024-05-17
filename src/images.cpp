#include "common.hpp"
#include "errors.hpp"

#include "images.hpp"

namespace mv {
auto vulkan_texture_t::create(
    const vulkan_device_t &device, uint32_t width, uint32_t height
) -> vulkan_texture_t {
    const VkImageCreateInfo image_create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .extent = {.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkImage image;
    VK_ERROR(vkCreateImage(device.logical, &image_create_info, nullptr, &image)
    );

    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(device.logical, image, &memory_requirements);

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(device.physical, &memory_properties);
    const auto memory_property_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    std::optional<uint32_t> memory_type_index;

    for (uint32_t i = 0; i < memory_properties.memoryTypeCount; i++) {
        const auto property_flags =
            memory_properties.memoryTypes[i].propertyFlags;

        const auto has_type_bit =
            (memory_requirements.memoryTypeBits & (1 << i)) != 0;

        const auto has_property_flags =
            (property_flags & memory_property_flags) == memory_property_flags;

        if (has_type_bit && has_property_flags) {
            memory_type_index = i;
        }
    }

    if (!memory_type_index.has_value()) {
        throw std::runtime_error("Could not find suitable memory type.");
    }

    VkMemoryAllocateInfo memory_allocate_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = memory_type_index.value(),
    };

    VkDeviceMemory memory;
    VK_ERROR(vkAllocateMemory(
        device.logical, &memory_allocate_info, nullptr, &memory
    ));

    return {
        image,
        VK_NULL_HANDLE,
        memory,
        device,
    };
}
} // namespace mv
