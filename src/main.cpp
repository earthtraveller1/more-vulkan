#include <GLFW/glfw3.h>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string_view>
#include <vector>

#include <vulkan/vulkan.h>

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

// May throw glfw_init_failed_exception and
// glfw_window_creation_failed_exception
auto create_window(std::string_view p_title, int p_width, int p_height)
    -> GLFWwindow * {
    if (!glfwInit()) {
        throw glfw_init_failed_exception{};
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    const auto window =
        glfwCreateWindow(p_width, p_height, p_title.data(), NULL, NULL);
    if (window == NULL) {
        glfwTerminate();
        throw glfw_window_creation_failed_exception{};
    }

    return window;
}

// May throw vulkan_exception
auto create_vulkan_instance(bool p_enable_validation) -> VkInstance {
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

    uint32_t glfw_extension_count;
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
        throw vulkan_exception{result};
    }

    return instance;
}
} // namespace

int main(int p_argc, const char *const * const p_argv) {
    bool enable_validation = false;

    for (auto arg = p_argv; arg < p_argv + p_argc; ++arg) {
        if (std::strcmp(*arg, "--validation") == 0) {
            enable_validation = true;
        }
    }

    try {
        const auto instance = create_vulkan_instance(enable_validation);
        const auto window = create_window("Hello!", 1280, 720);

        glfwShowWindow(window);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }

        glfwDestroyWindow(window);
        glfwTerminate();

        vkDestroyInstance(instance, nullptr);
    } catch (const vulkan_exception &e) {
        std::cerr << "[ERROR]: Vulkan error " << e.error_code << '\n';
        return EXIT_FAILURE;
    } catch (const std::exception &e) {
        std::cerr << "[ERROR]: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
