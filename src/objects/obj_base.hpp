#pragma once

#include <GL/glew.h>
#include <vector>

#include <filesystem>

namespace fs = std::filesystem;

struct obj_base {
    std::vector<float> points;
    std::vector<GLuint> edges;

    obj_base(std::vector<float> pts, std::vector<GLuint> eg);
};

enum object_type {
    CUBE_3D,
    NUM_3D_OBJ,
    // 4D shapes below
    CUBE_4D,
    COORD_4D,
    CELL5,
    DUO_CYLINDER,
    SPHERE_4D,
    PLANE_4D,
    SUPERPLANE_4D,
    SPRING_4D,
    CYLINDER_PRISM,
    HYPER_PARABOLA,
    HYPER_EX,
    // TODO below
};

obj_base *create_obj_base(object_type tp = object_type::CUBE_4D);

obj_base *create_obj_fromfile(const fs::path& filename, bool is_4D);