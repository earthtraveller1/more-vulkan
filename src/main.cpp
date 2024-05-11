#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "device.hpp"
#include "enumerate.hpp"
#include "errors.hpp"
#include "graphics.hpp"
#include "present.hpp"

using mv::vulkan_device_t;

namespace {
struct command_pool_t {
    VkCommandPool pool;
    const vulkan_device_t &device;

    command_pool_t(VkCommandPool p_pool, const vulkan_device_t &p_device)
        : pool(p_pool), device(p_device) {}

    static auto create(const vulkan_device_t &p_device) -> command_pool_t {
        const VkCommandPoolCreateInfo pool_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = p_device.graphics_family,
        };

        VkCommandPool pool;
        VK_ERROR(
            vkCreateCommandPool(p_device.logical, &pool_info, nullptr, &pool)
        );
        return command_pool_t(pool, p_device);
    }

    NO_COPY(command_pool_t);
    YES_MOVE(command_pool_t);

    auto allocate_buffer() const -> VkCommandBuffer {
        VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer buffer;
        VK_ERROR(vkAllocateCommandBuffers(device.logical, &alloc_info, &buffer)
        );
        return buffer;
    }

    ~command_pool_t() {
        vkDestroyCommandPool(device.logical, pool, nullptr);
    }
};

struct vulkan_semaphore_t {
    VkSemaphore semaphore;
    const vulkan_device_t &device;

    vulkan_semaphore_t(VkSemaphore p_semaphore, const vulkan_device_t &p_device)
        : semaphore(p_semaphore), device(p_device) {}

    NO_COPY(vulkan_semaphore_t);
    YES_MOVE(vulkan_semaphore_t);

    static auto create(const vulkan_device_t &p_device) -> vulkan_semaphore_t {
        const VkSemaphoreCreateInfo semaphore_info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };

        VkSemaphore semaphore;
        VK_ERROR(vkCreateSemaphore(
            p_device.logical, &semaphore_info, nullptr, &semaphore
        ));
        return vulkan_semaphore_t(semaphore, p_device);
    }

    ~vulkan_semaphore_t() {
        vkDestroySemaphore(device.logical, semaphore, nullptr);
    }
};

struct vulkan_fence_t {
    VkFence fence;
    const vulkan_device_t &device;

    vulkan_fence_t(VkFence p_fence, const vulkan_device_t &p_device)
        : fence(p_fence), device(p_device) {}

    NO_COPY(vulkan_fence_t);
    YES_MOVE(vulkan_fence_t);

    static auto create(const vulkan_device_t &p_device) -> vulkan_fence_t {
        const VkFenceCreateInfo fence_info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };

        VkFence fence;
        VK_ERROR(vkCreateFence(p_device.logical, &fence_info, nullptr, &fence));
        return vulkan_fence_t(fence, p_device);
    }

    ~vulkan_fence_t() {
        vkDestroyFence(device.logical, fence, nullptr);
    }
};

struct push_constants_t {
    float t;
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
    ) -> descriptor_set_layout_t {
        const VkDescriptorSetLayoutCreateInfo layout_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data(),
        };

        VkDescriptorSetLayout layout;
        VK_ERROR(vkCreateDescriptorSetLayout(
            p_device.logical, &layout_info, nullptr, &layout
        ));
        return descriptor_set_layout_t{layout, p_device};
    }

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
        std::span<const VkDescriptorPoolSize> pool_sizes
    ) -> descriptor_pool_t {
        const VkDescriptorPoolCreateInfo pool_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets = 1,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };

        VkDescriptorPool pool;
        VK_ERROR(
            vkCreateDescriptorPool(p_device.logical, &pool_info, nullptr, &pool)
        );
        return descriptor_pool_t{pool, p_device};
    }

    auto allocate_descriptor_set(const descriptor_set_layout_t &layout) const
        -> VkDescriptorSet {
        VkDescriptorSetAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pool,
            .descriptorSetCount = 1,
            .pSetLayouts = &layout.layout,
        };

        VkDescriptorSet set;
        VK_ERROR(vkAllocateDescriptorSets(device.logical, &alloc_info, &set));
        return set;
    }

    NO_COPY(descriptor_pool_t);
    YES_MOVE(descriptor_pool_t);

    ~descriptor_pool_t() {
        vkDestroyDescriptorPool(device.logical, pool, nullptr);
    }
};

