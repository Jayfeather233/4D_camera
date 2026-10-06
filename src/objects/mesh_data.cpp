#include "mesh_data.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <vector>

#include <Eigen/Core>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace {

using Eigen::scomplex;
using Eigen::Vector2f;
using Eigen::Vector3f;
using Eigen::Vector4f;

// ---------------------------------------------------------------- vertex data

std::uint32_t add_vertex(mesh_data &m, std::initializer_list<float> position, const Vector3f &color)
{
    return mesh_add_vertex(m, position, color.x(), color.y(), color.z());
}

void add_line(mesh_data &m, std::uint32_t a, std::uint32_t b) { mesh_add_line(m, a, b); }
void add_triangle(mesh_data &m, std::uint32_t a, std::uint32_t b, std::uint32_t c) { mesh_add_triangle(m, a, b, c); }

// ------------------------------------------------------- structured grids

/// Calls f() for every multi index of a `dims[0] x dims[1] x ...` grid.
template <int DIMS, typename F> void for_each_index(const std::array<int, DIMS> &dims, F &&f)
{
    for (int a = 0; a < DIMS; ++a) {
        if (dims[a] <= 0)
            return;
    }
    std::array<int, DIMS> k{};
    while (true) {
        f(k);
        int a = DIMS - 1;
        for (; a >= 0; --a) {
            if (++k[a] < dims[a])
                break;
            k[a] = 0;
        }
        if (a < 0)
            break;
    }
}

/**
 * Builds the topology of a structured grid whose vertices were pushed
 * contiguously starting at `base` (row major, last axis fastest).
 *
 * @param periodic  axis i is cyclic: its last vertex connects back to the first.
 * @param with_faces also emit GL_TRIANGLES for every axis aligned slice family
 *                   of the grid. For a 2D grid that is the surface itself; for a
 *                   3D grid it renders the volume as a "solid" (interior slices
 *                   are hidden by the outer shell when depth testing is on).
 */
template <int DIMS>
void emit_structured_grid(mesh_data &m, std::uint32_t base, const std::array<int, DIMS> &dims,
                          const std::array<bool, DIMS> &periodic, bool with_faces)
{
    for (int a = 0; a < DIMS; ++a) {
        if (dims[a] < 2)
            return;
    }

    const auto flat = [&](const std::array<int, DIMS> &k) {
        int f = 0;
        for (int a = 0; a < DIMS; ++a)
            f = f * dims[a] + k[a];
        return base + static_cast<std::uint32_t>(f);
    };
    const auto cells = [&](int a) { return periodic[a] ? dims[a] : dims[a] - 1; };

    // Wireframe: every grid edge, emitted exactly once.
    for (int a = 0; a < DIMS; ++a) {
        for_each_index<DIMS>(dims, [&](std::array<int, DIMS> k) {
            if (k[a] + 1 < dims[a]) {
                auto n = k;
                ++n[a];
                add_line(m, flat(k), flat(n));
            }
            else if (periodic[a]) {
                auto n = k;
                n[a] = 0;
                add_line(m, flat(k), flat(n));
            }
        });
    }

    if (!with_faces)
        return;

    // Surface: two triangles per grid cell, for every pair of axes.
    for (int a = 0; a < DIMS; ++a) {
        for (int b = a + 1; b < DIMS; ++b) {
            std::array<int, DIMS> ranges = dims;
            ranges[a] = cells(a);
            ranges[b] = cells(b);
            for_each_index<DIMS>(ranges, [&](std::array<int, DIMS> k) {
                auto ka = k, kb = k, kab = k;
                ka[a] = (k[a] + 1) % dims[a];
                kb[b] = (k[b] + 1) % dims[b];
                kab[a] = ka[a];
                kab[b] = kb[b];
                const std::uint32_t i0 = flat(k), i1 = flat(ka), i2 = flat(kb), i3 = flat(kab);
                add_triangle(m, i0, i1, i3);
                add_triangle(m, i0, i3, i2);
            });
        }
    }
}

