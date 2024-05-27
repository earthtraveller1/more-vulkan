#pragma once

namespace mv {
struct first_person_camera_t {
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 up;
    glm::vec3 right;

    double yaw = 0.0;
    double pitch = 0.0;

    first_person_camera_t(
        glm::vec3 position, glm::vec3 direction, glm::vec3 up, glm::vec3 right
    )
        : position(position), direction(direction), up(up), right(right) {}

    inline auto look_at() -> glm::mat4 const {
        return glm::lookAt(position, position + direction, up);
    }

    auto update_vectors() -> void;
};
} // namespace mv
