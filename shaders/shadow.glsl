#ifndef INCLUDED_shadow_glsl
#define INCLUDED_shadow_glsl

layout (binding = 0) uniform uniform_buffer_object_t {
    mat4 projection;
    mat4 view;
    mat4 model;
    
    vec3 light_position;
} ubo;

#endif