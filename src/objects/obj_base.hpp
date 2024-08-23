#pragma once

#include <vector>
#include <GL/glew.h>

struct obj_base
{
    std::vector<float> points;
    std::vector<GLuint> edges;

    obj_base(std::vector<float> pts, std::vector<GLuint> eg) : points(pts), edges(eg) {}
};


enum object_type
{
    CUBE_3D,
    NUM_3D_OBJ,
    // 4D shapes below
    CUBE_4D,
    COORD_4D,
    // TODO below
    SPHERE_4D,
    PLANE_4D,

};

obj_base *create_obj_base(object_type tp = object_type::CUBE_4D)
{
    if (tp == object_type::CUBE_4D)
    {
        obj_base *obj = new obj_base({
            0, 0, 0, 0, 0.8f, 0.8f, 0.0f,
            1, 0, 0, 0, 0.8f, 0.8f, 0.0f,
            0, 1, 0, 0, 0.8f, 0.8f, 0.0f,
            1, 1, 0, 0, 0.8f, 0.8f, 0.0f,
            0, 0, 1, 0, 0.0f, 0.8f, 0.8f,
            1, 0, 1, 0, 0.0f, 0.8f, 0.8f,
            0, 1, 1, 0, 0.0f, 0.8f, 0.8f,
            1, 1, 1, 0, 0.0f, 0.8f, 0.8f,
            0, 0, 0, 1, 0.8f, 0.0f, 0.8f,
            1, 0, 0, 1, 0.8f, 0.0f, 0.8f,
            0, 1, 0, 1, 0.8f, 0.0f, 0.8f,
            1, 1, 0, 1, 0.8f, 0.0f, 0.8f,
            0, 0, 1, 1, 0.8f, 0.0f, 0.0f,
            1, 0, 1, 1, 0.8f, 0.0f, 0.0f,
            0, 1, 1, 1, 0.8f, 0.0f, 0.0f,
            1, 1, 1, 1, 0.8f, 0.0f, 0.0f,
        },{
            0, 1,
            0, 2,
            1, 3,
            2, 3,
            
            4, 5,
            4, 6,
            5, 7,
            6, 7,
            
            8, 9,
            8, 10,
            9, 11,
            10, 11,
            
            12, 13,
            12, 14,
            13, 15,
            14, 15,

            0, 4,
            1, 5,
            2, 6,
            3, 7,

            0, 8,
            1, 9,
            2, 10,
            3, 11,

            8, 12,
            9, 13,
            10, 14,
            11, 15,

            4, 12,
            5, 13,
            6, 14,
            7, 15
        });
        return obj;
    }
    else if (tp == object_type::CUBE_3D)
    {
        obj_base *obj = new obj_base({
            0.0, 0.0, 0.0, 1.0f, 1.0f, 1.0f,
            1.0, 0.0, 0.0, 1.0f, 1.0f, 1.0f,
            0.0, 1.0, 0.0, 1.0f, 1.0f, 1.0f,
            1.0, 1.0, 0.0, 1.0f, 1.0f, 1.0f,
            0.0, 0.0, 1.0, 1.0f, 1.0f, 1.0f,
            1.0, 0.0, 1.0, 1.0f, 1.0f, 1.0f,
            0.0, 1.0, 1.0, 1.0f, 1.0f, 1.0f,
            1.0, 1.0, 1.0, 1.0f, 1.0f, 1.0f,
        },{
            0, 1,
            0, 2,
            1, 3,
            2, 3,
            
            4, 5,
            4, 6,
            5, 7,
            6, 7,
            
            0, 4,
            1, 5,
            2, 6,
            3, 7,
        });
        return obj;
    } else if (tp == object_type::COORD_4D){
        obj_base *obj = new obj_base({
            20, 0, 0, 0, 1.0f, 1.0f, 1.0f,
            -20, 0, 0, 0, 1.0f, 1.0f, 1.0f,
            0, 20, 0, 0, 1.0f, 1.0f, 1.0f,
            0, -20, 0, 0, 1.0f, 1.0f, 1.0f,
            0, 0, 20, 0, 1.0f, 1.0f, 1.0f,
            0, 0, -20, 0, 1.0f, 1.0f, 1.0f,
            0, 0, 0, 20, 1.0f, 1.0f, 1.0f,
            0, 0, 0, -20, 1.0f, 1.0f, 1.0f,
        },{
            0, 1,
            2, 3,
            4, 5,
            6, 7
        });
        return obj;
    }
    return nullptr;
}