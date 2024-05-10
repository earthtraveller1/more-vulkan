#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
    const auto pipeline = mv::graphics_pipeline_t::create(
        device, render_pass, "shaders/basic.vert.spv", "shaders/basic.frag.spv"
    );

    const auto command_pool = command_pool_t::create(device);
    const auto command_buffer = command_pool.allocate_buffer();

    const std::array<mv::vertex_t, 3> vertices{
        mv::vertex_t{{0.0f, -0.5f, 0.0f}},
        mv::vertex_t{{0.5f, 0.5f, 0.0f}},
        mv::vertex_t{{-0.5f, 0.5f, 0.0f}},
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
        vertex_buffer.buffer.copy_from(staging_buffer.buffer, command_pool.pool);
        vkQueueWaitIdle(device.graphics_queue);
    }

    const auto frame_fence = vulkan_fence_t::create(device);
    const auto image_available_semaphore = vulkan_semaphore_t::create(device);
    const auto render_done_semaphore = vulkan_semaphore_t::create(device);

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
