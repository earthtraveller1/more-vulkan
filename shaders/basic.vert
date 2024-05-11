#version 450

layout (location = 0) in vec3 a_position;

layout (location = 0) out float x_pos;

void main() {
    gl_Position = vec4(a_position, 1.0);
    x_pos = a_position.x;
}
