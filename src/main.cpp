#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "device.hpp"
#include "enumerate.hpp"
#include "errors.hpp"
#include "graphics.hpp"
#include "present.hpp"

int main(int p_argc, const char *const *const p_argv) try {
    bool enable_validation = false;

    for (auto arg = p_argv; arg < p_argv + p_argc; ++arg) {
        if (std::strcmp(*arg, "--validation") == 0) {
            enable_validation = true;
            std::cout << "[INFO]: Enabling validation layers.\n";
        }
    }

    if (!glfwInit()) {
        throw mv::glfw_init_failed_exception{};
    }

    const auto instance = mv::vulkan_instance_t::create(enable_validation);
    const auto window = mv::window_t::create(instance, "Hello!", 1280, 720);
    const auto device = mv::vulkan_device_t::create(instance, window.surface);
    const auto swapchain = mv::swapchain_t::create(device, window);
    const auto render_pass = mv::render_pass_t::create(device, swapchain);
    const auto framebuffers = swapchain.create_framebuffers(render_pass);
    const auto pipeline = mv::graphics_pipeline_t::create(
        device, render_pass, "shaders/basic.vert.spv", "shaders/basic.frag.spv"
    );

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
