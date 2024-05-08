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
        VkPipeline p_pipeline, const render_pass_t &p_render_pass,
        VkPipelineLayout p_layout, const mv::vulkan_device_t &p_device
    )
        : pipeline(p_pipeline), layout(p_layout), render_pass(p_render_pass),
          device(p_device) {}

    static auto create(
        const mv::vulkan_device_t &device, const render_pass_t &p_render_pass,
        std::string_view vertex_shader_path,
        std::string_view fragment_shader_path
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

    static auto
    create(const mv::vulkan_device_t &device, const mv::swapchain_t &swapchain)
        -> render_pass_t;

    ~render_pass_t() {
        vkDestroyRenderPass(device.logical, render_pass, nullptr);
    }
};

struct buffer_t {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;

    enum class type_t { vertex, index, staging } type;

    const mv::vulkan_device_t &device;

    buffer_t(
        VkBuffer p_buffer, VkDeviceMemory p_memory, VkDeviceSize p_size,
        type_t p_type, const mv::vulkan_device_t &p_device
    )
        : buffer(p_buffer), memory(p_memory), size(p_size), type(p_type),
          device(p_device) {}

    static auto
    create(const mv::vulkan_device_t &device, VkDeviceSize size, type_t type) -> buffer_t;

    NO_COPY(buffer_t);
    YES_MOVE(buffer_t);

    ~buffer_t() {
        vkDestroyBuffer(device.logical, buffer, nullptr);
        vkFreeMemory(device.logical, memory, nullptr);
    }
};

struct vector_3f_t {
    float x;
    float y;
    float z;
};

struct vertex_t {
    vector_3f_t position;
};

constexpr VkVertexInputBindingDescription vertex_input_binding_description{
    .binding = 0,
    .stride = sizeof(vertex_t),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

constexpr static std::array<VkVertexInputAttributeDescription, 1>
    vertex_attribute_descriptions{VkVertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(vertex_t, position),
    }};

} // namespace mv
