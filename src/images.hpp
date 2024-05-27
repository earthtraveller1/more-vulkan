#pragma once

#include <vulkan/vulkan.h>

#include "commands.hpp"
#include "common.hpp"
#include "device.hpp"

namespace mv {
struct buffer_t;
struct vulkan_memory_t;

struct vulkan_image_t {
    VkImage image;
    VkImageView view;
    VkFormat format;
    VkImageLayout layout;

    uint32_t width;
    uint32_t height;

    const vulkan_device_t *device;

    vulkan_image_t() = default;

    vulkan_image_t(
        VkImage p_image,
        VkImageView p_view,
        VkFormat p_format,
        VkImageLayout p_layout,
        uint32_t p_width,
        uint32_t p_height,
        const vulkan_device_t &p_device
    )
        : image(p_image), view(p_view), format(p_format), layout(p_layout),
          width(p_width), height(p_height), device(&p_device) {}

    NO_COPY(vulkan_image_t);

    vulkan_image_t(vulkan_image_t &&other) noexcept;
    auto operator=(vulkan_image_t &&other) noexcept -> vulkan_image_t &;

    static auto
    create(const vulkan_device_t &device, uint32_t width, uint32_t height)
        -> vulkan_image_t;

    static auto create_depth_attachment(
        const vulkan_device_t &device, uint32_t width, uint32_t height
    ) -> vulkan_image_t;

    auto load_from_file(
        const command_pool_t &command_pool,
        std::string_view file_path
    ) -> void;

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

    auto copy_from_buffer(
        const buffer_t &source, const command_pool_t &command_pool
    ) const -> void;

    auto transition_layout(
        const command_pool_t &command_pool, VkImageLayout new_layout
    ) -> void;

    auto get_memory_requirements() const -> VkMemoryRequirements {
        VkMemoryRequirements memory_requirements;
        vkGetImageMemoryRequirements(
            device->logical, image, &memory_requirements
        );
        return memory_requirements;
    }

    static auto get_set_layout_binding(
        uint32_t binding,
        uint32_t descriptor_count,
        VkShaderStageFlags stage_flags
    ) -> VkDescriptorSetLayoutBinding {
        return {
            .binding = binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = descriptor_count,
            .stageFlags = stage_flags,
            .pImmutableSamplers = nullptr,
        };
    }

    auto get_descriptor_image_info(VkSampler sampler) const
        -> VkDescriptorImageInfo {
        return {
            .sampler = sampler,
            .imageView = view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    inline ~vulkan_image_t() noexcept {
        if (device != nullptr) {
            vkDestroyImageView(device->logical, view, nullptr);
            vkDestroyImage(device->logical, image, nullptr);
        }
    }
};
} // namespace mv
