#pragma once

#include "common.hpp"
#include "images.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "device.hpp"

namespace mv {

struct render_pass_t;
struct vulkan_image_t;

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

    swapchain_t() = default;

    swapchain_t(
        VkSwapchainKHR p_swapchain, const vulkan_device_t &p_device,
        std::vector<VkImage> &&p_images,
        std::vector<VkImageView> &&p_image_views, VkFormat p_format,
        VkExtent2D p_extent
    )
        : swapchain(p_swapchain), images(p_images), image_views(p_image_views),
          format(p_format), extent(p_extent), device(&p_device) {
    }

    static auto create(const vulkan_device_t &device, const window_t &window)
        -> swapchain_t;

    struct framebuffers_t {
        std::vector<VkFramebuffer> framebuffers;
        const vulkan_device_t *device;

        framebuffers_t() = default;

        framebuffers_t(
            std::vector<VkFramebuffer> &&p_framebuffers,
            const vulkan_device_t &p_device
        )
            : framebuffers(p_framebuffers), device(&p_device) {}

        NO_COPY(framebuffers_t);

        YES_MOVE(framebuffers_t);

        auto operator=(framebuffers_t &&other) noexcept -> framebuffers_t & {
            fucking_destroy();

            framebuffers = std::move(other.framebuffers);
            device = other.device;

            other.device = nullptr;
            other.framebuffers.clear();
            return *this;
        }

        auto fucking_destroy() noexcept -> void {
            if (device == nullptr)
                return;

            for (auto framebuffer : framebuffers) {
                vkDestroyFramebuffer(device->logical, framebuffer, nullptr);
            }
        }

        ~framebuffers_t() {
            fucking_destroy();
        }
    };

    auto create_framebuffers(const render_pass_t &render_pass, const vulkan_image_view_t& depth_buffer) const
        -> framebuffers_t;

    NO_COPY(swapchain_t);
    YES_MOVE(swapchain_t);

    auto operator=(swapchain_t &&other) noexcept -> swapchain_t & {
        fucking_destroy();

        swapchain = other.swapchain;
        images = std::move(other.images);
        image_views = std::move(other.image_views);
        format = other.format;
        extent = other.extent;
        device = other.device;

        other.swapchain = VK_NULL_HANDLE;
        other.device = nullptr;
        other.images.clear();
        other.image_views.clear();
        return *this;
    }

    inline auto fucking_destroy() noexcept -> void {
        if (device == nullptr)
            return;

        for (auto view : image_views) {
            vkDestroyImageView(device->logical, view, nullptr);
        }

        if (swapchain != VK_NULL_HANDLE) {
            TRACE(vkDestroySwapchainKHR(device->logical, swapchain, nullptr));
        }
    }

    inline ~swapchain_t() {
        fucking_destroy();
    }
};
} // namespace mv
