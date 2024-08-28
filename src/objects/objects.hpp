#pragma once

#include "object_base.hpp"

#include <fmt/core.h>

class object4D : public object_base<4>
{
public:
    object4D(obj_base *ob);
    object4D(object_type ot);
    void set_VertexAttrPointer() override;
    void setUniform(std::shared_ptr<ogl::Program> p) override;
};

class object3D : public object_base<3>
{
public:
    object3D(obj_base *ob);
    object3D(object_type ot);
    void set_VertexAttrPointer() override;
    void setUniform(std::shared_ptr<ogl::Program> p) override;
};