#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;

layout (binding = 0) uniform uniform_buffer_object_t {
    mat4 projection;
    mat4 view;
    mat4 model;
} uniform_buffer_object;

layout (location = 0) out float x_pos;
layout (location = 1) out vec2 uv;
layout (location = 2) out vec4 view_position;

void main() {
    view_position = uniform_buffer_object.view * uniform_buffer_object.model * vec4(a_position, 1.0);
    gl_Position = uniform_buffer_object.projection * view_position;
    x_pos = a_position.x;
    uv = a_uv;
}
