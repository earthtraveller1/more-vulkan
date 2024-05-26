#version 450

#include "common.glsl"

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec2 a_uv;
layout (location = 2) in vec3 a_normal;
layout (location = 3) in float a_id;

layout (location = 0) out float x_pos;
layout (location = 1) out vec2 uv;
layout (location = 2) out vec4 view_position;
layout (location = 3) out vec3 normal;
layout (location = 4) flat out int id;
layout (location = 5) out vec3 light_position;
layout (location = 6) out vec3 fragment_position;

void main() {
    view_position = uniform_buffer_object.view * uniform_buffer_object.model * vec4(a_position, 1.0);
    gl_Position = uniform_buffer_object.projection * view_position;

    x_pos = a_position.x;
    uv = a_uv;
    normal = a_normal;
    id = int(a_id);
    light_position = uniform_buffer_object.light_position;
    fragment_position = (uniform_buffer_object.model * vec4(a_position, 1.0)).xyz;
}
