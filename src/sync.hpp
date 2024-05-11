#pragma once

#include <vulkan/vulkan.h>

#include "common.hpp"
#include "device.hpp"

namespace mv {

struct vulkan_device_t;

struct vulkan_fence_t {
    VkFence fence;
    const vulkan_device_t &device;

    vulkan_fence_t(VkFence p_fence, const vulkan_device_t &p_device)
        : fence(p_fence), device(p_device) {}

    NO_COPY(vulkan_fence_t);
    YES_MOVE(vulkan_fence_t);

    static auto create(const vulkan_device_t &p_device) -> vulkan_fence_t;

    ~vulkan_fence_t() {
        vkDestroyFence(device.logical, fence, nullptr);
    }
};

struct vulkan_semaphore_t {
    VkSemaphore semaphore;
    const vulkan_device_t &device;

    vulkan_semaphore_t(VkSemaphore p_semaphore, const vulkan_device_t &p_device)
        : semaphore(p_semaphore), device(p_device) {}

    NO_COPY(vulkan_semaphore_t);
    YES_MOVE(vulkan_semaphore_t);

    static auto create(const vulkan_device_t &p_device) -> vulkan_semaphore_t;

    ~vulkan_semaphore_t() {
        vkDestroySemaphore(device.logical, semaphore, nullptr);
    }
};

} // namespace mv
