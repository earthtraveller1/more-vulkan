#pragma once

#include <stb_image.h>
#include <vulkan/vulkan.h>

#include "commands.hpp"
#include "common.hpp"
#include "device.hpp"

namespace mv {
struct buffer_t;
struct vulkan_memory_t;

struct image_t {
    stbi_uc *data;
    int width;
    int height;
    int channels;

    image_t(stbi_uc *p_data, int p_width, int p_height, int p_channels)
        : data(p_data), width(p_width), height(p_height), channels(p_channels) {
    }

    static auto load_from_file(std::string_view file_path, int desired_channels)
        -> image_t;

    NO_COPY(image_t);

    ~image_t() {
        stbi_image_free(data);
    }
};

struct vulkan_image_t {
    VkImage image;
    VkFormat format;
    VkImageLayout layout;

    uint32_t width;
    uint32_t height;

    const vulkan_device_t *device;

    vulkan_image_t() = default;

    vulkan_image_t(
        VkImage p_image,
        VkFormat p_format,
        VkImageLayout p_layout,
        uint32_t p_width,
        uint32_t p_height,
        const vulkan_device_t &p_device
    )
        : image(p_image), format(p_format), layout(p_layout), width(p_width),
          height(p_height), device(&p_device) {}

    NO_COPY(vulkan_image_t);

    vulkan_image_t(vulkan_image_t &&other) noexcept;
    auto operator=(vulkan_image_t &&other) noexcept -> vulkan_image_t &;

    static auto
    create(const vulkan_device_t &device, uint32_t width, uint32_t height)
        -> vulkan_image_t;

    static auto create_depth_attachment(
        const vulkan_device_t &device, uint32_t width, uint32_t height
    ) -> vulkan_image_t;

    auto
    load_from_image(const command_pool_t &command_pool, const image_t &image)
        -> void;

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

    auto get_descriptor_image_info(VkSampler sampler, VkImageView view) const
        -> VkDescriptorImageInfo {
        return {
            .sampler = sampler,
            .imageView = view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    inline ~vulkan_image_t() noexcept {
        if (device != nullptr) {
            vkDestroyImage(device->logical, image, nullptr);
        }
    }
};

struct vulkan_image_view_t {
    VkImageView image_view;

    // This is a pointer, as this class has to be moveable, and I could not fig-
    // ure out how to move references.
    const vulkan_image_t *image;

    vulkan_image_view_t() = default;

    inline vulkan_image_view_t(
        VkImageView p_image_view, const vulkan_image_t &p_image
    )
        : image_view(p_image_view), image(&p_image) {}

    NO_COPY(vulkan_image_view_t);

    inline vulkan_image_view_t(vulkan_image_view_t&& other) {
        image_view = other.image_view;
        image = other.image;

        other.image_view = VK_NULL_HANDLE;
        other.image = nullptr;
    }

    inline auto operator=(vulkan_image_view_t&& other) -> vulkan_image_view_t& {
        std::swap(image_view, other.image_view);
        std::swap(image, other.image);
        return *this;
    }

    static auto
    create(const vulkan_image_t &p_image, VkImageAspectFlags image_aspect_flags)
        -> vulkan_image_view_t;

    inline ~vulkan_image_view_t() {
        if (image != nullptr &&  image->device != nullptr && image_view != VK_NULL_HANDLE) {
            vkDestroyImageView(this->image->device->logical, image_view, nullptr);
        }
    }
};

} // namespace mv
