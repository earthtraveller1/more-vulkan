#version 450

#include "shadow.glsl"

layout (location = 0) out float depth;
layout (location = 0) in vec3 fragment_pos;

void main() {
    depth = length(fragment_pos - ubo.light_position);
}