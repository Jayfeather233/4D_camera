#pragma once

#include "object_base.hpp"

class object4D : public object_base<4> {
public:
    object4D(obj_base *ob): object_base(ob) {};
    [[override]] void set_VertexAttrPointer(){
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void *)(4 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
};

class object3D : public object_base<3> {
public:
    object3D(obj_base *ob): object_base(ob) {};
    [[override]] void set_VertexAttrPointer(){
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
};


enum object_type
{
    CUBE_3D,
    CUBE_4D,
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
    } else if (tp == object_type::CUBE_3D) {
        obj_base *obj = new obj_base({
            0.0, 0.0, 0.0, 0.8f, 0.8f, 0.0f,
            1.5, 0.0, 0.0, 0.8f, 0.8f, 0.0f,
            0.0, 1.5, 0.0, 0.8f, 0.8f, 0.0f,
            1.5, 1.5, 0.0, 0.8f, 0.8f, 0.0f,
            0.0, 0.0, 1.5, 0.0f, 0.8f, 0.8f,
            1.5, 0.0, 1.5, 0.0f, 0.8f, 0.8f,
            0.0, 1.5, 1.5, 0.0f, 0.8f, 0.8f,
            1.5, 1.5, 1.5, 0.0f, 0.8f, 0.8f,
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
    }
}

object4D create_4D_obj(object_type tp = object_type::CUBE_4D)
{
    return object4D(create_obj_base(tp));
}
object4D create_4D_obj(obj_base *bs)
{
    return object4D(bs);
}
object3D create_3D_obj(object_type tp = object_type::CUBE_3D)
{
    return object3D(create_obj_base(tp));
}
object3D create_3D_obj(obj_base *bs)
{
    return object3D(bs);
}