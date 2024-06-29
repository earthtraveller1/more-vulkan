#version 450

#include "common.glsl"

#define LIGHT_ID 2
#define FLOOR_ID 3

#define M_PI 3.1415926535897932384626433832795

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
layout (location = 6) in vec3 fragment_position;

vec3 calculate_diffuse(vec3 normal, vec3 light_direction, vec3 light_color) {
    const float diffuse_factor = max(0.0, dot(normal, light_direction));
    return light_color * diffuse_factor;
}

vec3 calculate_specular(vec3 normal, vec3 light_direction, vec3 view_direction, vec3 light_color) {
    const vec3 halfway_direction = normalize(light_direction + view_direction);
    const float specular_factor = pow(max(0.0, dot(normal, halfway_direction)), 32.0);
    return light_color * specular_factor;
}

float calculate_attenuation(float distance, float c, float l, float q) {
    return 1.0 / (c + l*distance + q*distance*distance);
}

void main() {
    // For rendering the light
    if (id == LIGHT_ID) {
        frag_color = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    vec3 material_color;
    if (id == FLOOR_ID) {
        material_color = vec3(1.0, 1.0, 0.0);
    } else {
        material_color = texture(texture_samplers[id], uv).rgb;
    }

    const vec4 color_bands = vec4(1.0, sin(( push_constants.t + x_pos ) * 10), 0.0, 1.0);
    const vec3 light_color = vec3(1.0, 1.0, 1.0);

    const vec3 normalized_normal = normalize(normal);
    const vec3 light_direction = normalize(uniform_buffer_object.light_position - fragment_position);
    const vec3 diffuse = calculate_diffuse(normalized_normal, light_direction, light_color);

    const vec3 view_direction = normalize(uniform_buffer_object.camera_position - fragment_position);
    const vec3 specular = calculate_specular(normalized_normal, light_direction, view_direction, light_color);

    const float distance = length(uniform_buffer_object.light_position - fragment_position);
    const float attenuation = calculate_attenuation(distance, 1.0, 0.22, 0.20);

    const vec3 ambient = vec3(0.01, 0.01, 0.01);

    const float offset = dot(-uniform_buffer_object.global_light_direction, light_direction);
    const float cutoff = cos(radians(45.0));

    vec3 lighting;
    if (offset > cutoff) {
        lighting = (specular * attenuation + ambient + diffuse * attenuation);
    } else {
        lighting = ambient;
    }

    const vec3 color = material_color * lighting;
    frag_color = vec4(color, 1.0);
}
