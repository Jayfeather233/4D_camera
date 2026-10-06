#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <utility>

#include <glm/glm.hpp>

#include "opengl/mesh.hpp"
#include "opengl/shader.hpp"

/**
 * A drawable instance of a mesh.
 *
 * The GPU buffers live in the shared ogl::Mesh, so any number of Object<D> can
 * reuse the same vertex data and only carry their own transform. D is the
 * dimension of the source shape (3 or 4).
 *
 * The model transform is `rotate_ * p + offset_`, i.e. the same thing the
 * vertex shaders compute from `model_rot` / `model_off`.
 */
template <unsigned int D> class Object {
public:
    using vec_t = glm::vec<D, float>;
    using mat_t = glm::mat<D, D, float>;

    explicit Object(ogl::MeshPtr mesh, const vec_t &offset = vec_t(0.0f),
                    const mat_t &rotate = glm::mat<D, D, float>(1.0f))
        : mesh_(std::move(mesh)), offset_(offset), rotate_(rotate)
    {
        assert(!mesh_ || mesh_->dim() == static_cast<int>(D));
    }

    const ogl::MeshPtr &mesh() const { return mesh_; }
    const vec_t &offset() const { return offset_; }
    const mat_t &rotate() const { return rotate_; }

    void setOffset(const vec_t &off) { offset_ = off; }
    void addOffset(const vec_t &off) { offset_ += off; }
    void setRotate(const mat_t &rot) { rotate_ = rot; }
    void addRotate(const mat_t &rot) { rotate_ = rot * rotate_; }
    void addScale(float scale) { rotate_ = mat_t(scale) * rotate_; }

    /// Bounding box of the mesh in model space. False for an empty mesh.
    bool boundingBox(vec_t &min, vec_t &max) const
    {
        if (!mesh_ || mesh_->vertex_count() == 0)
            return false;
        const float *first = mesh_->data().position(0);
        for (unsigned int k = 0; k < D; ++k)
            min[k] = max[k] = first[k];
        for (std::size_t i = 1; i < mesh_->vertex_count(); ++i) {
            const float *p = mesh_->data().position(i);
            for (unsigned int k = 0; k < D; ++k) {
                min[k] = std::min(min[k], p[k]);
                max[k] = std::max(max[k], p[k]);
            }
        }
        return true;
    }

    /// Recenters the mesh around its bounding box center.
    void autoCenter()
    {
        vec_t min, max;
        if (!boundingBox(min, max))
            return;
        offset_ -= rotate_ * ((min + max) * 0.5f);
    }

    /// Recenters and scales the mesh so that every shape occupies the same
    /// volume, which makes a common 4D camera distance work for all scenes.
    void normalize(float radius = 1.0f)
    {
        vec_t min, max;
        if (!boundingBox(min, max))
            return;
        const vec_t center = (min + max) * 0.5f;
        float max_squared = 0.0f;
        for (std::size_t i = 0; i < mesh_->vertex_count(); ++i) {
            const float *p = mesh_->data().position(i);
            float squared = 0.0f;
            for (unsigned int k = 0; k < D; ++k) {
                const float d = p[k] - center[k];
                squared += d * d;
            }
            max_squared = std::max(max_squared, squared);
        }
        const float current = std::sqrt(max_squared);
        const float scale = current > 0.0f ? radius / current : 1.0f;
        // new(x) = old(scale * (x - center))
        rotate_ = rotate_ * mat_t(scale);
        offset_ -= rotate_ * center;
    }

    /// Uploads the per instance transform. glUniform* picks the mat3/vec3 or
    /// mat4/vec4 overload from D automatically.
    void setUniform(const std::shared_ptr<ogl::Program> &program) const
    {
        program->setUniform("model_rot", rotate_);
        program->setUniform("model_off", offset_);
    }

    void draw(ogl::RenderMode mode, const std::shared_ptr<ogl::Program> &program) const
    {
        if (mesh_)
            mesh_->draw(mode, program);
    }

private:
    ogl::MeshPtr mesh_;
    vec_t offset_;
    mat_t rotate_;
};

using Object3D = Object<3>;
using Object4D = Object<4>;
