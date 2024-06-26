#include "images.hpp"
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "buffers.hpp"
#include "cameras.hpp"
#include "commands.hpp"
#include "device.hpp"
#include "enumerate.hpp"
#include "errors.hpp"
#include "graphics.hpp"
#include "memory.hpp"
#include "mesh.hpp"
#include "present.hpp"
#include "sync.hpp"

using mv::vulkan_fence_t;
using mv::vulkan_semaphore_t;

namespace {

struct push_constants_t {
    float t;
};

struct uniform_buffer_object_t {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
    glm::mat4 light_mat;
    alignas(16) glm::vec3 light_position;
    alignas(16) glm::vec3 global_light_direction;
    alignas(16) glm::vec3 camera_position;
};

struct shadow_uniform_buffer_object_t {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 model;
    alignas(16) glm::vec3 light_position;
};

auto recreate_swapchain(
    const mv::window_t &window,
    const mv::vulkan_device_t &device,
    const mv::command_pool_t &command_pool,
    const mv::render_pass_t &render_pass,
    mv::swapchain_t &swapchain,
    mv::swapchain_t::framebuffers_t &framebuffers,
    mv::vulkan_image_t &depth_buffer,
    mv::vulkan_image_view_t &depth_buffer_view,
    mv::vulkan_memory_t &depth_buffer_memory
) -> void {
    swapchain = mv::swapchain_t{};
    depth_buffer_view = mv::vulkan_image_view_t{};
    depth_buffer = mv::vulkan_image_t{};
    framebuffers = mv::swapchain_t::framebuffers_t{};

    swapchain = mv::swapchain_t::create(device, window);
    depth_buffer = mv::vulkan_image_t::create_depth_attachment(
        device, swapchain.extent.width, swapchain.extent.height
    );
    const auto depth_buffer_memory_requirements =
        depth_buffer.get_memory_requirements();
    depth_buffer_memory = mv::vulkan_memory_t::allocate(
        device,
        std::array{depth_buffer_memory_requirements},
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );
    depth_buffer_memory.bind_offset = 0;
    depth_buffer_memory.bind_image(
        depth_buffer, depth_buffer_memory_requirements
    );
    depth_buffer_view = mv::vulkan_image_view_t::create(
        depth_buffer, VK_IMAGE_ASPECT_DEPTH_BIT
    );
    depth_buffer.transition_layout(
        command_pool, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );
    framebuffers =
        swapchain.create_framebuffers(render_pass, depth_buffer_view);
}
} // namespace
//

