#include <fstream>
#include <span>

#include <vulkan/vulkan_core.h>

#include "common.hpp"
#include "errors.hpp"

#include "graphics.hpp"

namespace {
auto read_file_to_vector(std::string_view file_name) -> std::vector<char>;
} // namespace

namespace mv {
auto graphics_pipeline_t::create(
    const mv::vulkan_device_t &p_device,
    const render_pass_t &p_render_pass,
    std::string_view p_vertex_shader_path,
    std::string_view p_fragment_shader_path,
    std::span<const VkPushConstantRange> push_constant_ranges,
    std::span<const VkDescriptorSetLayout> p_descriptor_set_layouts
) -> graphics_pipeline_t {
    const VkPipelineLayoutCreateInfo pipeline_layout_create_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount =
            static_cast<uint32_t>(p_descriptor_set_layouts.size()),
        .pSetLayouts = p_descriptor_set_layouts.data(),
        .pushConstantRangeCount =
            static_cast<uint32_t>(push_constant_ranges.size()),
        .pPushConstantRanges = push_constant_ranges.data(),
    };

    VkPipelineLayout pipeline_layout;
    auto result = vkCreatePipelineLayout(
        p_device.logical,
        &pipeline_layout_create_info,
        nullptr,
        &pipeline_layout
    );

    if (result != VK_SUCCESS) {
        throw mv::vulkan_exception{result};
    }

    const auto vertex_shader_code = read_file_to_vector(p_vertex_shader_path);
    const auto fragment_shader_code =
        read_file_to_vector(p_fragment_shader_path);

    const VkShaderModuleCreateInfo vertex_shader_module_create_info{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = vertex_shader_code.size(),
        .pCode = reinterpret_cast<const uint32_t *>(vertex_shader_code.data()),
    };

    VkShaderModule vertex_shader_module;
    result = vkCreateShaderModule(
        p_device.logical,
        &vertex_shader_module_create_info,
        nullptr,
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
        p_device.logical,
        &fragment_shader_module_create_info,
        nullptr,
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
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo multisample_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
    };

    const VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
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

    const std::array<VkDynamicState, 2> dynamic_states{
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
    };

    const VkPipelineDynamicStateCreateInfo dynamic_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamic_states.size(),
        .pDynamicStates = dynamic_states.data(),
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
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = pipeline_layout,
        .renderPass = p_render_pass.render_pass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

    VkPipeline pipeline;
    result = vkCreateGraphicsPipelines(
        p_device.logical,
        VK_NULL_HANDLE,
        1,
        &pipeline_create_info,
        nullptr,
        &pipeline
    );

    if (result != VK_SUCCESS) {
        throw mv::vulkan_exception{result};
    }

    vkDestroyShaderModule(p_device.logical, vertex_shader_module, nullptr);
    vkDestroyShaderModule(p_device.logical, fragment_shader_module, nullptr);

    return {pipeline, p_render_pass, pipeline_layout, p_device};
}

auto render_pass_t::create(
    const mv::vulkan_device_t &p_device,
    std::optional<VkFormat> color_format,
    std::optional<VkFormat> p_depth_format
) -> render_pass_t {
    const VkAttachmentReference color_attachment_reference{
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };

    const VkAttachmentReference depth_attachment_reference{
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    };

    std::vector<VkAttachmentDescription> attachments;

    VkSubpassDescription subpass{
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
    };

    if (color_format.has_value()) {
        const VkAttachmentDescription color_attachment{
            .format = color_format.value(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };

        attachments.push_back(color_attachment);

        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &color_attachment_reference;
    }

    if (p_depth_format.has_value()) {
        const VkAttachmentDescription depth_attachment{
            .format = p_depth_format.value(),
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        attachments.push_back(depth_attachment);
        subpass.pDepthStencilAttachment = &depth_attachment_reference;
    }

    const VkRenderPassCreateInfo render_pass_create_info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkRenderPass render_pass;
    const auto result = vkCreateRenderPass(
        p_device.logical, &render_pass_create_info, nullptr, &render_pass
    );
    if (result != VK_SUCCESS) {
        throw mv::vulkan_exception{result};
    }

    return {render_pass, p_device};
}

VkFramebuffer create_framebuffer(
    const vulkan_device_t &device,
    const vulkan_image_view_t &image_view,
    uint32_t width,
    uint32_t height,
    const render_pass_t &render_pass
) {
    const VkFramebufferCreateInfo framebuffer_info{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderPass = render_pass.render_pass,
        .attachmentCount = 1,
        .pAttachments = &image_view.image_view,
        .width = width,
        .height = height,
        .layers = 1
    };

    VkFramebuffer framebuffer;
    VK_ERROR(vkCreateFramebuffer(
        device.logical, &framebuffer_info, nullptr, &framebuffer
    ));

    return framebuffer;
}

auto descriptor_set_layout_t::create(
    const vulkan_device_t &p_device,
    std::span<const VkDescriptorSetLayoutBinding> bindings
) -> descriptor_set_layout_t {
    const VkDescriptorSetLayoutCreateInfo layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    VkDescriptorSetLayout layout;
    VK_ERROR(vkCreateDescriptorSetLayout(
        p_device.logical, &layout_info, nullptr, &layout
    ));
    return descriptor_set_layout_t{layout, p_device};
}

auto descriptor_pool_t::create(
    const vulkan_device_t &p_device,
    std::span<const VkDescriptorPoolSize> pool_sizes,
    uint32_t max_sets
) -> descriptor_pool_t {
    const VkDescriptorPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = max_sets,
        .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };

    VkDescriptorPool pool;
    VK_ERROR(
        vkCreateDescriptorPool(p_device.logical, &pool_info, nullptr, &pool)
    );
    return descriptor_pool_t{pool, p_device};
}

auto descriptor_pool_t::allocate_descriptor_set(
    const descriptor_set_layout_t &layout
) const -> VkDescriptorSet {
    VkDescriptorSetAllocateInfo alloc_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout.layout,
    };

    VkDescriptorSet set;
    VK_ERROR(vkAllocateDescriptorSets(device.logical, &alloc_info, &set));
    return set;
}

} // namespace mv

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
