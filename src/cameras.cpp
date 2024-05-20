
#include "cameras.hpp"

using mv::first_person_camera_t;

auto first_person_camera_t::update_vectors() -> void {
    glm::vec3 new_direction;
    new_direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    new_direction.y = sin(glm::radians(pitch));
    new_direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction = glm::normalize(new_direction);

    right = glm::normalize(glm::cross(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
    up = glm::normalize(glm::cross(right, direction));
}
