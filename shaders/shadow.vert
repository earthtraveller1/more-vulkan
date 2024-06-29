#version 450

#include "shadow.glsl"

layout (location = 0) in vec3 a_position;

layout (location = 0) out vec3 fragment_pos;

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(a_position, 1.0);
    fragment_pos = (ubo.model * vec4(a_position, 1.0)).xyz;
}