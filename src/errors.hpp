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

struct file_exception : public std::exception {
    enum class type_t {
        open,
        read,
    } type;

    std::string file_name;

    file_exception(type_t p_type, std::string_view p_file_name)
        : type(p_type), file_name(p_file_name.data()) {}

    virtual const char *what() const noexcept override {
        switch (type) {
        case type_t::open:
            return "Failed to open file.";
        case type_t::read:
            return "Failed to read file.";
        }
    }
};
} // namespace mv

template <typename T>
auto operator<<(
    std::basic_ostream<T> &p_ostream, const mv::file_exception::type_t &p_type
) -> std::basic_ostream<T> & {
    switch (p_type) {
    case mv::file_exception::type_t::open:
        return p_ostream << "open";
    case mv::file_exception::type_t::read:
        return p_ostream << "read";
    }
}
