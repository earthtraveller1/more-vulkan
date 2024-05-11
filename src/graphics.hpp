#pragma once

#include <span>

#include <glm/glm.hpp>
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

    enum class type_t { vertex, index, staging, uniform };

    const mv::vulkan_device_t &device;

    buffer_t(
        VkBuffer p_buffer, VkDeviceMemory p_memory, VkDeviceSize p_size,
        const mv::vulkan_device_t &p_device
    )
        : buffer(p_buffer), memory(p_memory), size(p_size), device(p_device) {}

    static auto
    create(const mv::vulkan_device_t &device, VkDeviceSize size, type_t type)
        -> buffer_t;

    NO_COPY(buffer_t);
    YES_MOVE(buffer_t);

    auto copy_from(const buffer_t &other, VkCommandPool command_buffer) const
        -> void;

    ~buffer_t() {
        vkDestroyBuffer(device.logical, buffer, nullptr);
        vkFreeMemory(device.logical, memory, nullptr);
    }
};

struct vertex_buffer_t {
    buffer_t buffer;

    inline static auto create(const mv::vulkan_device_t &device, size_t size)
        -> vertex_buffer_t {
        return vertex_buffer_t{
            mv::buffer_t::create(device, size, mv::buffer_t::type_t::vertex),
        };
    }

    auto bind(VkCommandBuffer command_buffer, VkDeviceSize offset) const
        -> void {
        vkCmdBindVertexBuffers(command_buffer, 0, 1, &buffer.buffer, &offset);
    }
};

struct index_buffer_t {
    buffer_t buffer;

    inline static auto
    create(const mv::vulkan_device_t &device, VkDeviceSize size)
        -> index_buffer_t {
        return index_buffer_t{
            mv::buffer_t::create(device, size, mv::buffer_t::type_t::index),
        };
    }

    auto bind(
        VkCommandBuffer command_buffer, VkDeviceSize offset,
        VkIndexType index_type
    ) const -> void {
        vkCmdBindIndexBuffer(command_buffer, buffer.buffer, offset, index_type);
    }
};

struct staging_buffer_t {
    buffer_t buffer;

    inline static auto
    create(const mv::vulkan_device_t &device, VkDeviceSize size)
        -> staging_buffer_t {
        return staging_buffer_t{
            mv::buffer_t::create(device, size, mv::buffer_t::type_t::staging),
        };
    }

    auto map_memory() -> void * {
        void *data;
        vkMapMemory(
            buffer.device.logical, buffer.memory, 0, buffer.size, 0, &data
        );
        return data;
    }

    auto unmap_memory() -> void {
        vkUnmapMemory(buffer.device.logical, buffer.memory);
    }
};

struct uniform_buffer_t {
    buffer_t buffer;

    inline static auto
    create(const mv::vulkan_device_t &device, VkDeviceSize size)
        -> uniform_buffer_t {
        return uniform_buffer_t{
            mv::buffer_t::create(device, size, mv::buffer_t::type_t::uniform),
        };
    }

    inline static auto get_set_layout_binding(
        uint32_t binding, uint32_t descriptor_count,
        VkShaderStageFlags stage_flags
    ) -> VkDescriptorSetLayoutBinding {
        return {
            .binding = binding,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = descriptor_count,
            .stageFlags = stage_flags,
            .pImmutableSamplers = nullptr,
        };
    }

    inline auto get_descriptor_buffer_info() const -> VkDescriptorBufferInfo {
        return {
            .buffer = buffer.buffer,
            .offset = 0,
            .range = buffer.size,
        };
    }

    auto map_memory() -> void * {
        void *data;
        vkMapMemory(
            buffer.device.logical, buffer.memory, 0, buffer.size, 0, &data
        );
        return data;
    }

    auto unmap_memory() -> void {
        vkUnmapMemory(buffer.device.logical, buffer.memory);
    }
};
struct vertex_t {
    glm::vec3 position;
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
