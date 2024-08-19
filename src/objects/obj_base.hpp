#pragma once

#include <vector>
#include <GL/glew.h>

struct obj_base
{
    std::vector<float> points;
    std::vector<GLuint> edges;

    obj_base(std::vector<float> pts, std::vector<GLuint> eg) : points(pts), edges(eg) {}
};