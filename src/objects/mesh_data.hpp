#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <memory>
#include <string_view>
#include <vector>

/**
 * CPU side geometry of a shape.
 *
 * Vertices are interleaved as `position[dim]` followed by `color[3]`, exactly in
 * the layout the vertex shaders expect. Two independent index lists are kept so
 * the same shape can be drawn as a wireframe (`line_indices`, GL_LINES) or as a
 * surface (`tri_indices`, GL_TRIANGLES). A shape is allowed to have no
 * triangles (e.g. a set of coordinate axes): renderers then fall back to lines.
 */
struct mesh_data {
    int dim = 4; // position components: 3 or 4
    std::vector<float> vertices;
    std::vector<std::uint32_t> line_indices;
    std::vector<std::uint32_t> tri_indices;

    std::size_t stride() const { return static_cast<std::size_t>(dim) + 3; }
    std::size_t vertex_count() const { return vertices.size() / stride(); }
    bool empty() const { return vertices.empty(); }
    bool has_triangles() const { return !tri_indices.empty(); }

    /// Pointer to the position of vertex `i` (`dim` floats).
    const float *position(std::size_t i) const { return vertices.data() + i * stride(); }
};

/// Low level helpers for hand written geometry (the position list must hold
/// `dim` values, exactly like the interleaved vertex layout).
std::uint32_t mesh_add_vertex(mesh_data &m, std::initializer_list<float> position, float r, float g, float b);
void mesh_add_line(mesh_data &m, std::uint32_t a, std::uint32_t b);
void mesh_add_triangle(mesh_data &m, std::uint32_t a, std::uint32_t b, std::uint32_t c);

enum class object_type {
    CUBE_3D,
    // 4D shapes
    CUBE_4D,
    COORD_4D,
    CELL5,
    DUO_CYLINDER,
    SPHERE_4D,
    PLANE_4D,
    SUPERPLANE_4D,
    SPRING_4D,
    CYLINDER_PRISM,
    HYPER_PARABOLA,
    HYPER_EX,
};

/// Human readable name of a shape (stable, used by the CLI).
const char *object_type_name(object_type tp);
/// Parses a name produced by object_type_name(). Returns false if unknown.
bool object_type_from_name(std::string_view name, object_type &out);
/// All shapes, in declaration order.
const std::vector<object_type> &all_object_types();

std::shared_ptr<mesh_data> create_obj_base(object_type tp = object_type::CUBE_4D);

std::shared_ptr<mesh_data> create_obj_fromfile(const std::filesystem::path &filename, bool is_4D);

/**
 * A "floor" for the explorable scene: the hyperplane y = height, spanning
 * [-half_size, half_size] in x, z and w. It comes with a grid line list and
 * triangles, so it works in every render mode.
 */
std::shared_ptr<mesh_data> create_hyperplane_floor(float half_size, int cells, float height = 0.0f);
