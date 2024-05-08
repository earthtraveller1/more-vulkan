#include <fstream>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "common.hpp"
#include "device.hpp"
#include "enumerate.hpp"
#include "errors.hpp"
#include "present.hpp"

namespace {
struct graphics_pipeline_t {
    VkPipeline pipeline;
    const mv::vulkan_device_t &device;

    graphics_pipeline_t(
        VkPipeline p_pipeline, const mv::vulkan_device_t &p_device
    )
        : pipeline(p_pipeline), device(p_device) {}

    static auto create(
        const mv::vulkan_device_t &device, std::string_view vertex_shader_path,
        std::string_view fragment_shader_path
    ) -> graphics_pipeline_t;

    NO_COPY(graphics_pipeline_t);
    YES_MOVE(graphics_pipeline_t);

    ~graphics_pipeline_t() {
        vkDestroyPipeline(device.logical, pipeline, nullptr);
    }
};

struct vector_3f_t {
    float x;
    float y;
    float z;
};

struct vertex_t {
    vector_3f_t position;
};

constexpr VkVertexInputBindingDescription vertex_input_binding_description{
    .binding = 0,
    .stride = sizeof(vertex_t),
    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
};

constexpr static std::array<VkVertexInputAttributeDescription, 1>
    vertex_attribute_descriptions{VkVertexInputAttributeDescription{
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(vertex_t, position),
    }};

auto read_file_to_vector(std::string_view file_name) -> std::vector<char>;
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
        const auto window = mv::window_t::create(instance, "Hello!", 1280, 720);
        const auto device =
            mv::vulkan_device_t::create(instance, window.surface);
        const auto swapchain = mv::swapchain_t::create(device, window);

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

auto graphics_pipeline_t::create(
    const mv::vulkan_device_t &p_device, std::string_view p_vertex_shader_path,
    std::string_view p_fragment_shader_path
) -> graphics_pipeline_t {
    const auto vertex_shader_code = read_file_to_vector(p_vertex_shader_path);
    const auto fragment_shader_code =
        read_file_to_vector(p_fragment_shader_path);

    const VkShaderModuleCreateInfo vertex_shader_module_create_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vertex_shader_code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(vertex_shader_code.data()),
    };

    VkShaderModule vertex_shader_module;
    auto result = vkCreateShaderModule(
        p_device.logical, &vertex_shader_module_create_info, nullptr,
        &vertex_shader_module
    );

    if (result != VK_SUCCESS) {
        throw mv::vulkan_exception{result};
    }

    const VkShaderModuleCreateInfo fragment_shader_module_create_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = fragment_shader_code.size(),
        .pCode =
            reinterpret_cast<const uint32_t *>(fragment_shader_code.data()),
    };

    VkShaderModule fragment_shader_module;
    result = vkCreateShaderModule(
        p_device.logical, &fragment_shader_module_create_info, nullptr,
        &fragment_shader_module
    );

    if (result != VK_SUCCESS) {
        throw mv::vulkan_exception{result};
    }

    const std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertex_shader_module,
            .pName = "main",
        },
        VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragment_shader_module,
            .pName = "main",
        },
    };

    const VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertex_input_binding_description,
        .vertexAttributeDescriptionCount = vertex_attribute_descriptions.size(),
        .pVertexAttributeDescriptions = vertex_attribute_descriptions.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineViewportStateCreateInfo viewport_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const VkPipelineRasterizationStateCreateInfo rasterization_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    const VkPipelineColorBlendAttachmentState color_blend_attachment{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    const VkPipelineColorBlendStateCreateInfo color_blend_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    const VkGraphicsPipelineCreateInfo pipeline_create_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_state_create_info,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = nullptr,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = nullptr,
        .pColorBlendState = &color_blend_state,
    };

    VkPipeline pipeline;
    result = vkCreateGraphicsPipelines(
        p_device.logical, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr,
        &pipeline
    );

    if (result != VK_SUCCESS) {
        throw mv::vulkan_exception{result};
    }

    vkDestroyShaderModule(p_device.logical, vertex_shader_module, nullptr);
    vkDestroyShaderModule(p_device.logical, fragment_shader_module, nullptr);

    return {pipeline, p_device};
}

namespace {
auto read_file_to_vector(std::string_view file_name) -> std::vector<char> {
    std::ifstream file(file_name.data(), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw mv::file_exception{
            mv::file_exception::type_t::open,
            file_name.data(),
        };
    }

    const auto file_size = file.tellg();
    std::vector<char> buffer(file_size);
    file.seekg(0);
    file.read(buffer.data(), file_size);

    return buffer;
}
} // namespace
