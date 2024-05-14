#pragma once

#include "common.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "device.hpp"

namespace mv {

struct render_pass_t;

struct window_t {
    GLFWwindow *window;
    VkSurfaceKHR surface;
    VkInstance instance;

    int width, height;

    static auto create(
        VkInstance p_instance, std::string_view p_title, int p_width,
        int p_height
    ) -> window_t;

    inline auto set_user_pointer() -> void {
        glfwSetWindowUserPointer(window, this);
    }

    inline ~window_t() {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

struct swapchain_t {
    VkSwapchainKHR swapchain;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
    VkFormat format;
    VkExtent2D extent;

    const vulkan_device_t *device;

    swapchain_t(
        VkSwapchainKHR p_swapchain, const vulkan_device_t &p_device,
        std::vector<VkImage> &&p_images,
        std::vector<VkImageView> &&p_image_views, VkFormat p_format,
        VkExtent2D p_extent
    )
        : swapchain(p_swapchain), images(p_images), image_views(p_image_views),
          format(p_format), extent(p_extent), device(&p_device) {}

    static auto create(const vulkan_device_t &device, const window_t &window)
        -> swapchain_t;

    struct framebuffers_t {
        std::vector<VkFramebuffer> framebuffers;
        const vulkan_device_t& device;

        ~framebuffers_t() {
            for (auto framebuffer : framebuffers) {
                vkDestroyFramebuffer(device.logical, framebuffer, nullptr);
            }
        }
    };

    auto create_framebuffers(const render_pass_t &render_pass) const 
        -> framebuffers_t;

    NO_COPY(swapchain_t);
    YES_MOVE(swapchain_t);
    YES_MOVE_ASSIGN(swapchain_t);

    inline ~swapchain_t() {
        for (auto view : image_views) {
            vkDestroyImageView(device->logical, view, nullptr);
        }

        vkDestroySwapchainKHR(device->logical, swapchain, nullptr);
    }
};
} // namespace mv