// ------------------------------------------------------------ colour helpers

Vector4f identity_4d(const scomplex &u) { return {u.real(), u.imag(), 0.0f, 0.0f}; }

/**
 * Saturated colour wheel: three cosine phases 120 degrees apart, so every point
 * gets a distinct, fully saturated colour (never white) and the gradient shows
 * which way a surface faces. Periodic in u and v, so periodic shapes keep a
 * seamless seam.
 */
Vector3f rainbow(float u, float v)
{
    const float tau = glm::two_pi<float>();
    return {0.5f + 0.5f * glm::cos(tau * u), 0.5f + 0.5f * glm::cos(tau * v + 2.0944f),
            0.5f + 0.5f * glm::cos(tau * (u + v) + 4.1888f)};
}

/// Saturated colour from a 3 parameter domain (same idea as rainbow()).
Vector3f rainbow3(float u, float v, float w)
{
    const float tau = glm::two_pi<float>();
    return {0.5f + 0.5f * glm::cos(tau * u), 0.5f + 0.5f * glm::cos(tau * v + 2.0944f),
            0.5f + 0.5f * glm::cos(tau * w + 4.1888f)};
}

/// Colour of a 4D axis: x, y, z, w.
Vector3f axis_color(int axis, bool positive)
{
    static const Vector3f colors[4] = {
        {1.0f, 0.28f, 0.28f}, // x: red
        {0.32f, 0.95f, 0.36f}, // y: green
        {0.34f, 0.56f, 1.0f},  // z: blue
        {1.0f, 0.86f, 0.24f},  // w: yellow
    };
    return positive ? colors[axis] : colors[axis] * 0.45f;
}

Vector3f identity_color_2d(const scomplex &u) { return rainbow(u.real(), u.imag()); }
Vector4f identity_3d(const Vector3f &u) { return {u.x(), u.y(), u.z(), 0.0f}; }
Vector3f identity_color_3d(const Vector3f &u) { return rainbow3(u.x(), u.y(), u.z()); }

/// 3 parameter surface: f(u, v) -> (x, y, z, w), u, v in [-D, D].
std::shared_ptr<mesh_data> create_hyper_plane(const std::function<Vector4f(const scomplex &)> &f, int N, Vector2f D,
                                              const std::function<Vector3f(const scomplex &)> &color, bool wrap_u,
                                              bool wrap_v)
{
    auto obj = std::make_shared<mesh_data>();
    obj->dim = 4;
    obj->vertices.reserve(static_cast<std::size_t>(N) * N * 7);
    const float span = static_cast<float>(N - 1);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            const scomplex ori((i - span / 2.0f) / span * 2.0f * D.x(), (j - span / 2.0f) / span * 2.0f * D.y());
            const Vector4f a = f(ori);
            const Vector3f col = color(scomplex(i / span, j / span));
            add_vertex(*obj, {a.x(), a.y(), a.z(), a.w()}, col);
        }
    }
    emit_structured_grid<2>(*obj, 0, {N, N}, {wrap_u, wrap_v}, true);
    return obj;
}

/// 4 parameter volume: f(u, v, w) -> (x, y, z, w), each parameter in [-D, D].
std::shared_ptr<mesh_data> create_cube(const std::function<Vector4f(const Vector3f &)> &f, int N, Vector3f D,
                                       const std::function<Vector3f(const Vector3f &)> &color, bool wrap_u, bool wrap_v,
                                       bool wrap_w)
{
    auto obj = std::make_shared<mesh_data>();
    obj->dim = 4;
    obj->vertices.reserve(static_cast<std::size_t>(N) * N * N * 7);
    const float span = static_cast<float>(N - 1);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            for (int k = 0; k < N; ++k) {
                const Vector3f ori((i - span / 2.0f) / span * 2.0f * D.x(), (j - span / 2.0f) / span * 2.0f * D.y(),
                                   (k - span / 2.0f) / span * 2.0f * D.z());
                const Vector4f a = f(ori);
                const Vector3f col = color(Vector3f(i / span, j / span, k / span));
                add_vertex(*obj, {a.x(), a.y(), a.z(), a.w()}, col);
            }
        }
    }
    emit_structured_grid<3>(*obj, 0, {N, N, N}, {wrap_u, wrap_v, wrap_w}, true);
    return obj;
}

