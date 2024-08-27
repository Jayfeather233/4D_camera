#pragma once

#include <vector>
#include <GL/glew.h>

struct obj_base
{
    std::vector<float> points;
    std::vector<GLuint> edges;

    obj_base(std::vector<float> pts, std::vector<GLuint> eg);
};


enum object_type
{
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
    // TODO below
};

obj_base *create_obj_base(object_type tp = object_type::CUBE_4D);