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
    float distance = length(view_position.xyz);
    vec4 color_bands = vec4(1.0, sin(( push_constants.t + x_pos ) * 10), 0.0, 1.0);
    vec4 texture_color = texture(texture_sampler, uv);

    vec3 light_origin = vec3(0.0, 1.0, 0.0);
    vec3 light_direction = normalize(light_origin - view_position.xyz);
    float brightness = abs(dot(light_direction, normal));

    frag_color = texture_color / ( 10.0 * distance * distance + 100.0 ) + color_bands * ( brightness / 10.0);
    // frag_color = vec4(brightness, brightness, brightness, 1.0);
}
