#include <GLFW/glfw3.h>

#include "enumerate.hpp"
#include "errors.hpp"

#include "device.hpp"

using mv::vulkan_device_t;
using mv::vulkan_instance_t;

auto vulkan_instance_t::create(bool p_enable_validation) -> vulkan_instance_t {
    if (p_enable_validation) {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, layers.data());

        bool layer_found = false;

        for (const auto &layer : layers) {
            if (std::strcmp(layer.layerName, "VK_LAYER_KHRONOS_validation") ==
                0) {
                layer_found = true;
            }
        }

        if (!layer_found) {
            throw vulkan_exception{VK_ERROR_LAYER_NOT_PRESENT};
        }
    }

    const VkApplicationInfo application_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Stupid Vulkan App",
        .apiVersion = VK_API_VERSION_1_2,
    };

    uint32_t glfw_extension_count = 0;
    const auto glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    const auto validation_layer_count = 1;
    const char *const validation_layers[validation_layer_count] = {
        "VK_LAYER_KHRONOS_validation"
    };

    VkInstanceCreateInfo instance_create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = glfw_extension_count,
        .ppEnabledExtensionNames = glfw_extensions,
    };

    if (p_enable_validation) {
        instance_create_info.enabledLayerCount = validation_layer_count;
        instance_create_info.ppEnabledLayerNames = validation_layers;
    }

    VkInstance instance;
    const auto result =
        vkCreateInstance(&instance_create_info, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cerr << "[ERROR]: Failed to create the Vulkan instance.\n";
        throw vulkan_exception{result};
    }

    return vulkan_instance_t{instance};
}

// May throw vulkan_exception
auto vulkan_device_t::create(VkInstance p_instance, VkSurfaceKHR p_surface)
    -> vulkan_device_t {
    uint32_t device_count;
    vkEnumeratePhysicalDevices(p_instance, &device_count, nullptr);

    if (device_count == 0) {
        throw no_adequate_devices_exception{};
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(p_instance, &device_count, devices.data());

    vulkan_device_t device{};

    for (const auto &physical_device : devices) {
        uint32_t queue_family_count;
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &queue_family_count, nullptr
        );

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(
            physical_device, &queue_family_count, queue_families.data()
        );

        std::optional<uint32_t> graphics_family, present_family;

        for (auto [i, queue_family] :
             enumerate(queue_families.begin(), queue_families.end())) {
            if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphics_family = i;
            }

            VkBool32 present_support = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(
                physical_device, i, p_surface, &present_support
            );

            if (present_support == VK_TRUE) {
                present_family = i;
            }
        }

        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(
            physical_device, nullptr, &extension_count, nullptr
        );

        std::vector<VkExtensionProperties> extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(
            physical_device, nullptr, &extension_count, extensions.data()
        );

        bool supports_swapchain = false;

        for (const auto &extension : extensions) {
            if (std::strcmp(
                    extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME
                ) == 0) {
                supports_swapchain = true;
            }
        }

        uint32_t surface_format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physical_device, p_surface, &surface_format_count, nullptr
        );

        uint32_t present_mode_count;
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            physical_device, p_surface, &present_mode_count, nullptr
        );

        if (graphics_family.has_value() && present_family.has_value() &&
            supports_swapchain && surface_format_count > 0 &&
            present_mode_count > 0) {
            device.physical = physical_device;
            device.graphics_family = graphics_family.value();
            device.present_family = present_family.value();
            break;
        }
    }

    if (device.physical == VK_NULL_HANDLE) {
        throw no_adequate_devices_exception{};
    }

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(device.physical, &device_properties);

    std::cout << "[INFO]: Selected the " << device_properties.deviceName
              << " graphics card.\n";

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

    float queue_priority = 1.0f;
    if (device.graphics_family != device.present_family) {
        std::array<uint32_t, 2> queue_family_indices{
            device.graphics_family, device.present_family
        };

        for (auto index : queue_family_indices) {
            queue_create_infos.push_back({
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .pNext = nullptr,
                .flags = 0,
                .queueFamilyIndex = index,
                .queueCount = 1,
                .pQueuePriorities = &queue_priority,
            });
        }
    } else {
        queue_create_infos.push_back({
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = device.graphics_family,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority,
        });
    }

    const std::array<const char *, 1> enabled_extensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    const VkDeviceCreateInfo device_create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount =
            static_cast<uint32_t>(queue_create_infos.size()),
        .pQueueCreateInfos = queue_create_infos.data(),
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
        .enabledExtensionCount = enabled_extensions.size(),
        .ppEnabledExtensionNames = enabled_extensions.data(),
        .pEnabledFeatures = nullptr,
    };

    auto result = vkCreateDevice(
        device.physical, &device_create_info, nullptr, &device.logical
    );
    if (result != VK_SUCCESS) {
        std::cerr << "[ERROR]: Failed to create the logical device.\n";
        throw vulkan_exception{result};
    }

    return device;
}
