#version 450

layout (location = 0) out vec4 frag_color;

layout (push_constant) uniform push_constants_t {
    float t;
} push_constants;

layout (location = 0) in float x_pos;
layout (location = 1) in vec2 uv;

void main()
{
    frag_color = vec4(1.0, sin(( push_constants.t + x_pos ) * 10), 0.0, 1.0);
}
