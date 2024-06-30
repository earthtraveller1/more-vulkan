#ifndef INCLUDED_common_glsl
#define INCLUDED_common_glsl

layout (binding = 0) uniform ubo_t {
    mat4 projection;
    mat4 view;
    mat4 model;
    mat4 light_mat;
    vec3 light_position;
    vec3 global_light_direction;
    vec3 camera_position;
} ubo;

#endif

