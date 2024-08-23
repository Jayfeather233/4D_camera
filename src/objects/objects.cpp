#include "object_base.hpp"
#include "objects.hpp"

#include <fmt/core.h>

object4D::object4D(obj_base *ob) : object_base(ob)
{
    if (ob == nullptr)
    {
        fmt::print("Warn: nullptr object at Object4D Constructor");
    }
}
object4D::object4D(object_type ot) : object_base(create_obj_base(ot))
{
}
void object4D::set_VertexAttrPointer()
{
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(1);
}
std::vector<float> object4D::transform_points()
{
    std::vector<float> ret;
    for (size_t i = 0; i < base_model->points.size(); i += 7)
    {
        glm::vec4 v4 = glm::vec4(base_model->points[i], base_model->points[i + 1], base_model->points[i + 2], base_model->points[i + 3]);
        v4 = rotate * (v4 + offset);
        for (size_t j = 0; j < 4; ++j)
        {
            ret.push_back(v4[j]);
        }
        for (size_t j = 0; j < 3; ++j)
        {
            ret.push_back(base_model->points[i + j + 4]);
        }
    }
    return ret;
}
object3D::object3D(obj_base *ob) : object_base(ob)
{
    if (ob == nullptr)
    {
        fmt::print("Warn: nullptr object at Object3D Constructor");
    }
}
object3D::object3D(object_type ot) : object_base(create_obj_base(ot))
{
}
void object3D::set_VertexAttrPointer()
{
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}
std::vector<float> object3D::transform_points()
{
    std::vector<float> ret;
    for (size_t i = 0; i < base_model->points.size(); i += 6)
    {
        glm::vec3 v4 = glm::vec3(base_model->points[i], base_model->points[i + 1], base_model->points[i + 2]);
        v4 = rotate * (v4 + offset);
        for (size_t j = 0; j < 3; ++j)
        {
            ret.push_back(v4[j]);
        }
        for (size_t j = 0; j < 3; ++j)
        {
            ret.push_back(base_model->points[i + j + 3]);
        }
    }
    return ret;
}