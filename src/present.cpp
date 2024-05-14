#include <algorithm>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "device.hpp"
#include "errors.hpp"
#include "graphics.hpp"

#include "present.hpp"

using mv::window_t;

namespace {
auto framebuffer_size_callback(GLFWwindow *p_window, int p_width, int p_height) {
    const auto window = reinterpret_cast<window_t *>(glfwGetWindowUserPointer(p_window));
    window->width = p_width;
    window->height = p_height;
}
}

auto window_t::create(
    VkInstance p_instance, std::string_view p_title, int p_width, int p_height
) -> window_t {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    const auto window =
        glfwCreateWindow(p_width, p_height, p_title.data(), NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        throw mv::glfw_window_creation_failed_exception{};
    }

    VkSurfaceKHR surface;
    const auto result =
        glfwCreateWindowSurface(p_instance, window, NULL, &surface);
    if (result != VK_SUCCESS) {
        std::cerr << "[ERROR]: Failed to create the window surface.\n";

        glfwDestroyWindow(window);
        glfwTerminate();
        throw mv::vulkan_exception{result};
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    return {
        .window = window,
        .surface = surface,
        .instance = p_instance,
        .width = p_width,
        .height = p_height,
    };
}

auto mv::swapchain_t::create(
    const vulkan_device_t &p_device, const window_t &p_window
) -> swapchain_t {
    uint32_t surface_format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        p_device.physical, p_window.surface, &surface_format_count, nullptr
    );

    std::vector<VkSurfaceFormatKHR> surface_formats(surface_format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        p_device.physical, p_window.surface, &surface_format_count,
        surface_formats.data()
    );

    if (surface_formats.empty()) {
        throw no_adequate_swapchain_settings_exception{};
    }

    auto surface_format = surface_formats.front();
    for (const auto &format : surface_formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surface_format = format;
            break;
        }
    }

    uint32_t present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        p_device.physical, p_window.surface, &present_mode_count, nullptr
    );

    std::vector<VkPresentModeKHR> present_modes(present_mode_count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        p_device.physical, p_window.surface, &present_mode_count,
        present_modes.data()
    );

    if (present_modes.empty()) {
        throw no_adequate_swapchain_settings_exception{};
    }

    auto present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (auto mode : present_modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            present_mode = mode;
            break;
        }
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        p_device.physical, p_window.surface, &surface_capabilities
    );

    auto swap_extent = surface_capabilities.currentExtent;
    if (swap_extent.width == UINT32_MAX) {
        int width, height;
        glfwGetFramebufferSize(p_window.window, &width, &height);

        swap_extent = {
            .width = std::clamp(
                static_cast<uint32_t>(width),
                surface_capabilities.minImageExtent.width,
                surface_capabilities.maxImageExtent.width
            ),
            .height = std::clamp(
                static_cast<uint32_t>(height),
                surface_capabilities.minImageExtent.height,
                surface_capabilities.maxImageExtent.height
            ),
        };
    }

    std::vector<uint32_t> queue_family_indices;
    if (p_device.graphics_family == p_device.present_family) {
        queue_family_indices.push_back(p_device.graphics_family);
    } else {
        queue_family_indices.push_back(p_device.graphics_family);
        queue_family_indices.push_back(p_device.present_family);
    }

    VkSwapchainCreateInfoKHR swapchain_create_info{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = p_window.surface,
        .minImageCount = surface_capabilities.minImageCount + 1,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = swap_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_CONCURRENT,
        .queueFamilyIndexCount =
            static_cast<uint32_t>(queue_family_indices.size()),
        .pQueueFamilyIndices = queue_family_indices.data(),
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    if (swapchain_create_info.minImageCount >
        surface_capabilities.maxImageCount) {
        swapchain_create_info.minImageCount =
            surface_capabilities.maxImageCount;
    }

    VkSwapchainKHR swapchain;
    const auto result = vkCreateSwapchainKHR(
        p_device.logical, &swapchain_create_info, nullptr, &swapchain
    );
    if (result != VK_SUCCESS) {
        std::cerr << "[ERROR]: Failed to create the swapchain." << std::endl;
        throw mv::vulkan_exception{result};
    }

    uint32_t image_count;
    vkGetSwapchainImagesKHR(p_device.logical, swapchain, &image_count, nullptr);

    std::vector<VkImage> images(image_count);
    vkGetSwapchainImagesKHR(
        p_device.logical, swapchain, &image_count, images.data()
    );

    std::vector<VkImageView> image_views;

    for (const auto &image : images) {
        VkImageViewCreateInfo image_view_create_info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = image,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surface_format.format,
            .components =
                {
                    .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                    .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                },
            .subresourceRange =
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
        };

        VkImageView image_view;
        const auto result = vkCreateImageView(
            p_device.logical, &image_view_create_info, nullptr, &image_view
        );

        if (result != VK_SUCCESS) {
            std::cerr << "[ERROR]: Failed to create an image view for the "
                         "swapchain.\n";
            throw vulkan_exception{result};
        }

        image_views.push_back(image_view);
    }

    return {
        swapchain,
        p_device,
        std::move(images),
        std::move(image_views),
        surface_format.format,
        swap_extent
    };
}

auto mv::swapchain_t::create_framebuffers(const render_pass_t &render_pass
) const -> mv::swapchain_t::framebuffers_t {
    std::vector<VkFramebuffer> framebuffers;

    for (auto view : image_views) {
        const VkFramebufferCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass.render_pass,
            .attachmentCount = 1,
            .pAttachments = &view,
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
        };

        VkFramebuffer framebuffer;
        VK_ERROR(vkCreateFramebuffer(
            device.logical, &create_info, nullptr, &framebuffer
        ));

        framebuffers.push_back(framebuffer);
    }

    return { framebuffers, device };
}
