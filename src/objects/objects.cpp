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
void object4D::setUniform(std::shared_ptr<ogl::Program> p)
{
    p->setUniform("model_rot", rotate);
    p->setUniform("model_off", offset);
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
void object3D::setUniform(std::shared_ptr<ogl::Program> p)
{
    p->setUniform("model_rot", rotate);
    p->setUniform("model_off", offset);
}