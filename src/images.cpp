#include <algorithm>

#include <stb_image.h>
#include <vulkan/vulkan_core.h>

#include "buffers.hpp"
#include "common.hpp"
#include "errors.hpp"

#include "images.hpp"

namespace mv {

auto image_t::load_from_file(std::string_view file_path, int desired_channels)
    -> image_t {
    int width, height, channels;
    const auto data = stbi_load(
        file_path.data(), &width, &height, &channels, desired_channels
    );

    if (data == nullptr) {
        throw file_exception(file_exception::type_t::read, file_path);
    }

    return {data, width, height, channels};
}

vulkan_image_t::vulkan_image_t(vulkan_image_t &&other) noexcept {
    image = other.image;
    format = other.format;
    layout = other.layout;
    width = other.width;
    height = other.height;
    device = other.device;

    other.image = VK_NULL_HANDLE;
    other.format = VK_FORMAT_UNDEFINED;
    other.layout = VK_IMAGE_LAYOUT_UNDEFINED;
    other.width = 0;
    other.height = 0;
    other.device = nullptr;
}

auto vulkan_image_t::operator=(vulkan_image_t &&other
) noexcept -> vulkan_image_t & {
    std::swap(image, other.image);
    std::swap(format, other.format);
    std::swap(layout, other.layout);
    std::swap(width, other.width);
    std::swap(height, other.height);
    std::swap(device, other.device);

    return *this;
}

auto vulkan_image_t::create(
    const vulkan_device_t &device,
    uint32_t width,
    uint32_t height,
    VkFormat format
) -> vulkan_image_t {
    const VkImageCreateInfo image_create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
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

    return {
        image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        width,
        height,
        device,
    };
}

auto vulkan_image_t::create_depth_attachment(
    const vulkan_device_t &device, uint32_t width, uint32_t height
) -> vulkan_image_t {
    const std::array format_candiates{
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    const auto format = std::find_if(
        format_candiates.begin(),
        format_candiates.end(),
        [&](auto p_format) {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(
                device.physical, p_format, &properties
            );

            return (properties.optimalTilingFeatures &
                    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
    );

    const VkImageCreateInfo image_create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = *format,
        .extent =
            {
                .width = width,
                .height = height,
                .depth = 1,
            },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
    };

    VkImage image;
    VK_ERROR(vkCreateImage(device.logical, &image_create_info, nullptr, &image)
    );

    return {image, *format, VK_IMAGE_LAYOUT_UNDEFINED, width, height, device};
}

auto vulkan_image_t::load_from_image(
    const command_pool_t &command_pool, const image_t &image
) -> void {
    transition_layout(command_pool, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    auto staging_buffer =
        staging_buffer_t::create(*device, width * height * 4 * sizeof(uint8_t));

    memcpy(
        staging_buffer.map_memory(),
        image.data,
        width * height * 4 * sizeof(uint8_t)
    );
    staging_buffer.unmap_memory();

    copy_from_buffer(staging_buffer.buffer, command_pool);

    vkQueueWaitIdle(device->graphics_queue);

    transition_layout(command_pool, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

auto vulkan_image_t::create_sampler() const -> sampler_t {
    const VkSamplerCreateInfo sampler_create_info{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 1.0f,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .minLod = 0.0f,
        .maxLod = 0.0f,
        .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        .unnormalizedCoordinates = VK_FALSE
    };

    VkSampler sampler;
    VK_ERROR(vkCreateSampler(
        device->logical, &sampler_create_info, nullptr, &sampler
    ));

    return {sampler, *device};
}

auto vulkan_image_t::copy_from_buffer(
    const buffer_t &p_source, const command_pool_t &p_command_pool
) const -> void {
    const auto command_buffer = p_command_pool.allocate_buffer();

    const VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VK_ERROR(vkBeginCommandBuffer(command_buffer, &begin_info));

    const VkBufferImageCopy copy_region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },

        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}
    };

    vkCmdCopyBufferToImage(
        command_buffer,
        p_source.buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy_region
    );

    VK_ERROR(vkEndCommandBuffer(command_buffer));

    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    };

    VK_ERROR(
        vkQueueSubmit(device->graphics_queue, 1, &submit_info, VK_NULL_HANDLE)
    );
}

auto vulkan_image_t::transition_layout(
    const command_pool_t &command_pool, VkImageLayout p_new_layout
) -> void {
    const auto command_buffer = command_pool.allocate_buffer();

    const VkCommandBufferBeginInfo begin_info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VK_ERROR(vkBeginCommandBuffer(command_buffer, &begin_info));

    struct masks_t {
        VkAccessFlags src_access_mask;
        VkPipelineStageFlags src_stage_mask;
        VkAccessFlags dst_access_mask;
        VkPipelineStageFlags dst_stage_mask;
    } masks = [&]() -> masks_t {
        if (this->layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            p_new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            return {
                .src_access_mask = 0,
                .src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            };
        } else if (this->layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
                   p_new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            return {
                .src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dst_access_mask = VK_ACCESS_SHADER_READ_BIT,
                .dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            };
        } else if (this->layout == VK_IMAGE_LAYOUT_UNDEFINED &&
                   p_new_layout ==
                       VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
            return {
                .src_access_mask = 0,
                .src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .dst_access_mask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dst_stage_mask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            };
        } else if (this->layout == VK_IMAGE_LAYOUT_UNDEFINED &&
                   p_new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
            return {
                .src_access_mask = 0,
                .src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                .dst_access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .dst_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            };
        } else if (this->layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && p_new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            return {
                .src_access_mask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                                   VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dst_access_mask = VK_ACCESS_SHADER_READ_BIT,
                .dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            };
        } else {
            throw std::runtime_error(
                std::string("unsupported layout transition.") +
                std::to_string(this->layout) + " -> " +
                std::to_string(p_new_layout)
            );
        }
    }();

    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = masks.src_access_mask, // TODO
        .dstAccessMask = masks.dst_access_mask, // TODO
        .oldLayout = this->layout,
        .newLayout = p_new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            {
                .aspectMask = [&]() -> VkImageAspectFlags {
                    if (p_new_layout ==
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {

                        const auto has_stencil_component =
                            format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                            format == VK_FORMAT_D24_UNORM_S8_UINT;

                        if (has_stencil_component) {
                            return VK_IMAGE_ASPECT_DEPTH_BIT |
                                   VK_IMAGE_ASPECT_STENCIL_BIT;
                        } else {
                            return VK_IMAGE_ASPECT_DEPTH_BIT;
                        }

                    } else {
                        return VK_IMAGE_ASPECT_COLOR_BIT;
                    }
                }(),
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            },
    };

    vkCmdPipelineBarrier(
        command_buffer,
        masks.src_stage_mask,
        masks.dst_stage_mask,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier
    );
    VK_ERROR(vkEndCommandBuffer(command_buffer));

    const VkSubmitInfo submit_info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    };

    VK_ERROR(
        vkQueueSubmit(device->graphics_queue, 1, &submit_info, VK_NULL_HANDLE)
    );

    this->layout = p_new_layout;
}

auto vulkan_image_view_t::create(
    const vulkan_image_t &p_image, VkImageAspectFlags p_aspect_flags
) -> vulkan_image_view_t {
    const VkImageViewCreateInfo view_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = p_image.image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = p_image.format,
        .components =
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
        .subresourceRange =
            {
                .aspectMask = p_aspect_flags,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1,
            }
    };

    VkImageView view;
    VK_ERROR(
        vkCreateImageView(p_image.device->logical, &view_info, nullptr, &view)
    );

    return {view, p_image};
}
} // namespace mv
