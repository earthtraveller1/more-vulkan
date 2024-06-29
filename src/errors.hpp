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

struct no_adequate_memory_type_exception : public std::exception {
    virtual const char *what() const noexcept override {
        return "Could not find adequate memory type.";
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

template <typename T>
auto operator<<(std::basic_ostream<T> &p_ostream, VkResult p_error_code)
    -> std::basic_ostream<T> & {
    switch (p_error_code) {
    case VK_SUCCESS:
        return p_ostream << "VK_SUCCESS";
    case VK_NOT_READY:
        return p_ostream << "VK_NOT_READY";
    case VK_TIMEOUT:
        return p_ostream << "VK_TIMEOUT";
    case VK_EVENT_SET:
        return p_ostream << "VK_EVENT_SET";
    case VK_EVENT_RESET:
        return p_ostream << "VK_EVENT_RESET";
    case VK_INCOMPLETE:
        return p_ostream << "VK_INCOMPLETE";
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        return p_ostream << "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        return p_ostream << "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED:
        return p_ostream << "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST:
        return p_ostream << "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED:
        return p_ostream << "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT:
        return p_ostream << "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT:
        return p_ostream << "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT:
        return p_ostream << "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER:
        return p_ostream << "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS:
        return p_ostream << "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED:
        return p_ostream << "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_FRAGMENTED_POOL:
        return p_ostream << "VK_ERROR_FRAGMENTED_POOL";
    case VK_ERROR_UNKNOWN:
        return p_ostream << "VK_ERROR_UNKNOWN";
    case VK_ERROR_OUT_OF_POOL_MEMORY:
        return p_ostream << "VK_ERROR_OUT_OF_POOL_MEMORY";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        return p_ostream << "VK_ERROR_INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FRAGMENTATION:
        return p_ostream << "VK_ERROR_FRAGMENTATION";
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        return p_ostream << "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
    case VK_PIPELINE_COMPILE_REQUIRED:
        return p_ostream << "VK_PIPELINE_COMPILE_REQUIRED";
    case VK_ERROR_SURFACE_LOST_KHR:
        return p_ostream << "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        return p_ostream << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
    case VK_SUBOPTIMAL_KHR:
        return p_ostream << "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR:
        return p_ostream << "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        return p_ostream << "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
    case VK_ERROR_VALIDATION_FAILED_EXT:
        return p_ostream << "VK_ERROR_VALIDATION_FAILED_EXT";
    case VK_ERROR_INVALID_SHADER_NV:
        return p_ostream << "VK_ERROR_INVALID_SHADER_NV";
    case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
        return p_ostream << "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
        return p_ostream << "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
        return p_ostream
               << "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
        return p_ostream << "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
        return p_ostream << "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
    case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
        return p_ostream << "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
    case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
        return p_ostream
               << "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
    case VK_ERROR_NOT_PERMITTED_KHR:
        return p_ostream << "VK_ERROR_NOT_PERMITTED_KHR";
    case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        return p_ostream << "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
    case VK_THREAD_IDLE_KHR:
        return p_ostream << "VK_THREAD_IDLE_KHR";
    case VK_THREAD_DONE_KHR:
        return p_ostream << "VK_THREAD_DONE_KHR";
    case VK_OPERATION_DEFERRED_KHR:
        return p_ostream << "VK_OPERATION_DEFERRED_KHR";
    case VK_OPERATION_NOT_DEFERRED_KHR:
        return p_ostream << "VK_OPERATION_NOT_DEFERRED_KHR";
    case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR:
        return p_ostream << "VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
    case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
        return p_ostream << "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
    case VK_INCOMPATIBLE_SHADER_BINARY_EXT:
        return p_ostream << "VK_INCOMPATIBLE_SHADER_BINARY_EXT";
    case VK_RESULT_MAX_ENUM:
        return p_ostream << "VK_RESULT_MAX_ENUM";
        default : return p_ostream << "Unknown error";
    }
}
