#include "opengl/mesh.hpp"

#include <cstdint>
#include <vector>

namespace ogl {

namespace {
constexpr std::size_t kIndexSize = sizeof(std::uint32_t);

void *index_offset(GLsizei first_index)
{
    return reinterpret_cast<void *>(static_cast<std::uintptr_t>(first_index) * kIndexSize);
}
} // namespace

const char *render_mode_name(RenderMode mode)
{
    switch (mode) {
    case RenderMode::Wireframe: return "wireframe";
    case RenderMode::Solid: return "solid";
    case RenderMode::SolidWireframe: return "solid+wireframe";
    case RenderMode::TriangleWireframe: return "triangle wireframe";
    }
    return "unknown";
}

RenderMode next_render_mode(RenderMode mode)
{
    switch (mode) {
    case RenderMode::Wireframe: return RenderMode::Solid;
    case RenderMode::Solid: return RenderMode::SolidWireframe;
    case RenderMode::SolidWireframe: return RenderMode::TriangleWireframe;
    case RenderMode::TriangleWireframe: return RenderMode::Wireframe;
    }
    return RenderMode::Wireframe;
}

Mesh::Mesh(const std::shared_ptr<const mesh_data> &data) : data_(data)
{
    if (!data_ || data_->vertices.empty())
        return;

    // One element buffer holds lines first, then triangles.
    std::vector<std::uint32_t> indices;
    indices.reserve(data_->line_indices.size() + data_->tri_indices.size());
    indices.insert(indices.end(), data_->line_indices.begin(), data_->line_indices.end());
    indices.insert(indices.end(), data_->tri_indices.begin(), data_->tri_indices.end());

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    upload(indices, GL_STATIC_DRAW);
}

void Mesh::update(const mesh_data &data)
{
    data_ = std::make_shared<const mesh_data>(data);
    if (!vao_) {
        if (data_->vertices.empty())
            return;
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &vbo_);
        glGenBuffers(1, &ebo_);
    }
    std::vector<std::uint32_t> indices;
    indices.reserve(data_->line_indices.size() + data_->tri_indices.size());
    indices.insert(indices.end(), data_->line_indices.begin(), data_->line_indices.end());
    indices.insert(indices.end(), data_->tri_indices.begin(), data_->tri_indices.end());
    upload(indices, GL_DYNAMIC_DRAW);
}

void Mesh::upload(const std::vector<std::uint32_t> &indices, GLenum usage)
{
    line_count_ = static_cast<GLsizei>(data_->line_indices.size());
    tri_count_ = static_cast<GLsizei>(data_->tri_indices.size());
    tri_first_ = static_cast<GLsizei>(data_->line_indices.size());

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(data_->vertices.size() * sizeof(float)),
                 data_->vertices.data(), usage);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * kIndexSize), indices.data(),
                 usage);

    const GLsizei stride = static_cast<GLsizei>(data_->stride() * sizeof(float));
    glVertexAttribPointer(0, data_->dim, GL_FLOAT, GL_FALSE, stride, nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<void *>(static_cast<std::uintptr_t>(data_->dim * sizeof(float))));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

Mesh::~Mesh()
{
    if (ebo_)
        glDeleteBuffers(1, &ebo_);
    if (vbo_)
        glDeleteBuffers(1, &vbo_);
    if (vao_)
        glDeleteVertexArrays(1, &vao_);
}

void Mesh::draw(RenderMode mode, const std::shared_ptr<Program> &program) const
{
    if (!vao_)
        return;
    const bool use_triangles = tri_count_ > 0;

    glBindVertexArray(vao_);
    switch (mode) {
    case RenderMode::Wireframe:
        glDrawElements(GL_LINES, line_count_, GL_UNSIGNED_INT, nullptr);
        break;
    case RenderMode::Solid:
        if (use_triangles)
            glDrawElements(GL_TRIANGLES, tri_count_, GL_UNSIGNED_INT, index_offset(tri_first_));
        else
            glDrawElements(GL_LINES, line_count_, GL_UNSIGNED_INT, nullptr);
        break;
    case RenderMode::SolidWireframe:
        if (use_triangles) {
            // Push the filled triangles back so the wireframe stays visible. The
            // depth is written by the fragment shader, so glPolygonOffset cannot
            // be used and the bias goes through a uniform instead.
            if (program)
                program->setUniform("depth_bias", 0.0008f);
            glDrawElements(GL_TRIANGLES, tri_count_, GL_UNSIGNED_INT, index_offset(tri_first_));
            if (program)
                program->setUniform("depth_bias", 0.0f);
        }
        glDrawElements(GL_LINES, line_count_, GL_UNSIGNED_INT, nullptr);
        break;
    case RenderMode::TriangleWireframe:
        if (use_triangles) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glDrawElements(GL_TRIANGLES, tri_count_, GL_UNSIGNED_INT, index_offset(tri_first_));
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else {
            glDrawElements(GL_LINES, line_count_, GL_UNSIGNED_INT, nullptr);
        }
        break;
    }
    glBindVertexArray(0);
}

MeshPtr MeshLibrary::get(const std::shared_ptr<const mesh_data> &data)
{
    if (!data)
        return nullptr;
    auto it = meshes_.find(data.get());
    if (it != meshes_.end())
        return it->second;
    MeshPtr mesh = std::make_shared<Mesh>(data);
    meshes_.emplace(data.get(), mesh);
    return mesh;
}

} // namespace ogl
