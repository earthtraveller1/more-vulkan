#version 450

layout (location = 0) in vec3 a_position;

layout (binding = 0) uniform uniform_buffer_object_t {
    mat4 projection;
    mat4 view;
    mat4 model;
} uniform_buffer_object;

layout (location = 0) out float x_pos;

void main() {
    gl_Position = uniform_buffer_object.projection * uniform_buffer_object.view * uniform_buffer_object.model * vec4(a_position, 1.0);
    x_pos = a_position.x;
}
