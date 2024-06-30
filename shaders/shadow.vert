#version 450

#include "shadow.glsl"

layout (location = 0) in vec3 a_position;

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(a_position, 1.0);
}
