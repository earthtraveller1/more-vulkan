#pragma once

#include "common.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "device.hpp"

namespace mv {

struct window_t {
    GLFWwindow *window;
    VkSurfaceKHR surface;
    VkInstance instance;

    static auto create(
        VkInstance p_instance, std::string_view p_title, int p_width,
        int p_height
    ) -> window_t;

    inline ~window_t() {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

struct swapchain_t {
    VkSwapchainKHR swapchain;
    const vulkan_device_t &device;

    swapchain_t(VkSwapchainKHR p_swapchain, const vulkan_device_t &p_device)
        : swapchain(p_swapchain), device(p_device) {}

    static auto create(const vulkan_device_t &device, const window_t &window)
        -> swapchain_t;

    NO_COPY(swapchain_t);
    YES_MOVE(swapchain_t);

    inline ~swapchain_t() {
        vkDestroySwapchainKHR(device.logical, swapchain, nullptr);
    }
};
} // namespace mv