/// Emits the 12 triangles of a box whose 8 vertices are given in bit order:
/// v[n] has bit 0 -> negative/positive corner of the first local axis, etc.
void add_box_triangles(mesh_data &m, const std::array<std::uint32_t, 8> &v)
{
    for (int axis = 0; axis < 3; ++axis) {
        const int p = (axis + 1) % 3;
        const int q = (axis + 2) % 3;
        for (int side = 0; side < 2; ++side) {
            const std::uint32_t base = static_cast<std::uint32_t>(side << axis);
            const std::uint32_t a = v[base];
            const std::uint32_t b = v[base | (1u << p)];
            const std::uint32_t c = v[base | (1u << p) | (1u << q)];
            const std::uint32_t d = v[base | (1u << q)];
            add_triangle(m, a, b, c);
            add_triangle(m, a, c, d);
        }
    }
}

/**
 * 4D hypercube. Its boundary is made of 8 cubical cells (one per axis and side),
 * all of which are emitted as triangles, so solid rendering shows a solid
 * hypercube. Vertex index bit 0..3 = x, y, z, w.
 */
std::shared_ptr<mesh_data> create_hypercube()
{
    auto obj = std::make_shared<mesh_data>();
    obj->dim = 4;
    const float s = 0.5f;
    // Same colour scheme as before: one colour per (z, w) pair of cells.
    static const Vector3f cell_color[2][2] = {
        {{0.8f, 0.8f, 0.0f}, {0.8f, 0.0f, 0.8f}}, // z = 0: w = 0, 1
        {{0.0f, 0.8f, 0.8f}, {0.8f, 0.0f, 0.0f}}, // z = 1: w = 0, 1
    };
    for (int v = 0; v < 16; ++v) {
        const int x = v & 1, y = (v >> 1) & 1, z = (v >> 2) & 1, w = (v >> 3) & 1;
        add_vertex(*obj, {x ? s : -s, y ? s : -s, z ? s : -s, w ? s : -s}, cell_color[z][w]);
    }
    for (int v = 0; v < 16; ++v) {
        for (int a = 0; a < 4; ++a) {
            if (!(v >> a & 1))
                add_line(*obj, static_cast<std::uint32_t>(v), static_cast<std::uint32_t>(v | (1 << a)));
        }
    }
    for (int axis = 0; axis < 4; ++axis) {
        int other[3];
        int n = 0;
        for (int a = 0; a < 4; ++a) {
            if (a != axis)
                other[n++] = a;
        }
        for (int side = 0; side < 2; ++side) {
            std::array<std::uint32_t, 8> cell{};
            for (int local = 0; local < 8; ++local) {
                std::uint32_t idx = static_cast<std::uint32_t>(side << axis);
                for (int j = 0; j < 3; ++j) {
                    if (local >> j & 1)
                        idx |= 1u << other[j];
                }
                cell[local] = idx;
            }
            add_box_triangles(*obj, cell);
        }
    }
    return obj;
}

} // namespace

std::uint32_t mesh_add_vertex(mesh_data &m, std::initializer_list<float> position, float r, float g, float b)
{
    const auto id = static_cast<std::uint32_t>(m.vertex_count());
    m.vertices.insert(m.vertices.end(), position);
    m.vertices.push_back(r);
    m.vertices.push_back(g);
    m.vertices.push_back(b);
    return id;
}

void mesh_add_line(mesh_data &m, std::uint32_t a, std::uint32_t b)
{
    m.line_indices.push_back(a);
    m.line_indices.push_back(b);
}

