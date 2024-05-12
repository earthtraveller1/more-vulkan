#include "errors.hpp"

#include "buffers.hpp"

namespace mv {
auto buffer_t::create(
    const mv::vulkan_device_t &p_device, VkDeviceSize p_size, type_t p_type
) -> buffer_t {
    const VkBufferCreateInfo buffer_create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = p_size,
        .usage =
            [&]() {
                switch (p_type) {
                case type_t::vertex:
                    return static_cast<VkBufferUsageFlags>(
                        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT
                    );
                case type_t::index:
                    return static_cast<VkBufferUsageFlags>(
                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT
                    );
                case type_t::staging:
                    return static_cast<VkBufferUsageFlags>(
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                    );
                case type_t::uniform:
                    return static_cast<VkBufferUsageFlags>(
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                    );
                }
            }(),
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VkBuffer buffer;
    auto result =
        vkCreateBuffer(p_device.logical, &buffer_create_info, nullptr, &buffer);

    if (result != VK_SUCCESS) {
        throw mv::vulkan_exception{result};
    }

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(
        p_device.logical, buffer, &memory_requirements
    );

    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(p_device.physical, &memory_properties);

    VkMemoryPropertyFlags memory_property_flags = [&]() {
        switch (p_type) {
        case type_t::vertex:
            return static_cast<VkMemoryPropertyFlags>(
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
        case type_t::index:
            return static_cast<VkMemoryPropertyFlags>(
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
            );
        case type_t::staging:
            return static_cast<VkMemoryPropertyFlags>(
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        case type_t::uniform:
            return static_cast<VkMemoryPropertyFlags>(
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            );
        }
    }();

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

    const VkMemoryAllocateInfo memory_allocate_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = memory_type_index.value(),
    };

    VkDeviceMemory memory;
    result = vkAllocateMemory(
        p_device.logical, &memory_allocate_info, nullptr, &memory
    );
    if (result != VK_SUCCESS) {
        throw vulkan_exception{result};
    }

    vkBindBufferMemory(p_device.logical, buffer, memory, 0);

    return {buffer, memory, p_size, p_device};
}

auto buffer_t::copy_from(const buffer_t &p_other, VkCommandPool p_command_pool)
    const -> void {
    // Pick the smallest of the two sizes
    const auto size = std::min(p_other.size, this->size);

    const VkCommandBufferAllocateInfo command_buffer_allocate_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = p_command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer p_command_buffer;
    VK_ERROR(vkAllocateCommandBuffers(
        device.logical, &command_buffer_allocate_info, &p_command_buffer
    ));

    const VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    VK_ERROR(vkBeginCommandBuffer(p_command_buffer, &begin_info));

    const VkBufferCopy copy_region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    vkCmdCopyBuffer(
        p_command_buffer, p_other.buffer, this->buffer, 1, &copy_region
    );

    VK_ERROR(vkEndCommandBuffer(p_command_buffer));

    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &p_command_buffer,
    };

    VK_ERROR(
        vkQueueSubmit(device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE)
    );
}
}
