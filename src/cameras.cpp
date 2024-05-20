
#include "cameras.hpp"

using mv::first_person_camera_t;

auto first_person_camera_t::update_vectors(float p_yaw, float p_pitch) -> void {
    glm::vec3 new_direction;
    new_direction.x = cos(glm::radians(p_yaw)) * cos(glm::radians(p_pitch));
    new_direction.y = sin(glm::radians(p_pitch));
    new_direction.z = sin(glm::radians(p_yaw)) * cos(glm::radians(p_pitch));
    direction = glm::normalize(new_direction);

    right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, direction));
}
