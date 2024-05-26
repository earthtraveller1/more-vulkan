#version 450

#include "common.glsl"

layout (location = 0) out vec4 frag_color;

layout (push_constant) uniform push_constants_t {
    float t;
} push_constants;

layout (binding = 1) uniform sampler2D texture_samplers[2];

layout (location = 0) in float x_pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 view_position;
layout (location = 3) in vec3 normal;
layout (location = 4) flat in int id;
layout (location = 5) in vec3 light_position;
layout (location = 6) in vec3 fragment_position;

void main()
{
    if (id == 2) {
        frag_color = vec4(1.0, 1.0, 1.0, 1.0);
    } else {
        const vec4 color_bands = vec4(1.0, sin(( push_constants.t + x_pos ) * 10), 0.0, 1.0);
        const vec4 texture_color = texture(texture_samplers[id], uv);

        const vec3 normalize_normal = normalize(normal);
        const vec3 light_direction = normalize(light_position - fragment_position);
        const float diffuse_factor = max(0.0, dot(normalize_normal, light_direction));
        const vec3 light_color = vec3(1.0, 1.0, 1.0);
        const vec3 diffuse = light_color * diffuse_factor;

        const vec3 ambient = vec3(0.01, 0.01, 0.01);

        const vec3 color = (diffuse + ambient) * texture_color.rgb;
        frag_color = vec4(color, 1.0);
        // frag_color = vec4(normalize_normal, 1.0);
    }
}
