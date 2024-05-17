#pragma once

#include <vulkan/vulkan.h>

#include "common.hpp"
#include "device.hpp"
#include "errors.hpp"

namespace mv {
struct command_pool_t {
    VkCommandPool pool;
    const vulkan_device_t &device;

    command_pool_t(VkCommandPool p_pool, const vulkan_device_t &p_device)
        : pool(p_pool), device(p_device) {}

    static auto create(const vulkan_device_t &p_device) -> command_pool_t;

    NO_COPY(command_pool_t);
    YES_MOVE(command_pool_t);

    auto allocate_buffer() const -> VkCommandBuffer; 

    ~command_pool_t() {
        vkDestroyCommandPool(device.logical, pool, nullptr);
    }
};
} // namespace mv
