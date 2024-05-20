#version 450

layout (location = 0) out vec4 frag_color;

layout (push_constant) uniform push_constants_t {
    float t;
} push_constants;

layout (binding = 1) uniform sampler2D texture_sampler;

layout (location = 0) in float x_pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 view_position;
layout (location = 3) in vec3 normal;

void main()
{
    vec4 color_bands = vec4(1.0, sin(( push_constants.t + x_pos ) * 10), 0.0, 1.0);
    vec4 texture_color = texture(texture_sampler, uv);

    frag_color = normalize(color_bands * texture_color);
}