void mesh_add_triangle(mesh_data &m, std::uint32_t a, std::uint32_t b, std::uint32_t c)
{
    m.tri_indices.push_back(a);
    m.tri_indices.push_back(b);
    m.tri_indices.push_back(c);
}

const char *object_type_name(object_type tp)
{
    switch (tp) {
    case object_type::CUBE_3D: return "cube3d";
    case object_type::CUBE_4D: return "cube4d";
    case object_type::COORD_4D: return "coords";
    case object_type::CELL5: return "cell5";
    case object_type::DUO_CYLINDER: return "duo-cylinder";
    case object_type::SPHERE_4D: return "sphere4d";
    case object_type::PLANE_4D: return "plane4d";
    case object_type::SUPERPLANE_4D: return "superplane4d";
    case object_type::SPRING_4D: return "spring4d";
    case object_type::CYLINDER_PRISM: return "cylinder-prism";
    case object_type::HYPER_PARABOLA: return "hyper-parabola";
    case object_type::HYPER_EX: return "hyper-exp";
    }
    return "unknown";
}

const std::vector<object_type> &all_object_types()
{
    static const std::vector<object_type> types = {
        object_type::CUBE_3D,        object_type::CUBE_4D,         object_type::COORD_4D,
        object_type::CELL5,          object_type::DUO_CYLINDER,    object_type::SPHERE_4D,
        object_type::PLANE_4D,       object_type::SUPERPLANE_4D,   object_type::SPRING_4D,
        object_type::CYLINDER_PRISM, object_type::HYPER_PARABOLA,  object_type::HYPER_EX,
    };
    return types;
}

bool object_type_from_name(std::string_view name, object_type &out)
{
    for (object_type tp : all_object_types()) {
        if (name == object_type_name(tp)) {
            out = tp;
            return true;
        }
    }
    return false;
}

