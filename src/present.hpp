#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace mv {
struct window_t {
    GLFWwindow *window;
    VkSurfaceKHR surface;
    VkInstance instance;

    static auto create(
        VkInstance p_instance, std::string_view p_title, int p_width,
        int p_height
    ) -> window_t;

    inline ~window_t() {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};
} // namespace mv