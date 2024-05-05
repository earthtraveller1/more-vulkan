#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <string_view>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "enumerate.hpp"
#include "device.hpp"
#include "errors.hpp"

namespace {

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

} // namespace

int main(int p_argc, const char *const *const p_argv) {
    bool enable_validation = false;

    for (auto arg = p_argv; arg < p_argv + p_argc; ++arg) {
        if (std::strcmp(*arg, "--validation") == 0) {
            enable_validation = true;
        }
    }

    if (!glfwInit()) {
        throw mv::glfw_init_failed_exception{};
    }

    try {
        const auto instance = mv::vulkan_instance_t::create(enable_validation);
        const auto window = window_t::create(instance, "Hello!", 1280, 720);
        const auto device = mv::vulkan_device_t::create(instance, window.surface);

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
}
