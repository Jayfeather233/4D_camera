#pragma once

#include <GL/glew.h>

#include <cstddef>
#include <memory>
#include <unordered_map>

#include "objects/mesh_data.hpp"
#include "opengl/shader.hpp"

namespace ogl {

/// How geometry is rasterized.
enum class RenderMode {
    Wireframe,        ///< the shape's own line list (grid / edges)
    Solid,            ///< triangle list, opaque
    SolidWireframe,   ///< triangles plus the line list on top
    TriangleWireframe ///< triangles drawn as lines (glPolygonMode)
};

const char *render_mode_name(RenderMode mode);
RenderMode next_render_mode(RenderMode mode);

/**
 * GPU buffers of one mesh_data. Buffers are created once and can be shared by
 * any number of Object<D> instances; the line and triangle index lists are
 * packed into a single element buffer so switching render mode never rebinds.
 */
class Mesh {
public:
    explicit Mesh(const std::shared_ptr<const mesh_data> &data);
    ~Mesh();

    /// Re-uploads the buffers; used for dynamic geometry such as the HUD.
    void update(const mesh_data &data);

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    Mesh(Mesh &&) = delete;
    Mesh &operator=(Mesh &&) = delete;

    /// `program` must be the one currently bound for this mesh: the filled pass
    /// of the wireframe overlay needs a small depth bias.
    void draw(RenderMode mode, const std::shared_ptr<Program> &program) const;

    int dim() const { return data_->dim; }
    bool has_triangles() const { return data_->has_triangles(); }
    std::size_t vertex_count() const { return data_->vertex_count(); }
    GLsizei line_count() const { return line_count_; }
    GLsizei triangle_count() const { return tri_count_; }
    const mesh_data &data() const { return *data_; }

private:
    void upload(const std::vector<std::uint32_t> &indices, GLenum usage);

    std::shared_ptr<const mesh_data> data_;
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei line_count_ = 0;
    GLsizei tri_count_ = 0;
    GLsizei tri_first_ = 0; // first triangle index inside the shared element buffer
};

using MeshPtr = std::shared_ptr<Mesh>;

/// Uploads each mesh_data once and hands out shared Mesh objects.
class MeshLibrary {
public:
    MeshPtr get(const std::shared_ptr<const mesh_data> &data);
    std::size_t size() const { return meshes_.size(); }

private:
    std::unordered_map<const mesh_data *, MeshPtr> meshes_;
};

} // namespace ogl
