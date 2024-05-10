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

    auto allocate_buffer() -> VkCommandBuffer {
        VkCommandBufferAllocateInfo alloc_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer buffer;
        VK_ERROR(vkAllocateCommandBuffers(device.logical, &alloc_info, &buffer));
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
        VK_ERROR(vkCreateSemaphore(p_device.logical, &semaphore_info, nullptr, &semaphore));
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
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
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

    glfwShowWindow(window.window);
    while (!glfwWindowShouldClose(window.window)) {
        glfwPollEvents();
    }
} catch (const mv::vulkan_exception &e) {
    std::cerr << "[ERROR]: Vulkan error " << e.error_code << '\n';
    return EXIT_FAILURE;
} catch (const std::exception &e) {
    std::cerr << "[ERROR]: " << e.what() << '\n';
    return EXIT_FAILURE;
}
