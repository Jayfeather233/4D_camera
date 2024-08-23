#pragma once

#include "object_base.hpp"

#include <fmt/core.h>

class object4D : public object_base<4>
{
public:
    object4D(obj_base *ob) : object_base(ob)
    {
        if (ob == nullptr)
        {
            fmt::print("Warn: nullptr object at Object4D Constructor");
        }
    };
    [[override]] void set_VertexAttrPointer()
    {
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void *)(4 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    [[override]] std::vector<float> transform_points(){
        std::vector<float> ret;
        for(size_t i=0;i<base_model->points.size(); i += 7){
            glm::vec4 v4 = glm::vec4(base_model->points[i],base_model->points[i+1],base_model->points[i+2],base_model->points[i+3]);
            v4 = rotate * (v4 + offset);
            for(size_t j = 0;j<4;++j){
                ret.push_back(v4[j]);
            }
            for(size_t j = 0;j<3;++j){
                ret.push_back(base_model->points[i+j+4]);
            }
        }
        return ret;
    }
};

class object3D : public object_base<3>
{
public:
    object3D(obj_base *ob) : object_base(ob)
    {
        if (ob == nullptr)
        {
            fmt::print("Warn: nullptr object at Object3D Constructor");
        }
    };
    [[override]] void set_VertexAttrPointer()
    {
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }
    [[override]] std::vector<float> transform_points(){
        std::vector<float> ret;
        for(size_t i=0;i<base_model->points.size(); i += 6){
            glm::vec3 v4 = glm::vec3(base_model->points[i],base_model->points[i+1],base_model->points[i+2]);
            v4 = rotate * (v4 + offset);
            for(size_t j = 0;j<3;++j){
                ret.push_back(v4[j]);
            }
            for(size_t j = 0;j<3;++j){
                ret.push_back(base_model->points[i+j+3]);
            }
        }
        return ret;
    }
};

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