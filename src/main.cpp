#include <GLFW/glfw3.h>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

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

auto create_vulkan_instance() -> VkInstance {
    const VkApplicationInfo application_info{
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Stupid Vulkan App",
        .apiVersion = VK_API_VERSION_1_2,
    };

    uint32_t glfw_extension_count;
    const auto glfw_extensions =
        glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    const VkInstanceCreateInfo instance_create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = nullptr,
        .pApplicationInfo = &application_info,
        .enabledLayerCount = 0,
        .ppEnabledLayerNames = nullptr,
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

int main() {
    try {
        const auto instance = create_vulkan_instance();
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
