#include <GLFW/glfw3.h>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>

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
} // namespace

int main() {
    try {
        const auto window = create_window("Hello!", 1280, 720);

        glfwShowWindow(window);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
        }

        glfwDestroyWindow(window);
        glfwTerminate();
    } catch (const std::exception &e) {
        std::cerr << "[ERROR]: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
}