std::shared_ptr<mesh_data> create_obj_base(object_type tp)
{
    switch (tp) {
    case object_type::CUBE_4D:
        return create_hypercube();

    case object_type::CUBE_3D: {
        // The 3D layer is only ever used as a wireframe size reference, so this
        // shape deliberately has no triangles: just 8 corners and 12 edges.
        auto obj = std::make_shared<mesh_data>();
        obj->dim = 3;
        const float s = 0.5f;
        for (int v = 0; v < 8; ++v) {
            const int x = v & 1, y = (v >> 1) & 1, z = (v >> 2) & 1;
            add_vertex(*obj, {x ? s : -s, y ? s : -s, z ? s : -s}, {0.72f, 0.78f, 0.88f});
        }
        for (int v = 0; v < 8; ++v) {
            for (int a = 0; a < 3; ++a) {
                if (!(v >> a & 1))
                    add_line(*obj, static_cast<std::uint32_t>(v), static_cast<std::uint32_t>(v | (1 << a)));
            }
        }
        return obj;
    }

    case object_type::COORD_4D: {
        auto obj = std::make_shared<mesh_data>();
        obj->dim = 4;
        for (int a = 0; a < 4; ++a) {
            std::array<float, 4> pos{0.0f, 0.0f, 0.0f, 0.0f};
            pos[a] = 20.0f;
            const std::uint32_t p = add_vertex(*obj, {pos[0], pos[1], pos[2], pos[3]}, axis_color(a, true));
            pos[a] = -20.0f;
            const std::uint32_t n = add_vertex(*obj, {pos[0], pos[1], pos[2], pos[3]}, axis_color(a, false));
            add_line(*obj, p, n);
        }
        return obj;
    }

    case object_type::CELL5: {
        auto obj = std::make_shared<mesh_data>();
        obj->dim = 4;
        // One colour per vertex: every one of the 5 cells then reads differently.
        add_vertex(*obj, {0.0f, 0.0f, 2.1213f, -0.5477f}, {1.00f, 0.30f, 0.28f});
        add_vertex(*obj, {-1.0f, 1.7320f, -0.7071f, -0.5477f}, {0.32f, 0.95f, 0.36f});
        add_vertex(*obj, {-1.0f, -1.7320f, -0.7071f, -0.5477f}, {0.34f, 0.56f, 1.00f});
        add_vertex(*obj, {2.0f, 0.0f, -0.7071f, -0.5477f}, {1.00f, 0.86f, 0.24f});
        add_vertex(*obj, {0.0f, 0.0f, 0.0f, 2.1909f}, {0.95f, 0.36f, 0.92f});
        for (std::uint32_t a = 0; a < 5; ++a) {
            for (std::uint32_t b = a + 1; b < 5; ++b)
                add_line(*obj, a, b);
        }
        // Boundary: 5 tetrahedra (each one omitting one vertex), 4 faces each.
        for (std::uint32_t omitted = 0; omitted < 5; ++omitted) {
            std::array<std::uint32_t, 4> tetra{};
            int n = 0;
            for (std::uint32_t v = 0; v < 5; ++v) {
                if (v != omitted)
                    tetra[n++] = v;
            }
            for (int skip = 0; skip < 4; ++skip) {
                std::uint32_t f[3];
                int k = 0;
                for (int i = 0; i < 4; ++i) {
                    if (i != skip)
                        f[k++] = tetra[i];
                }
                add_triangle(*obj, f[0], f[1], f[2]);
            }
        }
        return obj;
    }

    case object_type::DUO_CYLINDER:
        return create_hyper_plane(
            [](const scomplex &a) {
                return Vector4f(glm::cos(a.real()), glm::sin(a.real()), glm::cos(a.imag()), glm::sin(a.imag()));
            },
            100, Vector2f(glm::pi<float>(), glm::pi<float>()),
            [](const scomplex &a) { return rainbow(a.real(), a.imag()); }, true, true);

    case object_type::SPHERE_4D:
        return create_cube(
            [](const Vector3f &a) {
                const float c1 = glm::cos(a.x() + glm::pi<float>() / 2);
                const float s1 = glm::sin(a.x() + glm::pi<float>() / 2);
                const float c2 = glm::cos(a.y() + glm::pi<float>() / 2);
                const float s2 = glm::sin(a.y() + glm::pi<float>() / 2);
                const float c3 = glm::cos(a.z());
                const float s3 = glm::sin(a.z());
                return Vector4f(c1, s1 * c2, s1 * s2 * c3, s1 * s2 * s3);
            },
            9, Vector3f(glm::pi<float>() / 2, glm::pi<float>() / 2, glm::pi<float>()),
            identity_color_3d, /*wrap_u=*/false, /*wrap_v=*/false, /*wrap_w=*/true);

    case object_type::PLANE_4D:
        // A flat patch: its parameter domain is not periodic (wrapping it would
        // glue two opposite edges and stretch a seam across the surface).
        return create_hyper_plane(identity_4d, 20, Vector2f(5.0f, 5.0f), identity_color_2d, false, false);

    case object_type::SUPERPLANE_4D:
        return create_cube(identity_3d, 20, Vector3f(1.0f, 1.0f, 1.0f), identity_color_3d, false, false, false);

    case object_type::SPRING_4D: {
        const float r = 0.3f, R = 1.0f, C = 1.0f;
        return create_cube(
            [=](const Vector3f &a) {
                return Vector4f(R * glm::cos(a.z()) + r * glm::cos(a.x() + glm::pi<float>() / 2),
                                R * glm::sin(a.z()) + r * glm::sin(a.x() + glm::pi<float>() / 2) * glm::cos(a.y()),
                                R * glm::sin(a.z()) + r * glm::sin(a.x() + glm::pi<float>() / 2) * glm::sin(a.y()),
                                C * a.z());
            },
            20, Vector3f(glm::pi<float>() / 2, glm::pi<float>(), 10.0f), identity_color_3d,
            /*wrap_u=*/true, /*wrap_v=*/true, /*wrap_w=*/false);
    }

    case object_type::CYLINDER_PRISM: {
        // (u, v) in {-1, 0, 1} x {-1, 0, 1} grid of rings, each ring a circle.
        const int rings = 3;
        const int res = 20;
        auto obj = std::make_shared<mesh_data>();
        obj->dim = 4;
        for (int i = 0; i < rings; ++i) {
            for (int j = 0; j < rings; ++j) {
                for (int k = 0; k < res; ++k) {
                    const double angle = k * 1.0 / res * glm::two_pi<double>();
                    add_vertex(*obj, {static_cast<float>(i - 1), static_cast<float>(j - 1),
                                      static_cast<float>(glm::cos(angle)), static_cast<float>(glm::sin(angle))},
                               rainbow(static_cast<float>(k) / res,
                                       (static_cast<float>(i + j) + 0.5f) / (2.0f * (rings - 1) + 1.0f)));
                }
            }
        }
        emit_structured_grid<3>(*obj, 0, {rings, rings, res}, {false, false, true}, true);
        return obj;
    }

    case object_type::HYPER_PARABOLA:
        return create_hyper_plane(
            [](const scomplex &a) {
                const scomplex u1 = a * a;
                return Vector4f(a.real(), a.imag(), u1.real(), u1.imag());
            },
            100, Vector2f(2.0f, 2.0f), identity_color_2d, false, false);

    case object_type::HYPER_EX:
        return create_hyper_plane(
            [](const scomplex &a) {
                const scomplex u1 = std::exp(a);
                return Vector4f(a.real(), a.imag(), u1.real(), u1.imag());
            },
            100, Vector2f(glm::two_pi<float>() * 2, glm::two_pi<float>() * 2), identity_color_2d, false, false);
    }
    return nullptr;
}