#define SHADOW_SIZE 4096

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
    auto window = mv::window_t::create(instance, "Hello!", 1280, 720);
    window.set_user_pointer();
    glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    const auto device = mv::vulkan_device_t::create(instance, window.surface);
    auto swapchain = mv::swapchain_t::create(device, window);
    const auto command_pool = mv::command_pool_t::create(device);
    auto depth_buffer = mv::vulkan_image_t::create_depth_attachment(
        device, swapchain.extent.width, swapchain.extent.height
    );

    auto depth_buffer_memory_requirements =
        depth_buffer.get_memory_requirements();

    auto depth_buffer_memory = mv::vulkan_memory_t::allocate(
        device,
        std::array{depth_buffer_memory_requirements},
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    depth_buffer_memory.bind_image(
        depth_buffer, depth_buffer_memory_requirements
    );

    auto depth_buffer_view = mv::vulkan_image_view_t::create(
        depth_buffer, VK_IMAGE_ASPECT_DEPTH_BIT
    );

    depth_buffer.transition_layout(
        command_pool, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    const auto texture_image =
        mv::image_t::load_from_file("textures/can-pooper.png", 4);
    auto texture = mv::vulkan_image_t::create(
        device,
        texture_image.width,
        texture_image.height,
        VK_FORMAT_R8G8B8A8_SRGB
    );
    const auto texture_memory_requirements = texture.get_memory_requirements();

    const auto another_texture_image =
        mv::image_t::load_from_file("textures/neng-face.jpg", 4);
    auto another_texture = mv::vulkan_image_t::create(
        device,
        another_texture_image.width,
        another_texture_image.height,
        VK_FORMAT_R8G8B8A8_SRGB
    );
    const auto another_texture_memory_requirements =
        another_texture.get_memory_requirements();

    auto shadow_depth_buffer =
        mv::vulkan_image_t::create_depth_attachment(device, SHADOW_SIZE, SHADOW_SIZE, true);
    const auto shadow_depth_buffer_memory_requirements =
        shadow_depth_buffer.get_memory_requirements();

    const std::array memory_requirements{
        texture_memory_requirements,
        another_texture_memory_requirements,
        shadow_depth_buffer_memory_requirements,
    };

    auto image_memory = mv::vulkan_memory_t::allocate(
        device, memory_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    image_memory.bind_image(texture, texture_memory_requirements);
    image_memory.bind_image(
        another_texture, another_texture_memory_requirements
    );
    image_memory.bind_image(
        shadow_depth_buffer, shadow_depth_buffer_memory_requirements
    );

    texture.load_from_image(command_pool, texture_image);
    another_texture.load_from_image(command_pool, another_texture_image);
    const auto texture_view =
        mv::vulkan_image_view_t::create(texture, VK_IMAGE_ASPECT_COLOR_BIT);

    const auto another_texture_view = mv::vulkan_image_view_t::create(
        another_texture, VK_IMAGE_ASPECT_COLOR_BIT
    );

    const auto shadow_depth_buffer_view = mv::vulkan_image_view_t::create(
        shadow_depth_buffer, VK_IMAGE_ASPECT_DEPTH_BIT
    );

    const auto shadow_sampler = shadow_depth_buffer.create_sampler(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

    const auto render_pass = mv::render_pass_t::create(
        device,
        swapchain.format,
        depth_buffer.format,
        std::array{VkSubpassDependency{
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dependencyFlags = 0,
        }}
    );
    auto framebuffers =
        swapchain.create_framebuffers(render_pass, depth_buffer_view);

    const auto shadow_render_pass = mv::render_pass_t::create(
        device,
        {},
        shadow_depth_buffer.format,
        std::array{
            VkSubpassDependency{
                .srcSubpass = VK_SUBPASS_EXTERNAL,
                .dstSubpass = 0,
                .srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            },
            VkSubpassDependency{
                .srcSubpass = 0,
                .dstSubpass = VK_SUBPASS_EXTERNAL,
                .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
                .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
            },
        },
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    );

    const auto shadow_framebuffer = mv::create_framebuffer(
        device, shadow_depth_buffer_view, SHADOW_SIZE, SHADOW_SIZE, shadow_render_pass
    );

    const auto descriptor_set_layout = mv::descriptor_set_layout_t::create(
        device,
        std::array{
            mv::uniform_buffer_t::get_set_layout_binding(
                0, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
            ),
            mv::vulkan_image_t::get_set_layout_binding(
                1, 2, VK_SHADER_STAGE_FRAGMENT_BIT
            ),
            mv::vulkan_image_t::get_set_layout_binding(
                2, 1, VK_SHADER_STAGE_FRAGMENT_BIT
            ),
        }
    );

    const auto shadow_descriptor_set_layout =
        mv::descriptor_set_layout_t::create(
            device,
            std::array{
                mv::uniform_buffer_t::get_set_layout_binding(
                    0,
                    1,
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
                ),
            }
        );

    const auto pipeline = mv::graphics_pipeline_t::create(
        device,
        render_pass,
        "shaders/basic.vert.spv",
        "shaders/basic.frag.spv",
        std::array<VkPushConstantRange, 1>{VkPushConstantRange{
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            .offset = 0,
            .size = sizeof(push_constants_t),
        }},
        std::array<VkDescriptorSetLayout, 1>{
            descriptor_set_layout.layout,
        }
    );

    const auto shadow_pipeline = mv::graphics_pipeline_t::create(
        device,
        shadow_render_pass,
        "shaders/shadow.vert.spv",
        "shaders/shadow.frag.spv",
        std::array<VkPushConstantRange, 0>{},
        std::array{shadow_descriptor_set_layout.layout}
    );

    const auto command_buffer = command_pool.allocate_buffer();

    const glm::vec3 light_position{1.5f, -1.7f, -1.8f};

    auto cube = mv::mesh_t::create_cube(0.0f, 1.0, glm::vec3(0.0f, 0.0f, 2.0f));
    cube.append_cube(1.0f, 1.0, glm::vec3(0.0f, 2.0f, 1.0f));
    cube.append_cube(2.0f, 0.5f, light_position);
    cube.append_cube(3.0f, 100.0f, glm::vec3(0.0f, 53.0f, 0.0f));

    auto vertex_buffer = mv::vertex_buffer_t::create(
        device, cube.vertices.size() * sizeof(mv::vertex_t)
    );

    vertex_buffer.buffer.load_using_staging(
        command_pool.pool,
        cube.vertices.data(),
        cube.vertices.size() * sizeof(mv::vertex_t)
    );

    auto index_buffer = mv::index_buffer_t::create(
        device, cube.indices.size() * sizeof(uint32_t)
    );
    index_buffer.buffer.load_using_staging(
        command_pool.pool,
        cube.indices.data(),
        cube.indices.size() * sizeof(uint32_t)
    );

    const auto uniform_buffer =
        mv::uniform_buffer_t::create(device, sizeof(uniform_buffer_object_t));

    const auto shadow_uniform_buffer = mv::uniform_buffer_t::create(
        device, sizeof(shadow_uniform_buffer_object_t)
    );

    const auto frame_fence = vulkan_fence_t::create(device);
    const auto image_available_semaphore = vulkan_semaphore_t::create(device);
    const auto shadow_done_semaphore = vulkan_semaphore_t::create(device);
    const auto shadow_texture_layout_done_semaphore =
        vulkan_semaphore_t::create(device);
    const auto render_done_semaphore = vulkan_semaphore_t::create(device);

    const auto descriptor_pool = mv::descriptor_pool_t::create(
        device,
        std::array{
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
            },
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
            },
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 2,
            }
        },
        2
    );

    const auto texture_sampler = texture.create_sampler();

    const auto descriptor_set =
        descriptor_pool.allocate_descriptor_set(descriptor_set_layout);

    {
        const auto buffer_info = uniform_buffer.get_descriptor_buffer_info();
        const auto image_info = texture.get_descriptor_image_info(
            texture_sampler.sampler, texture_view.image_view
        );
        const auto image2_info = another_texture.get_descriptor_image_info(
            texture_sampler.sampler, another_texture_view.image_view
        );

        const VkDescriptorImageInfo shadow_info{
            .sampler = shadow_sampler.sampler,
            .imageView = shadow_depth_buffer_view.image_view,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        };

        const std::array set_writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = descriptor_set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &buffer_info,
                .pTexelBufferView = nullptr,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = descriptor_set,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = descriptor_set,
                .dstBinding = 1,
                .dstArrayElement = 1,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image2_info,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = descriptor_set,
                .dstBinding = 2,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &shadow_info,
                .pBufferInfo = nullptr,
                .pTexelBufferView = nullptr,
            }
        };

        vkUpdateDescriptorSets(
            device.logical, set_writes.size(), set_writes.data(), 0, nullptr
        );
    }

    const auto shadow_descriptor_set =
        descriptor_pool.allocate_descriptor_set(shadow_descriptor_set_layout);

    {
        const auto buffer_info =
            shadow_uniform_buffer.get_descriptor_buffer_info();

        const std::array set_writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .pNext = nullptr,
                .dstSet = shadow_descriptor_set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pImageInfo = nullptr,
                .pBufferInfo = &buffer_info,
                .pTexelBufferView = nullptr,
            },
        };

        vkUpdateDescriptorSets(
            device.logical, set_writes.size(), set_writes.data(), 0, nullptr
        );
    }

    const auto light_direction = glm::normalize(glm::vec3(-1.7f, 2.0f, 3.0f));

    const auto light_right =
        glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), light_direction)
        );

    mv::first_person_camera_t light_camera{
        light_position,
        light_direction,
        glm::cross(light_direction, light_right),
        light_right
    };

    shadow_uniform_buffer_object_t shadow_ubo{
        .projection = glm::perspective(45.0f, 1.0f, 0.1f, 100.0f),
        .view = light_camera.look_at(),
        .model = glm::mat4(1.0f),
        .light_position = light_position,
    };

    uniform_buffer_object_t ubo{
        .projection = glm::perspective(
            45.0f,
            static_cast<float>(window.width) /
                static_cast<float>(window.height),
            0.1f,
            100.0f
        ),
        .view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 4.0f)),
        .model = glm::mat4(1.0f),
        .light_mat = shadow_ubo.projection * shadow_ubo.view,
        .light_position = light_position,
        .global_light_direction = light_direction,
        .camera_position = glm::vec3(0.0f)
    };

    mv::first_person_camera_t camera{
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, -1.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    };

    double delta_time = 0.0;
    double time = 0.0;

    double previous_mouse_x = 0.0;
    double previous_mouse_y = 0.0;
    bool has_mouse_set = false;
    bool should_follow_mouse = true;

    glfwShowWindow(window.window);
    while (!glfwWindowShouldClose(window.window)) {
        const auto start_time = glfwGetTime();

        const float speed = 1.0;
        const auto bob_rate = window.height * 0.00000025;
        const auto bob_factor = time * 15.0;
        auto should_update_time = false;
        const auto move_direction = camera.direction;
        if (glfwGetKey(window.window, GLFW_KEY_W)) {
            camera.position += (float)delta_time * speed * move_direction;

            should_update_time = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_S)) {
            camera.position -= (float)delta_time * speed * move_direction;

            should_update_time = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_A)) {
            camera.position -= (float)delta_time * speed * camera.right;

            should_update_time = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_D)) {
            camera.position += (float)delta_time * speed * camera.right;

            should_update_time = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_ESCAPE)) {
            glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            should_follow_mouse = false;
        }
        if (glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_LEFT)) {
            glfwSetInputMode(window.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            should_follow_mouse = true;
            has_mouse_set = false;
        }

        if (!has_mouse_set) {
            glfwGetCursorPos(
                window.window, &previous_mouse_x, &previous_mouse_y
            );
            has_mouse_set = true;
        }

        if (should_follow_mouse) {
            double mouse_x, mouse_y;
            const auto mouse_sensitivity = 0.1;
            glfwGetCursorPos(window.window, &mouse_x, &mouse_y);
            camera.yaw += (mouse_x - previous_mouse_x) * mouse_sensitivity;
            camera.pitch -= (previous_mouse_y - mouse_y) * mouse_sensitivity;

            if (camera.pitch > 89.0f) {
                camera.pitch = 89.0f;
            }
            if (camera.pitch < -89.0f) {
                camera.pitch = -89.0f;
            }

            camera.update_vectors();
            previous_mouse_x = mouse_x;
            previous_mouse_y = mouse_y;
        }

        ubo.camera_position = camera.position;

        if (should_update_time) {
            time += delta_time;
            camera.position.y += std::cos(bob_factor) * bob_rate;
        }

        ubo.view = camera.look_at();

        ubo.projection = glm::perspective(
            45.0f,
            static_cast<float>(window.width) /
                static_cast<float>(window.height),
            0.1f,
            100.0f
        );

        vkWaitForFences(
            device.logical, 1, &frame_fence.fence, VK_TRUE, UINT64_MAX
        );

        uint32_t image_index;
        auto result = vkAcquireNextImageKHR(
            device.logical,
            swapchain.swapchain,
            UINT64_MAX,
            image_available_semaphore.semaphore,
            VK_NULL_HANDLE,
            &image_index
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            std::cout << "out of date frame.\n";
            std::cout.flush();
            // Make sure the device is done doing shit before we try to
            // destroy the swapchain
            vkDeviceWaitIdle(device.logical);
            recreate_swapchain(
                window,
                device,
                command_pool,
                render_pass,
                swapchain,
                framebuffers,
                depth_buffer,
                depth_buffer_view,
                depth_buffer_memory
            );

            continue;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw mv::vulkan_exception{result};
        }

        vkResetFences(device.logical, 1, &frame_fence.fence);

        vkResetCommandBuffer(command_buffer, 0);

        const VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = 0,
            .pInheritanceInfo = nullptr,
        };

        VK_ERROR(vkBeginCommandBuffer(command_buffer, &begin_info));

        const VkClearValue clear_color{
            .color = {.float32 = {0.0f, 0.00f, 0.00f, 1.0f}},
        };

        const VkClearValue clear_depth{
            .depthStencil = {.depth = 1.0f, .stencil = 0},
        };

        const VkClearValue clear_values[]{clear_color, clear_depth};

        const VkRenderPassBeginInfo shadow_render_pass_begin_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = shadow_render_pass.render_pass,
            .framebuffer = shadow_framebuffer.framebuffer,
            .renderArea =
                {
                    .offset = {0, 0},
                    .extent =
                        {
                            .width = SHADOW_SIZE,
                            .height = SHADOW_SIZE,
                        },
                },
            .clearValueCount = 1,
            .pClearValues = &clear_values[1],
        };

        vkCmdBeginRenderPass(
            command_buffer,
            &shadow_render_pass_begin_info,
            VK_SUBPASS_CONTENTS_INLINE
        );

        auto data = shadow_uniform_buffer.map_memory();
        memcpy(data, &shadow_ubo, sizeof(shadow_ubo));
        shadow_uniform_buffer.unmap_memory();

        vkCmdBindPipeline(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadow_pipeline.pipeline
        );

        vkCmdBindDescriptorSets(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            shadow_pipeline.layout,
            0,
            1,
            &shadow_descriptor_set,
            0,
            nullptr
        );

        const VkViewport shadow_viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = SHADOW_SIZE,
            .height = SHADOW_SIZE,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        const VkRect2D shadow_scissor{
            .offset = {0, 0},
            .extent = {.width = SHADOW_SIZE, .height = SHADOW_SIZE},
        };

        vkCmdSetViewport(command_buffer, 0, 1, &shadow_viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &shadow_scissor);

        const VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(
            command_buffer, 0, 1, &vertex_buffer.buffer.buffer, &offset
        );

        vkCmdBindIndexBuffer(
            command_buffer, index_buffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT32
        );

        vkCmdDrawIndexed(command_buffer, cube.indices.size(), 1, 0, 0, 1);

        vkCmdEndRenderPass(command_buffer);

        const VkRenderPassBeginInfo render_pass_begin_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .pNext = nullptr,
            .renderPass = render_pass.render_pass,
            .framebuffer = framebuffers.framebuffers.at(image_index),
            .renderArea =
                {
                    .offset = {0, 0},
                    .extent = swapchain.extent,
                },
            .clearValueCount = 2,
            .pClearValues = clear_values,
        };

        vkCmdBeginRenderPass(
            command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE
        );

        vkCmdBindPipeline(
            command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline
        );

        data = uniform_buffer.map_memory();
        memcpy(data, &ubo, sizeof ubo);
        uniform_buffer.unmap_memory();

        vkCmdBindDescriptorSets(
            command_buffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.layout,
            0,
            1,
            &descriptor_set,
            0,
            nullptr
        );

        const VkViewport viewport{
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(swapchain.extent.width),
            .height = static_cast<float>(swapchain.extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };

        const VkRect2D scissor{
            .offset = {0, 0},
            .extent = swapchain.extent,
        };

        vkCmdSetViewport(command_buffer, 0, 1, &viewport);
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        vkCmdBindVertexBuffers(
            command_buffer, 0, 1, &vertex_buffer.buffer.buffer, &offset
        );

        vkCmdBindIndexBuffer(
            command_buffer, index_buffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT32
        );

        const push_constants_t push_constants{
            .t = static_cast<float>(glfwGetTime()),
        };

        vkCmdPushConstants(
            command_buffer,
            pipeline.layout,
            VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(push_constants_t),
            &push_constants
        );

        // vkCmdDraw(command_buffer, 3, 1, 0, 0);
        vkCmdDrawIndexed(command_buffer, cube.indices.size(), 1, 0, 0, 1);

        vkCmdEndRenderPass(command_buffer);
        VK_ERROR(vkEndCommandBuffer(command_buffer));

        const VkPipelineStageFlags wait_stage =
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &image_available_semaphore.semaphore,
            .pWaitDstStageMask = &wait_stage,
            .commandBufferCount = 1,
            .pCommandBuffers = &command_buffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &render_done_semaphore.semaphore,
        };

        VK_ERROR(vkQueueSubmit(
            device.graphics_queue, 1, &submit_info, frame_fence.fence
        ));

        const VkPresentInfoKHR present_info{
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .pNext = nullptr,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_done_semaphore.semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain.swapchain,
            .pImageIndices = &image_index,
            .pResults = nullptr,
        };

        result = vkQueuePresentKHR(device.present_queue, &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            std::cout << "second out of date frame.\n";
            std::cout.flush();
            // Make sure the device is done doing shit before we try to
            // destroy the swapchain
            vkDeviceWaitIdle(device.logical);

            recreate_swapchain(
                window,
                device,
                command_pool,
                render_pass,
                swapchain,
                framebuffers,
                depth_buffer,
                depth_buffer_view,
                depth_buffer_memory
            );
        } else if (result != VK_SUCCESS) {
            throw mv::vulkan_exception{result};
        }

        glfwPollEvents();

        const auto end_time = glfwGetTime();
        delta_time = end_time - start_time;
    }

    vkDeviceWaitIdle(device.logical);
} catch (const mv::vulkan_exception &e) {
    std::cerr << "[ERROR]: Vulkan error " << e.error_code << '\n';
    return EXIT_FAILURE;
} catch (const mv::file_exception &e) {
    std::cerr << "[ERROR]: Failed to " << e.type << " " << e.file_name << '\n';
} catch (const std::exception &e) {
    std::cerr << "[ERROR]: Failed to " << e.what() << '\n';
    return EXIT_FAILURE;
}
