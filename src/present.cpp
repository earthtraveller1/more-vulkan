#include "errors.hpp"

#include "present.hpp"

using mv::window_t;

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

    return {
        .window = window,
        .surface = surface,
        .instance = p_instance,
    };
}
