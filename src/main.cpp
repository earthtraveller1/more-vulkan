#include "images.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "buffers.hpp"
#include "commands.hpp"
#include "device.hpp"
#include "enumerate.hpp"
#include "errors.hpp"
#include "graphics.hpp"
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
};

enum class axis_t { x, y, z };

auto append_cube_face_to_mesh(
    axis_t p_axis,
    bool p_negate,
    bool p_backface,
    float p_first_offset,
    float p_second_offset,
    std::vector<mv::vertex_t> &p_vertices,
    std::vector<uint32_t> &p_indices,
    bool p_flip_uv = false
) -> void {
    const float values[][2]{
        {0.5f + p_first_offset, -0.5f + p_second_offset},
        {0.5f + p_first_offset, 0.5f + p_second_offset},
        {-0.5f + p_first_offset, 0.5f + p_second_offset},
        {-0.5f + p_first_offset, -0.5f + p_second_offset},
    };

    const float uvs[][2]{
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},
    };

    const float flipped_uvs[][2]{
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},
    };

    const float third_value = p_negate ? -0.5f : 0.5f;

    const std::array indices{0, 1, 2, 0, 2, 3};

    const auto pivot_index = static_cast<uint16_t>(p_vertices.size());

    for (int i = 0; i < 4; i++) {
        float x_value, y_value, z_value;
        float normal_x = 0.0f, normal_y = 0.0f, normal_z = 0.0f;

        switch (p_axis) {
        case axis_t::x:
            z_value = -values[i][0];
            y_value = values[i][1];
            x_value = third_value;
            normal_x = 1.0f;

            if (p_backface) {
                y_value = -y_value;
            }

            break;
        case axis_t::y:
            x_value = values[i][0];
            z_value = -values[i][1];
            y_value = third_value;
            normal_y = 1.0f;

            if (p_backface) {
                z_value = -z_value;
            }

            break;
        case axis_t::z:
            x_value = values[i][0];
            y_value = values[i][1];
            z_value = third_value;
            normal_z = 1.0f;

            if (p_backface) {
                x_value = -x_value;
            }
        }

        if (p_flip_uv) {
            p_vertices.push_back({
                .position = glm::vec3{x_value, y_value, z_value},
                .uv = glm::vec2{flipped_uvs[i][0], flipped_uvs[i][1]},
                .normal = glm::vec3{normal_x, normal_y, normal_z},
            });
        } else {
            p_vertices.push_back({
                .position = glm::vec3{x_value, y_value, z_value},
                .uv = glm::vec2{uvs[i][0], uvs[i][1]},
                .normal = glm::vec3{normal_x, normal_y, normal_z},
            });
        }
    }

    for (auto index : indices) {
        p_indices.push_back(pivot_index + index);
    }
}

} // namespace

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
    const auto device = mv::vulkan_device_t::create(instance, window.surface);
    auto swapchain = mv::swapchain_t::create(device, window);

    const auto command_pool = mv::command_pool_t::create(device);

    auto depth_buffer = mv::vulkan_texture_t::create_depth_attachment(
        device, swapchain.extent.width, swapchain.extent.height
    );

    depth_buffer.transition_layout(
        command_pool, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );

    const auto render_pass =
        mv::render_pass_t::create(device, swapchain, depth_buffer.format);
    auto framebuffers =
        swapchain.create_framebuffers(render_pass, depth_buffer);

    const auto descriptor_set_layout = mv::descriptor_set_layout_t::create(
        device,
        std::array{
            mv::uniform_buffer_t::get_set_layout_binding(
                0, 1, VK_SHADER_STAGE_VERTEX_BIT
            ),
            mv::vulkan_texture_t::get_set_layout_binding(
                1, 1, VK_SHADER_STAGE_FRAGMENT_BIT
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

    const auto command_buffer = command_pool.allocate_buffer();

    std::vector<mv::vertex_t> vertices;
    std::vector<uint32_t> indices;

    append_cube_face_to_mesh(
        axis_t::y, false, true, 0.0f, 0.0f, vertices, indices
    );
    append_cube_face_to_mesh(
        axis_t::z, true, false, 0.0f, 0.0f, vertices, indices
    );

    auto vertex_buffer = mv::vertex_buffer_t::create(
        device, vertices.size() * sizeof(mv::vertex_t)
    );

    vertex_buffer.buffer.load_using_staging(
        command_pool.pool,
        vertices.data(),
        vertices.size() * sizeof(mv::vertex_t)
    );

    auto index_buffer =
        mv::index_buffer_t::create(device, indices.size() * sizeof(uint32_t));
    index_buffer.buffer.load_using_staging(
        command_pool.pool, indices.data(), indices.size() * sizeof(uint32_t)
    );

    const auto uniform_buffer =
        mv::uniform_buffer_t::create(device, sizeof(uniform_buffer_object_t));

    const auto frame_fence = vulkan_fence_t::create(device);
    const auto image_available_semaphore = vulkan_semaphore_t::create(device);
    const auto render_done_semaphore = vulkan_semaphore_t::create(device);

    const auto descriptor_pool = mv::descriptor_pool_t::create(
        device,
        std::array{
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
            },
            VkDescriptorPoolSize{
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
            }
        }
    );

    const auto texture = mv::vulkan_texture_t::load_from_file(
        device, command_pool, "textures/can-pooper.png"
    );

    const auto texture_sampler = texture.create_sampler();

    const auto descriptor_set =
        descriptor_pool.allocate_descriptor_set(descriptor_set_layout);

    {
        const auto buffer_info = uniform_buffer.get_descriptor_buffer_info();
        const auto image_info =
            texture.get_descriptor_image_info(texture_sampler.sampler);

        const std::array set_writes{
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_set,
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .pBufferInfo = &buffer_info,
            },
            VkWriteDescriptorSet{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = descriptor_set,
                .dstBinding = 1,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &image_info,
            }
        };

        vkUpdateDescriptorSets(
            device.logical, set_writes.size(), set_writes.data(), 0, nullptr
        );
    }

    uniform_buffer_object_t ubo{
        .projection = glm::perspective(
            45.0f,
            static_cast<float>(window.width) /
                static_cast<float>(window.height),
            0.1f,
            100.0f
        ),
        .view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -4.0f)),
        .model = glm::mat4(1.0f)
    };

    double delta_time = 0.0;
    double time = 0.0;

    glfwShowWindow(window.window);
    while (!glfwWindowShouldClose(window.window)) {
        const auto start_time = glfwGetTime();

        const double speed = 1.0;
        const auto bob_rate = window.height * 0.00000025;
        const auto bob_factor = time * 15.0;
        if (glfwGetKey(window.window, GLFW_KEY_W)) {
            ubo.view = glm::translate(
                ubo.view,
                glm::vec3(
                    0.0, std::cos(bob_factor) * bob_rate, speed * delta_time
                )
            );

            time += delta_time;
        }
        if (glfwGetKey(window.window, GLFW_KEY_S)) {
            ubo.view = glm::translate(
                ubo.view,
                glm::vec3(
                    0.0, std::cos(bob_factor) * bob_rate, -speed * delta_time
                )
            );

            time += delta_time;
        }
        if (glfwGetKey(window.window, GLFW_KEY_A)) {
            ubo.view = glm::translate(
                ubo.view,
                glm::vec3(
                    speed * delta_time, std::cos(bob_factor) * bob_rate, 0.0
                )
            );

            time += delta_time;
        }
        if (glfwGetKey(window.window, GLFW_KEY_D)) {
            ubo.view = glm::translate(
                ubo.view,
                glm::vec3(
                    -speed * delta_time, std::cos(bob_factor) * bob_rate, 0.0
                )
            );

            time += delta_time;
        }

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
            // Make sure the device is done doing shit before we try to
            // destroy the swapchain
            vkDeviceWaitIdle(device.logical);

            framebuffers.fucking_destroy();
            swapchain.fucking_destroy();
            depth_buffer = mv::vulkan_texture_t{};

            swapchain = mv::swapchain_t::create(device, window);
            depth_buffer = mv::vulkan_texture_t::create_depth_attachment(
                device, swapchain.extent.width, swapchain.extent.height
            );
            depth_buffer.transition_layout(
                command_pool, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            );
            framebuffers =
                swapchain.create_framebuffers(render_pass, depth_buffer);

            continue;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw mv::vulkan_exception{result};
        }

        vkResetFences(device.logical, 1, &frame_fence.fence);

        vkResetCommandBuffer(command_buffer, 0);

        const VkCommandBufferBeginInfo begin_info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        };

        VK_ERROR(vkBeginCommandBuffer(command_buffer, &begin_info));

        const VkClearValue clear_color{
            .color = {.float32 = {0.0f, 0.00f, 0.00f, 1.0f}},
        };

        const VkClearValue clear_depth{
            .depthStencil = {.depth = 1.0f, .stencil = 0},
        };

        const VkClearValue clear_values[]{clear_color, clear_depth};

        const VkRenderPassBeginInfo render_pass_begin_info{
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
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

        const auto data = uniform_buffer.map_memory();
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

        const VkDeviceSize offset = 0;
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
        vkCmdDrawIndexed(command_buffer, indices.size(), 1, 0, 0, 1);

        vkCmdEndRenderPass(command_buffer);
        VK_ERROR(vkEndCommandBuffer(command_buffer));

        const VkPipelineStageFlags wait_stage =
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        const VkSubmitInfo submit_info{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
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
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_done_semaphore.semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain.swapchain,
            .pImageIndices = &image_index,
        };

        result = vkQueuePresentKHR(device.present_queue, &present_info);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
            // Make sure the device is done doing shit before we try to
            // destroy the swapchain
            vkDeviceWaitIdle(device.logical);

            swapchain = mv::swapchain_t{};
            depth_buffer = mv::vulkan_texture_t{};
            framebuffers = mv::swapchain_t::framebuffers_t{};

            swapchain = mv::swapchain_t::create(device, window);
            depth_buffer = mv::vulkan_texture_t::create_depth_attachment(
                device, swapchain.extent.width, swapchain.extent.height
            );
            depth_buffer.transition_layout(
                command_pool, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
            );
            framebuffers =
                swapchain.create_framebuffers(render_pass, depth_buffer);
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