std::shared_ptr<mesh_data> create_hyperplane_floor(float half_size, int cells, float height)
{
    auto obj = std::make_shared<mesh_data>();
    obj->dim = 4;
    const int n = cells + 1;
    const float span = static_cast<float>(cells);
    obj->vertices.reserve(static_cast<std::size_t>(n) * n * n * 7);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int k = 0; k < n; ++k) {
                const float x = (i / span * 2.0f - 1.0f) * half_size;
                const float z = (j / span * 2.0f - 1.0f) * half_size;
                const float w = (k / span * 2.0f - 1.0f) * half_size;
                // Subdued, but tinted along the 4th axis so the position in w
                // is readable while exploring.
                const Vector3f tint = rainbow(k / span, 0.5f);
                const Vector3f color(0.14f + 0.30f * tint.x(), 0.16f + 0.30f * tint.y(), 0.21f + 0.34f * tint.z());
                add_vertex(*obj, {x, height, z, w}, color);
            }
        }
    }
    emit_structured_grid<3>(*obj, 0, {n, n, n}, {false, false, false}, true);
    return obj;
}

std::shared_ptr<mesh_data> create_obj_fromfile(const std::filesystem::path &filename, bool is_4D)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return nullptr;

    auto obj = std::make_shared<mesh_data>();
    obj->dim = is_4D ? 4 : 3;

    std::size_t n_points = 0;
    file >> n_points;
    for (std::size_t i = 0; i < n_points; ++i) {
        float p[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        for (int k = 0; k < obj->dim; ++k)
            file >> p[k];
        Vector3f color(1.0f, 1.0f, 1.0f);
        file >> color.x() >> color.y() >> color.z();
        if (is_4D)
            add_vertex(*obj, {p[0], p[1], p[2], p[3]}, color);
        else
            add_vertex(*obj, {p[0], p[1], p[2]}, color);
    }

    std::size_t n_faces = 0;
    file >> n_faces;
    for (std::size_t i = 0; i < n_faces; ++i) {
        std::uint32_t a = 0, b = 0, c = 0;
        file >> a >> b >> c;
        add_triangle(*obj, a, b, c);
        // Keep the wireframe consistent with the file's topology.
        add_line(*obj, a, b);
        add_line(*obj, b, c);
        add_line(*obj, c, a);
    }
    return obj;
}
