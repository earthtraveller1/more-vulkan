#include "errors.hpp"

#include "sync.hpp"

auto mv::vulkan_fence_t::create(const vulkan_device_t &p_device)
    -> vulkan_fence_t {
    const VkFenceCreateInfo fence_info{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    VkFence fence;
    VK_ERROR(vkCreateFence(p_device.logical, &fence_info, nullptr, &fence));
    return vulkan_fence_t(fence, p_device);
}

auto mv::vulkan_semaphore_t::create(const vulkan_device_t &p_device)
    -> vulkan_semaphore_t {
    const VkSemaphoreCreateInfo semaphore_info{
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkSemaphore semaphore;
    VK_ERROR(vkCreateSemaphore(
        p_device.logical, &semaphore_info, nullptr, &semaphore
    ));
    return vulkan_semaphore_t(semaphore, p_device);
}
