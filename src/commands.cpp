
#include "commands.hpp"

namespace mv {
auto command_pool_t::create(const vulkan_device_t &p_device) -> command_pool_t {
    const VkCommandPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = p_device.graphics_family,
    };

    VkCommandPool pool;
    VK_ERROR(vkCreateCommandPool(p_device.logical, &pool_info, nullptr, &pool));
    return command_pool_t(pool, p_device);
}

auto command_pool_t::allocate_buffer() const -> VkCommandBuffer {
    VkCommandBufferAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer buffer;
    VK_ERROR(vkAllocateCommandBuffers(device.logical, &alloc_info, &buffer));
    return buffer;
}
} // namespace mv
