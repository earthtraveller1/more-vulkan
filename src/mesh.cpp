#include "mesh.hpp"

auto mv::mesh_t::create_cube(float p_id, float p_size, glm::vec3 p_position)
    -> mesh_t {
    mesh_t mesh;
    mesh.append_cube(p_id, p_size, p_position);
    return mesh;
}

auto mv::mesh_t::append_cube(float p_id, float p_size, glm::vec3 p_position)
    -> void {
    append_cube_face(axis_t::x, false, false, p_size, p_id, p_position);
    append_cube_face(axis_t::x, true, true, p_size, p_id, p_position);
    append_cube_face(axis_t::y, false, false, p_size, p_id, p_position);
    append_cube_face(axis_t::y, true, true, p_size, p_id, p_position);
    append_cube_face(axis_t::z, false, false, p_size, p_id, p_position);
    append_cube_face(axis_t::z, true, true, p_size, p_id, p_position);
}

auto mv::mesh_t::append_cube_face(
    axis_t p_axis,
    bool p_negate,
    bool p_backface,
    float p_size,
    float p_id,
    glm::vec3 p_position,
    bool p_flip_uv
) -> void {
    const auto half_size = p_size / 2.0f;

    const float values[][2]{
        {half_size, -half_size},
        {half_size, half_size},
        {-half_size, half_size},
        {-half_size, -half_size},
    };

    const float uvs[][2]{
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
        {0.0f, 0.0f},
    };

    const float flipped_uvs[][2]{
        {1.0f, 1.0f},
        {1.0f, 0.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f},
    };

    const float third_value = p_negate ? -half_size : half_size;

    const std::array new_indices{0, 1, 2, 0, 2, 3};

    const auto pivot_index = static_cast<uint16_t>(vertices.size());

    for (int i = 0; i < 4; i++) {
        float x_value, y_value, z_value;
        float normal_x = 0.0f, normal_y = 0.0f, normal_z = 0.0f;

        switch (p_axis) {
        case axis_t::x:
            z_value = -values[i][0];
            y_value = values[i][1];
            x_value = third_value;
            normal_x = 1.0f;

            if (p_backface) {
                y_value = -y_value;
            }

            break;
        case axis_t::y:
            x_value = values[i][0];
            z_value = -values[i][1];
            y_value = third_value;
            normal_y = 1.0f;

            if (p_backface) {
                z_value = -z_value;
            }

            break;
        case axis_t::z:
            x_value = values[i][0];
            y_value = values[i][1];
            z_value = third_value;
            normal_z = 1.0f;

            if (p_backface) {
                x_value = -x_value;
            }
        }

        if (p_negate) {
            normal_x = -normal_x;
            normal_y = -normal_y;
            normal_z = -normal_z;
        }

        x_value += p_position.x;
        y_value += p_position.y;
        z_value += p_position.z;

        if (p_flip_uv) {
            vertices.push_back({
                .position = glm::vec3{x_value, y_value, z_value},
                .uv = glm::vec2{flipped_uvs[i][0], flipped_uvs[i][1]},
                .normal = glm::vec3{normal_x, normal_y, normal_z},
                .id = p_id,
            });
        } else {
            vertices.push_back({
                .position = glm::vec3{x_value, y_value, z_value},
                .uv = glm::vec2{uvs[i][0], uvs[i][1]},
                .normal = glm::vec3{normal_x, normal_y, normal_z},
                .id = p_id,
            });
        }
    }

    for (auto index : new_indices) {
        indices.push_back(pivot_index + index);
    }
}