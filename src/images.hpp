#pragma once

#include <vulkan/vulkan.h>

#include "commands.hpp"
#include "common.hpp"
#include "device.hpp"

namespace mv {
struct buffer_t;

struct vulkan_texture_t {
    VkImage image;
    VkImageView view;
    VkDeviceMemory memory;
    VkFormat format;

    uint32_t width;
    uint32_t height;

    const vulkan_device_t &device;

    vulkan_texture_t(
        VkImage p_image,
        VkImageView p_view,
        VkDeviceMemory p_memory,
        VkFormat p_format,
        uint32_t p_width,
        uint32_t p_height,
        const vulkan_device_t &p_device
    )
        : image(p_image), view(p_view), memory(p_memory), format(p_format),
          width(p_width), height(p_height), device(p_device) {}

    NO_COPY(vulkan_texture_t);
    YES_MOVE(vulkan_texture_t);

    static auto
    create(const vulkan_device_t &device, uint32_t width, uint32_t height)
        -> vulkan_texture_t;

    static auto
    load_from_file(const vulkan_device_t &device, const command_pool_t& command_pool, std::string_view file_path)
        -> vulkan_texture_t;

    struct sampler_t {
        VkSampler sampler;
        const vulkan_device_t &device;

        sampler_t(VkSampler p_sampler, const vulkan_device_t &p_device)
            : sampler(p_sampler), device(p_device) {}

        NO_COPY(sampler_t);

        inline ~sampler_t() noexcept {
            vkDestroySampler(device.logical, sampler, nullptr);
        }
    };

    auto create_sampler() const -> sampler_t;

    auto copy_from_buffer(const buffer_t &source, const command_pool_t &command_pool) const -> void;

    auto transition_layout(
        const command_pool_t &command_pool,
        VkImageLayout old_layout,
        VkImageLayout new_layout
    ) const -> void;

    inline ~vulkan_texture_t() noexcept {
        vkDestroyImageView(device.logical, view, nullptr);
        vkDestroyImage(device.logical, image, nullptr);
        vkFreeMemory(device.logical, memory, nullptr);
    }
};
} // namespace mv
