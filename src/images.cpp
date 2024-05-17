#include "common.hpp"
#include "errors.hpp"

#include "images.hpp"

namespace mv {
auto vulkan_texture_t::create(
    const vulkan_device_t &device, uint32_t width, uint32_t height
) -> vulkan_texture_t {
    const auto format = VK_FORMAT_R8G8B8A8_SRGB;

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

    vkBindImageMemory(device.logical, image, memory, 0);

    const VkImageViewCreateInfo image_view_create_info{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .components =
            {.r = VK_COMPONENT_SWIZZLE_R,
             .g = VK_COMPONENT_SWIZZLE_G,
             .b = VK_COMPONENT_SWIZZLE_B,
             .a = VK_COMPONENT_SWIZZLE_A},
        .subresourceRange =
            {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
             .baseMipLevel = 0,
             .levelCount = 1,
             .baseArrayLayer = 0,
             .layerCount = 1},
    };

    VkImageView image_view;
    VK_ERROR(vkCreateImageView(
        device.logical, &image_view_create_info, nullptr, &image_view
    ));

    return {
        image,
        image_view,
        memory,
        format,
        device,
    };
}

auto vulkan_texture_t::create_sampler() const -> sampler_t {
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
    VK_ERROR(
        vkCreateSampler(device.logical, &sampler_create_info, nullptr, &sampler)
    );

    return {sampler, device};
}

auto vulkan_texture_t::tansition_layout(
    const command_pool_t &command_pool,
    VkImageLayout p_old_layout,
    VkImageLayout p_new_layout
) const -> void {
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
        if (p_old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
            p_new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            return {
                .src_access_mask = 0,
                .src_stage_mask = 0,
                .dst_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT,
            };
        } else if (p_old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && p_new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            return {
                .src_access_mask = VK_ACCESS_TRANSFER_WRITE_BIT,
                .src_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT,
                .dst_access_mask = VK_ACCESS_SHADER_READ_BIT,
                .dst_stage_mask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            };
        } else {
            throw std::runtime_error("unsupported layout transition.");
        }
    }();

    VkImageMemoryBarrier barrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = masks.src_access_mask, // TODO
        .dstAccessMask = masks.dst_access_mask, // TODO
        .oldLayout = p_old_layout,
        .newLayout = p_new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
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
        vkQueueSubmit(device.graphics_queue, 1, &submit_info, VK_NULL_HANDLE)
    );
}
} // namespace mv
