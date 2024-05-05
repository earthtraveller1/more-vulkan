#include <array>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "enumerate.hpp"

namespace {
struct glfw_init_failed_exception : public std::exception {
    virtual const char *what() const noexcept override {
        return "Failed to initialize GLFW.";
    }
};

struct glfw_window_creation_failed_exception : public std::exception {
    virtual const char *what() const noexcept override {
        return "Failed to create the GLFW window.";
    }
};

struct vulkan_exception : public std::exception {
    VkResult error_code;

    vulkan_exception(VkResult p_error_code) : error_code(p_error_code) {}

    virtual const char *what() const noexcept override {
        return "A Vulkan error occured.";
    }
};

struct no_adequate_devices_exception : public std::exception {
    virtual const char *what() const noexcept override {
        return "No adequate devices found.";
    }
};

struct window_t {
    GLFWwindow *window;
    VkSurfaceKHR surface;
    VkInstance instance;

    // May throw glfw_init_failed_exception and
    // glfw_window_creation_failed_exception
    static auto create(
        VkInstance p_instance, std::string_view p_title, int p_width,
        int p_height
    ) -> window_t {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        const auto window =
            glfwCreateWindow(p_width, p_height, p_title.data(), NULL, NULL);
        if (window == NULL) {
            glfwTerminate();
            throw glfw_window_creation_failed_exception{};
        }

        VkSurfaceKHR surface;
        const auto result =
            glfwCreateWindowSurface(p_instance, window, NULL, &surface);
        if (result != VK_SUCCESS) {
            std::cerr << "[ERROR]: Failed to create the window surface.\n";

            glfwDestroyWindow(window);
            glfwTerminate();
            throw vulkan_exception{result};
        }

        return {
            .window = window,
            .surface = surface,
            .instance = p_instance,
        };
    }

    ~window_t() {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
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

            std::vector<VkQueueFamilyProperties> queue_families(
                queue_family_count
            );
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

            if (graphics_family.has_value() && present_family.has_value()) {
                device.physical = physical_device;
                device.graphics_family = graphics_family.value();
                device.present_family = present_family.value();
                break;
            }
        }

        if (device.physical == VK_NULL_HANDLE) {
            throw no_adequate_devices_exception{};
        }

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

        const VkDeviceCreateInfo device_create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount =
                static_cast<uint32_t>(queue_create_infos.size()),
            .pQueueCreateInfos = queue_create_infos.data(),
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = 0,
            .ppEnabledExtensionNames = nullptr,
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

    ~vulkan_device_t() {
        vkDestroyDevice(logical, nullptr);
    }
};

struct vulkan_instance_t {
    VkInstance instance;

    // May throw vulkan_exception
    static auto create(bool p_enable_validation) -> vulkan_instance_t {
        if (p_enable_validation) {
            uint32_t layer_count;
            vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

            std::vector<VkLayerProperties> layers(layer_count);
            vkEnumerateInstanceLayerProperties(&layer_count, layers.data());

            bool layer_found = false;

            for (const auto &layer : layers) {
                if (std::strcmp(
                        layer.layerName, "VK_LAYER_KHRONOS_validation"
                    ) == 0) {
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

        const VkInstanceCreateInfo instance_create_info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = nullptr,
            .pApplicationInfo = &application_info,
            .enabledLayerCount = validation_layer_count,
            .ppEnabledLayerNames = validation_layers,
            .enabledExtensionCount = glfw_extension_count,
            .ppEnabledExtensionNames = glfw_extensions,
        };

        VkInstance instance;
        const auto result =
            vkCreateInstance(&instance_create_info, nullptr, &instance);
        if (result != VK_SUCCESS) {
            std::cerr << "[ERROR]: Failed to create the Vulkan instance.\n";
            throw vulkan_exception{result};
        }

        return vulkan_instance_t{instance};
    }

    operator VkInstance() const noexcept { return instance; }

    ~vulkan_instance_t() {
        vkDestroyInstance(instance, nullptr);
    }
};

} // namespace

int main(int p_argc, const char *const *const p_argv) {
    bool enable_validation = false;

    for (auto arg = p_argv; arg < p_argv + p_argc; ++arg) {
        if (std::strcmp(*arg, "--validation") == 0) {
            enable_validation = true;
        }
    }

    if (!glfwInit()) {
        throw glfw_init_failed_exception{};
    }

    try {
        const auto instance = vulkan_instance_t::create(enable_validation);
        const auto window = window_t::create(instance, "Hello!", 1280, 720);
        const auto device = vulkan_device_t::create(instance, window.surface);

        glfwShowWindow(window.window);
        while (!glfwWindowShouldClose(window.window)) {
            glfwPollEvents();
        }
    } catch (const vulkan_exception &e) {
        std::cerr << "[ERROR]: Vulkan error " << e.error_code << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception &e) {
        std::cerr << "[ERROR]: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
