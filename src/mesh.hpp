#pragma once

#include "graphics.hpp"

namespace mv {

enum class axis_t { x, y, z };

struct mesh_t {
    std::vector<vertex_t> vertices;
    std::vector<uint32_t> indices;

    mesh_t() = default;

    static auto create_cube(
        float p_id = 0.0f,
        float p_size = 1.0f,
        glm::vec3 p_position = glm::vec3(0.0f)
    ) -> mesh_t;

    auto append_cube(float p_id, float p_size, glm::vec3 p_position) -> void;

    auto append_cube_face(
        axis_t p_axis,
        bool p_negate,
        bool p_backface,
        float p_size = 1.0f,
        float p_id = 0.0f,
        glm::vec3 p_position = glm::vec3(0.0f),
        bool p_flip_uv = false
    ) -> void;
};

} // namespace mv