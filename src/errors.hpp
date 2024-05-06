#pragma once

#include <vulkan/vulkan.h>


namespace mv {
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

struct no_adequate_swapchain_settings_exception : public std::exception {
    virtual const char *what() const noexcept override {
        return "There appears to be no adequate options for the swap chain.";
    }
};
}
