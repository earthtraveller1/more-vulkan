#pragma once

#include <vulkan/vulkan.h>

#include "common.hpp"
#include "device.hpp"
#include "present.hpp"

namespace mv {
struct render_pass_t;

struct graphics_pipeline_t {
    VkPipeline pipeline;
    VkPipelineLayout layout;

    const render_pass_t &render_pass;

    const mv::vulkan_device_t &device;

    graphics_pipeline_t(
        VkPipeline p_pipeline,
        const render_pass_t &p_render_pass,
        VkPipelineLayout p_layout,
        const mv::vulkan_device_t &p_device
    )
        : pipeline(p_pipeline), layout(p_layout), render_pass(p_render_pass),
          device(p_device) {}

    static auto create(
        const mv::vulkan_device_t &device,
        const render_pass_t &p_render_pass,
        std::string_view vertex_shader_path,
        std::string_view fragment_shader_path,
        std::span<const VkPushConstantRange> push_constant_ranges,
        std::span<const VkDescriptorSetLayout> descriptor_set_layouts
    ) -> graphics_pipeline_t;

    NO_COPY(graphics_pipeline_t);
    YES_MOVE(graphics_pipeline_t);

    ~graphics_pipeline_t() {
        vkDestroyPipelineLayout(device.logical, layout, nullptr);
        vkDestroyPipeline(device.logical, pipeline, nullptr);
    }
};

struct render_pass_t {
    VkRenderPass render_pass;
    const mv::vulkan_device_t &device;

    render_pass_t(
        VkRenderPass p_render_pass, const mv::vulkan_device_t &p_device
    )
        : render_pass(p_render_pass), device(p_device) {}

    NO_COPY(render_pass_t);
    YES_MOVE(render_pass_t);

    static auto create(
        const mv::vulkan_device_t &device,
        std::optional<VkFormat> color_format,
        std::optional<VkFormat> depth_format,
        std::span<const VkSubpassDependency> subpass_dependencies =
            std::array<VkSubpassDependency, 0>{},
        VkImageLayout final_depth_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    ) -> render_pass_t;

    ~render_pass_t() {
        vkDestroyRenderPass(device.logical, render_pass, nullptr);
    }
};

struct framebuffer_t {
    VkFramebuffer framebuffer;
    const vulkan_device_t &device;

    ~framebuffer_t() {
        vkDestroyFramebuffer(device.logical, framebuffer, nullptr);
    }
};

auto create_framebuffer(
    const vulkan_device_t &device,
    const vulkan_image_view_t &image_view,
    uint32_t width,
    uint32_t height,
    const render_pass_t &render_pass
) -> framebuffer_t;

struct vertex_t {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 normal;
    float id;
};

constexpr VkVertexInputBindingDescription vertex_input_binding_description{
    .binding = 0,
    .stride = sizeof(vertex_t),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

constexpr static std::array vertex_attribute_descriptions{
    VkVertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(vertex_t, position),
    },
    VkVertexInputAttributeDescription{
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32_SFLOAT,
        .offset = offsetof(vertex_t, uv),
    },
    VkVertexInputAttributeDescription{
        .location = 2,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(vertex_t, normal),
    },
    VkVertexInputAttributeDescription{
        .location = 3,
        .binding = 0,
        .format = VK_FORMAT_R32_SFLOAT,
        .offset = offsetof(vertex_t, id),
    }
};

struct descriptor_set_layout_t {
    VkDescriptorSetLayout layout;
    const vulkan_device_t &device;

    descriptor_set_layout_t(
        VkDescriptorSetLayout p_layout, const vulkan_device_t &p_device
    )
        : layout(p_layout), device(p_device) {}

    NO_COPY(descriptor_set_layout_t);
    YES_MOVE(descriptor_set_layout_t);

    static auto create(
        const vulkan_device_t &p_device,
        std::span<const VkDescriptorSetLayoutBinding> bindings
    ) -> descriptor_set_layout_t;

    ~descriptor_set_layout_t() {
        vkDestroyDescriptorSetLayout(device.logical, layout, nullptr);
    }
};

struct descriptor_pool_t {
    VkDescriptorPool pool;
    const vulkan_device_t &device;

    descriptor_pool_t(VkDescriptorPool p_pool, const vulkan_device_t &p_device)
        : pool(p_pool), device(p_device) {}

    static auto create(
        const vulkan_device_t &p_device,
        std::span<const VkDescriptorPoolSize> pool_sizes,
        uint32_t max_sets
    ) -> descriptor_pool_t;

    auto allocate_descriptor_set(const descriptor_set_layout_t &layout
    ) const -> VkDescriptorSet;

    NO_COPY(descriptor_pool_t);
    YES_MOVE(descriptor_pool_t);

    ~descriptor_pool_t() {
        vkDestroyDescriptorPool(device.logical, pool, nullptr);
    }
};

} // namespace mv
