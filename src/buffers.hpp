#pragma once 

#include <vulkan/vulkan.h>

#include "common.hpp"
#include "device.hpp"

namespace mv {
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

    auto load_using_staging(VkCommandPool command_pool, const void* data, VkDeviceSize size) -> void;

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

    auto map_memory() const -> void * {
        void *data;
        vkMapMemory(
            buffer.device.logical, buffer.memory, 0, buffer.size, 0, &data
        );
        return data;
    }

    auto unmap_memory() const -> void {
        vkUnmapMemory(buffer.device.logical, buffer.memory);
    }
};
}
