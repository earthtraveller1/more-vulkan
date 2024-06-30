#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mv {
struct vulkan_instance_t {
    VkInstance instance;
    VkDebugUtilsMessengerEXT messenger;

    // May throw vulkan_exception
    static auto create(bool p_enable_validation) -> vulkan_instance_t;

    inline operator VkInstance() const noexcept {
        return instance;
    }

    inline ~vulkan_instance_t() {
        if (messenger != VK_NULL_HANDLE) {
            const auto vk_destroy_debug_utils_messenger_ext =
                reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                    vkGetInstanceProcAddr(
                        instance, "vkDestroyDebugUtilsMessengerEXT"
                    )
                );

            if (vk_destroy_debug_utils_messenger_ext != nullptr) {
                vk_destroy_debug_utils_messenger_ext(
                    instance, messenger, nullptr
                );
            }
        }

        vkDestroyInstance(instance, nullptr);
    }
};

struct vulkan_device_t {
    VkPhysicalDevice physical;
    VkDevice logical;

    uint32_t graphics_family;
    uint32_t present_family;

    VkQueue graphics_queue;
    VkQueue present_queue;

    // May throw vulkan_exception
    static auto create(VkInstance p_instance, VkSurfaceKHR p_surface)
        -> vulkan_device_t;

    inline ~vulkan_device_t() {
        vkDestroyDevice(logical, nullptr);
    }
};
} // namespace mv