struct uniform_buffer_object_t {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
};

} // namespace

int main(int p_argc, const char *const *const p_argv) try {
    bool enable_validation = false;

    for (auto arg = p_argv; arg < p_argv + p_argc; ++arg) {
        if (std::strcmp(*arg, "--validation") == 0) {
            enable_validation = true;
            std::cout << "[INFO]: Enabling validation layers.\n";
        }
    }

    if (!glfwInit()) {
        throw mv::glfw_init_failed_exception{};
    }

    const auto instance = mv::vulkan_instance_t::create(enable_validation);
    const auto window = mv::window_t::create(instance, "Hello!", 1280, 720);
    const auto device = mv::vulkan_device_t::create(instance, window.surface);
    const auto swapchain = mv::swapchain_t::create(device, window);
    const auto render_pass = mv::render_pass_t::create(device, swapchain);
    const auto framebuffers = swapchain.create_framebuffers(render_pass);

    const auto uniform_buffer_descriptor_layout =
        descriptor_set_layout_t::create(
            device,
            std::array<VkDescriptorSetLayoutBinding, 1>{
                mv::uniform_buffer_t::get_set_layout_binding(
                    0, 1, VK_SHADER_STAGE_VERTEX_BIT
                ),
            }
        );

    const auto pipeline = mv::graphics_pipeline_t::create(
        device, render_pass, "shaders/basic.vert.spv", "shaders/basic.frag.spv",
        std::array<VkPushConstantRange, 1>{VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(push_constants_t),
        }},
        std::array<VkDescriptorSetLayout, 1>{
            uniform_buffer_descriptor_layout.layout,
        }
    );

    const auto command_pool = command_pool_t::create(device);
    const auto command_buffer = command_pool.allocate_buffer();

    const std::array<mv::vertex_t, 4> vertices{
        mv::vertex_t{{-0.5f, -0.5f, 0.0f}},
        mv::vertex_t{{-0.5f, 0.5f, 0.0f}},
        mv::vertex_t{{0.5f, 0.5f, 0.0f}},
        mv::vertex_t{{0.5f, -0.5f, 0.0f}},
    };

    const auto vertex_buffer = mv::vertex_buffer_t::create(
        device, vertices.size() * sizeof(mv::vertex_t)
    );
    {
        auto staging_buffer = mv::staging_buffer_t::create(
            device, vertices.size() * sizeof(mv::vertex_t)
        );
        const auto data = staging_buffer.map_memory();
        std::memcpy(
            data, vertices.data(), vertices.size() * sizeof(mv::vertex_t)
        );
        staging_buffer.unmap_memory();
        vertex_buffer.buffer.copy_from(
            staging_buffer.buffer, command_pool.pool
        );
        vkQueueWaitIdle(device.graphics_queue);
    }

    const std::array<uint16_t, 6> indices{0, 1, 2, 0, 2, 3};

    const auto index_buffer =
        mv::index_buffer_t::create(device, indices.size() * sizeof(uint16_t));

    {
        auto staging_buffer = mv::staging_buffer_t::create(
            device, indices.size() * sizeof(uint16_t)
        );
        const auto data = staging_buffer.map_memory();
        std::memcpy(data, indices.data(), indices.size() * sizeof(uint16_t));
        staging_buffer.unmap_memory();
        index_buffer.buffer.copy_from(staging_buffer.buffer, command_pool.pool);
        vkQueueWaitIdle(device.graphics_queue);
    }

    const auto uniform_buffer =
        mv::uniform_buffer_t::create(device, sizeof(uniform_buffer_object_t));

    const auto frame_fence = vulkan_fence_t::create(device);
    const auto image_available_semaphore = vulkan_semaphore_t::create(device);
    const auto render_done_semaphore = vulkan_semaphore_t::create(device);

    const auto descriptor_pool = descriptor_pool_t::create(
        device, std::array<VkDescriptorPoolSize, 1>{VkDescriptorPoolSize{
                    .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                }}
    );

    const auto descriptor_set =
        descriptor_pool.allocate_descriptor_set(uniform_buffer_descriptor_layout
        );

    {
        const auto buffer_info = uniform_buffer.get_descriptor_buffer_info();

        VkWriteDescriptorSet set_write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .pBufferInfo = &buffer_info,
        };

        vkUpdateDescriptorSets(device.logical, 1, &set_write, 0, nullptr);
    }

    uniform_buffer_object_t ubo{
        .projection = glm::perspective(45.0f, 1280.0f / 720.0f, 0.1f, 100.0f),
        .view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -4.0f)),
        .model = glm::mat4(1.0f)
    };

    glfwShowWindow(window.window);
    while (!glfwWindowShouldClose(window.window)) {
        vkWaitForFences(
            device.logical, 1, &frame_fence.fence, VK_TRUE, UINT64_MAX
        );
        vkResetFences(device.logical, 1, &frame_fence.fence);

        uint32_t image_index;
        VK_ERROR(vkAcquireNextImageKHR(
            device.logical, swapchain.swapchain, UINT64_MAX,
            image_available_semaphore.semaphore, VK_NULL_HANDLE, &image_index
        ));

        vkResetCommandBuffer(command_buffer, 0);

        const VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };

        VK_ERROR(vkBeginCommandBuffer(command_buffer, &begin_info));

        const VkClearValue clear_color{
            .color = {.float32 = {0.0f, 0.01f, 0.01f, 1.0f}}
        };

        const VkRenderPassBeginInfo render_pass_begin_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = render_pass.render_pass,
            .framebuffer = framebuffers.framebuffers.at(image_index),
            .renderArea =
                {
                    .offset = {0, 0},
                    .extent = swapchain.extent,
                },
            .clearValueCount = 1,
            .pClearValues = &clear_color,
        };

        vkCmdBeginRenderPass(
            command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE
        );

        vkCmdBindPipeline(
            command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline
        );

        const auto data = uniform_buffer.map_memory();
        memcpy(data, &ubo, sizeof ubo);
        uniform_buffer.unmap_memory();

        vkCmdBindDescriptorSets(
            command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, 0,
            1, &descriptor_set, 0, nullptr
        );

        const VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swapchain.extent.width),
            .height = static_cast<float>(swapchain.extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        const VkRect2D scissor{
            .offset = {0, 0},
            .extent = swapchain.extent,
        };

        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(
            command_buffer, 0, 1, &vertex_buffer.buffer.buffer, &offset
        );

        vkCmdBindIndexBuffer(
            command_buffer, index_buffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT16
        );

        const push_constants_t push_constants{
            .t = static_cast<float>(glfwGetTime()),
        };

        vkCmdPushConstants(
            command_buffer, pipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0,
            sizeof(push_constants_t), &push_constants
        );

        // vkCmdDraw(command_buffer, 3, 1, 0, 0);
        vkCmdDrawIndexed(command_buffer, indices.size(), 1, 0, 0, 1);

        vkCmdEndRenderPass(command_buffer);
        VK_ERROR(vkEndCommandBuffer(command_buffer));

        const VkPipelineStageFlags wait_stage =
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &image_available_semaphore.semaphore,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_done_semaphore.semaphore,
        };

        VK_ERROR(vkQueueSubmit(
            device.graphics_queue, 1, &submit_info, frame_fence.fence
        ));

        const VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_done_semaphore.semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain.swapchain,
            .pImageIndices = &image_index,
        };

        VK_ERROR(vkQueuePresentKHR(device.present_queue, &present_info));

        glfwPollEvents();
    }

    vkDeviceWaitIdle(device.logical);
} catch (const mv::vulkan_exception &e) {
    std::cerr << "[ERROR]: Vulkan error " << e.error_code << '\n';
    return EXIT_FAILURE;
} catch (const std::exception &e) {
    std::cerr << "[ERROR]: " << e.what() << '\n';
    return EXIT_FAILURE;
}
