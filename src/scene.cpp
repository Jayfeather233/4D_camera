#include "scene.hpp"

#include <iterator>
#include <utility>

#include <glm/glm.hpp>

#include "objects/mesh_data.hpp"

namespace app {

void Scene::accumulate(const ogl::MeshPtr &mesh, ogl::RenderMode mode)
{
    if (!mesh)
        return;
    const auto lines = static_cast<std::size_t>(mesh->line_count()) / 2;
    const auto triangles = static_cast<std::size_t>(mesh->triangle_count()) / 3;
    const bool use_triangles = mesh->has_triangles();

    switch (mode) {
    case ogl::RenderMode::Wireframe:
        stats_.lines += lines;
        break;
    case ogl::RenderMode::Solid:
        if (use_triangles)
            stats_.triangles += triangles;
        else
            stats_.lines += lines;
        break;
    case ogl::RenderMode::SolidWireframe:
        stats_.triangles += triangles;
        stats_.lines += lines;
        break;
    case ogl::RenderMode::TriangleWireframe:
        if (use_triangles)
            stats_.triangles += triangles;
        else
            stats_.lines += lines;
        break;
    }
}

bool Scene::depthRange(const glm::mat4 &view_rot, const glm::vec4 &camera_pos, glm::vec2 &range) const
{
    // q.w is a linear function of the world position, so the extremes over an
    // object are reached at a corner of its bounding box.
    const glm::vec4 axis(view_rot[0][3], view_rot[1][3], view_rot[2][3], view_rot[3][3]);
    float lo = 0.0f, hi = 0.0f;
    bool any = false;
    const auto include = [&](float w) {
        if (!any) {
            lo = hi = w;
            any = true;
        }
        lo = std::min(lo, w);
        hi = std::max(hi, w);
    };

    for (const auto &obj : objects4D_) {
        glm::vec4 mn, mx;
        if (!obj->boundingBox(mn, mx))
            continue;
        const glm::mat4 &rot = obj->rotate();
        const glm::vec4 &off = obj->offset();
        for (int corner = 0; corner < 16; ++corner) {
            const glm::vec4 local((corner & 1) ? mx.x : mn.x, (corner & 2) ? mx.y : mn.y, (corner & 4) ? mx.z : mn.z,
                                  (corner & 8) ? mx.w : mn.w);
            include(glm::dot(axis, rot * local + off - camera_pos));
        }
    }
    if (objects3D_visible_) {
        for (const auto &obj : objects3D_) {
            glm::vec3 mn, mx;
            if (!obj->boundingBox(mn, mx))
                continue;
            const glm::mat3 &rot = obj->rotate();
            const glm::vec3 &off = obj->offset();
            for (int corner = 0; corner < 8; ++corner) {
                const glm::vec3 local((corner & 1) ? mx.x : mn.x, (corner & 2) ? mx.y : mn.y,
                                      (corner & 4) ? mx.z : mn.z);
                include(glm::dot(axis, glm::vec4(rot * local + off, 0.0f) - camera_pos));
            }
        }
    }
    if (!any)
        return false;
    if (hi - lo < 1e-4f)
        return false;
    range = glm::vec2(lo, hi);
    return true;
}

void Scene::draw(const std::shared_ptr<ogl::Program> &program3D, const std::shared_ptr<ogl::Program> &program4D,
                 const ogl::Camera3D &cam3d, const ogl::Camera4D &cam4d, ogl::RenderMode mode, std::uint32_t width,
                 std::uint32_t height)
{
    stats_ = RenderStats{};
    stats_.meshes = meshes_.size();

    // Fragment order follows the real 4D distance (see simple4D.vs).
    glm::vec2 depth_range;
    const bool has_depth_range = depth_by_4d_ && depthRange(cam4d.GetRotationMat(), cam4d.Position, depth_range);

    if (objects3D_visible_ && !objects3D_.empty()) {
        program3D->bind();
        program3D->setUniform("depth_bias", 0.0f);
        cam3d.SetUniform(program3D, width, height);
        cam4d.SetUniform3D(program3D);
        if (has_depth_range)
            cam4d.SetDepthRange(program3D, depth_range);
        for (const auto &obj : objects3D_) {
            obj->setUniform(program3D);
            // The 3D layer is a reference only: always wireframe, never solid.
            obj->draw(ogl::RenderMode::Wireframe, program3D);
            ++stats_.draw_calls;
            accumulate(obj->mesh(), ogl::RenderMode::Wireframe);
        }
        program3D->release();
    }

    if (!objects4D_.empty()) {
        program4D->bind();
        program4D->setUniform("depth_bias", 0.0f);
        cam3d.SetUniform(program4D, width, height);
        cam4d.SetUniform(program4D);
        if (has_depth_range)
            cam4d.SetDepthRange(program4D, depth_range);
        for (const auto &obj : objects4D_) {
            obj->setUniform(program4D);
            obj->draw(mode, program4D);
            ++stats_.draw_calls;
            accumulate(obj->mesh(), mode);
        }
        program4D->release();
    }
}

void Scene::drawFree(const std::shared_ptr<ogl::Program> &program3D, const std::shared_ptr<ogl::Program> &program4D,
                     const ogl::Camera4DFree &camera, ogl::RenderMode mode, std::uint32_t width, std::uint32_t height)
{
    stats_ = RenderStats{};
    stats_.meshes = meshes_.size();

    // The 4D perspective shader clips geometry that is behind the camera.
    glEnable(GL_CLIP_DISTANCE0);
    if (!objects4D_.empty()) {
        program4D->bind();
        program4D->setUniform("depth_bias", 0.0f);
        camera.setUniform(program4D, width, height);
        if (!depth_by_4d_)
            program4D->setUniform("view4D_depth_near", 0.0f); // fall back to the projected depth
        for (const auto &obj : objects4D_) {
            obj->setUniform(program4D);
            obj->draw(mode, program4D);
            ++stats_.draw_calls;
            accumulate(obj->mesh(), mode);
        }
        program4D->release();
    }
    glDisable(GL_CLIP_DISTANCE0);

    if (objects3D_visible_ && !objects3D_.empty()) {
        program3D->bind();
        program3D->setUniform("depth_bias", 0.0f);
        camera.setUniform3D(program3D, width, height);
        if (!depth_by_4d_)
            program3D->setUniform("view4D_depth_near", 0.0f);
        for (const auto &obj : objects3D_) {
            obj->setUniform(program3D);
            obj->draw(ogl::RenderMode::Wireframe, program3D);
            ++stats_.draw_calls;
            accumulate(obj->mesh(), ogl::RenderMode::Wireframe);
        }
        program3D->release();
    }
}

namespace {

/// Adds a shape centered and scaled to `radius`, then moved by `offset`.
/// A radius <= 0 keeps the shape's own scale (used by shapes whose extent is
/// dominated by a blow up far from the interesting region, e.g. the exponential).
std::shared_ptr<Object4D> add_shape(Scene &scene, object_type tp, float radius,
                                   const glm::vec4 &offset = glm::vec4(0.0f))
{
    auto obj = std::make_shared<Object4D>(scene.mesh(create_obj_base(tp)));
    if (radius > 0.0f)
        obj->normalize(radius);
    obj->addOffset(offset);
    scene.add(obj);
    return obj;
}

/// The wireframe box that gives the viewer a 3D size reference.
void add_reference(Scene &scene, float scale = 1.0f)
{
    auto reference = std::make_shared<Object3D>(scene.mesh(create_obj_base(object_type::CUBE_3D)));
    reference->addScale(scale);
    scene.add(reference);
}

/// 24 tesseracts, six per 4D axis, plus the reference box.
void build_arms(Scene &scene)
{
    const ogl::MeshPtr cube = scene.mesh(create_obj_base(object_type::CUBE_4D));
    constexpr int kAxes = 4;
    constexpr int kPerAxis = 6;
    for (int axis = 0; axis < kAxes; ++axis) {
        for (int i = 0; i < kPerAxis; ++i) {
            auto obj = std::make_shared<Object4D>(cube);
            glm::vec4 offset(0.0f);
            offset[axis] = static_cast<float>(i) - (kPerAxis - 1) * 0.5f;
            obj->setOffset(offset);
            scene.add(obj);
        }
    }
    add_reference(scene);
}

void build_tesseract(Scene &scene)
{
    add_shape(scene, object_type::CUBE_4D, 1.0f);
    add_reference(scene);
}

/// Four tesseracts spread along the 4th axis: rotating 4D space makes them line
/// up as a tunnel of cubes of different sizes.
void build_tunnel(Scene &scene)
{
    const float w[] = {-2.0f, -0.7f, 0.7f, 2.0f};
    for (float offset : w)
        add_shape(scene, object_type::CUBE_4D, 0.5f, glm::vec4(0.0f, 0.0f, 0.0f, offset));
    add_reference(scene, 0.5f);
}

/// A small 4D cross with a tesseract sitting in it.
void build_axes(Scene &scene)
{
    add_shape(scene, object_type::COORD_4D, 1.0f);
    add_shape(scene, object_type::CUBE_4D, 0.6f);
    add_reference(scene, 0.5f);
}

void build_cell5(Scene &scene)
{
    add_shape(scene, object_type::CELL5, 0.9f);
    add_reference(scene, 0.6f);
}

void build_glome(Scene &scene)
{
    add_shape(scene, object_type::SPHERE_4D, 0.8f);
}

void build_helix(Scene &scene)
{
    add_shape(scene, object_type::SPRING_4D, 1.0f);
    add_reference(scene, 0.5f);
}

/// The Clifford torus: a flat torus living in 4D.
void build_torus(Scene &scene)
{
    add_shape(scene, object_type::DUO_CYLINDER, 0.95f);
    add_reference(scene, 0.5f);
}

void build_plane(Scene &scene)
{
    add_shape(scene, object_type::PLANE_4D, 1.0f);
    add_reference(scene, 0.5f);
}

/// The graph of z = u^2 in the complex plane, lifted into 4D.
void build_paraboloid(Scene &scene)
{
    add_shape(scene, object_type::HYPER_PARABOLA, 0.9f);
    add_reference(scene, 0.5f);
}

/**
 * The explorable map: a 4D floor plus landmarks to look at.
 *
 * The floor is the hyperplane y = -2, so the walkable space is the 3D space
 * spanned by x, z and w. Landmarks sit above it at different places, including
 * different positions along the 4th axis: some of them only become visible once
 * you move or turn in 4D.
 */
void build_explore(Scene &scene)
{
    constexpr float kHalf = 6.0f;   // the map is 12 x 12 x 12 in x, z, w
    constexpr int kCells = 15;
    constexpr float kFloorY = -2.0f;
    scene.add(std::make_shared<Object4D>(scene.mesh(create_hyperplane_floor(kHalf, kCells, kFloorY))));

    struct Landmark {
        object_type type;
        glm::vec4 position;
        float radius;
    };
    const Landmark landmarks[] = {
        {object_type::CUBE_4D, {0.0f, 0.0f, 0.0f, 0.0f}, 1.5f},
        {object_type::SPHERE_4D, {-3.4f, 0.0f, 1.6f, -1.2f}, 1.1f},
        {object_type::CELL5, {3.2f, 0.0f, -1.4f, 1.4f}, 1.1f},
        {object_type::DUO_CYLINDER, {-2.0f, 0.0f, -3.0f, 3.6f}, 1.0f},
        {object_type::SPRING_4D, {3.0f, 0.0f, 3.0f, -3.4f}, 1.2f},
        {object_type::PLANE_4D, {0.0f, 0.0f, -4.2f, 0.0f}, 1.3f},
        {object_type::HYPER_PARABOLA, {-4.2f, 0.0f, 0.0f, 4.4f}, 1.0f},
        {object_type::CUBE_4D, {1.8f, 0.0f, 0.0f, 5.6f}, 1.3f},
    };
    for (const Landmark &landmark : landmarks) {
        auto obj = std::make_shared<Object4D>(scene.mesh(create_obj_base(landmark.type)));
        obj->normalize(landmark.radius);
        obj->addOffset(landmark.position);
        scene.add(obj);
    }
}

/// Four different shapes in a row: tesseract, 5 cell, 3 sphere, spring.
void build_gallery(Scene &scene)
{
    const object_type types[] = {object_type::CUBE_4D, object_type::CELL5, object_type::SPHERE_4D,
                                 object_type::DUO_CYLINDER};
    const int count = static_cast<int>(std::size(types));
    for (int i = 0; i < count; ++i) {
        const float x = (static_cast<float>(i) - (count - 1) * 0.5f) * 1.6f;
        add_shape(scene, types[i], 0.5f, glm::vec4(x, 0.0f, 0.0f, 0.0f));
    }
    add_reference(scene, 0.5f);
}

} // namespace

const std::vector<SceneInfo> &scene_list()
{
    static const std::vector<SceneInfo> scenes = {
        {"arms", "ARMS - 24 TESSERACTS", 4.3f, 30.0f, -25.0f, 20.0f, 3.2f, -90.0f, build_arms},
        {"tesseract", "TESSERACT", 2.6f, 30.0f, -25.0f, 20.0f, 3.0f, -90.0f, build_tesseract},
        {"tunnel", "4D TUNNEL", 3.4f, 35.0f, -30.0f, 22.0f, 3.0f, -90.0f, build_tunnel},
        {"axes", "4D AXES", 2.4f, 28.0f, -22.0f, 18.0f, 2.6f, -90.0f, build_axes},
        {"cell5", "5 CELL SIMPLEX", 2.4f, 30.0f, -25.0f, 20.0f, 3.0f, -90.0f, build_cell5},
        {"glome", "3 SPHERE (GLOME)", 2.4f, 30.0f, -25.0f, 20.0f, 3.4f, -90.0f, build_glome},
        {"helix", "SPRING / HELIX", 2.4f, 30.0f, -25.0f, 20.0f, 2.0f, -90.0f, build_helix},
        {"torus", "CLIFFORD TORUS", 2.6f, 32.0f, -28.0f, 20.0f, 2.8f, -90.0f, build_torus},
        {"plane", "FLAT PLANE IN 4D", 2.6f, 30.0f, -25.0f, 20.0f, 2.8f, -90.0f, build_plane},
        {"paraboloid", "HYPER PARABOLOID", 2.6f, 28.0f, -22.0f, 18.0f, 2.8f, -90.0f, build_paraboloid},
        {"gallery", "GALLERY - 4 SHAPES", 2.8f, 22.0f, -18.0f, 14.0f, 3.4f, -90.0f, build_gallery},
        {"explore", "FREE EXPLORATION", 2.6f, 0.0f, 0.0f, 0.0f, 3.0f, -90.0f, build_explore, true,
         {0.0f, 0.0f, 0.0f, -13.0f}, 11.0f, 0.42f,
         "W/S FORWARD BACK  A/D STRAFE  Q/E 4D DEPTH  N/M ZOOM|"
         "SHIFT+CTRL+W/S UP DOWN   R RESET   H HUD|"
         "IJKLUO LOOK  SHIFT+IJKLUO 4D ROTATION", "wireframe"},
    };
    return scenes;
}

const SceneInfo *find_scene(std::string_view id)
{
    for (const SceneInfo &scene : scene_list()) {
        if (scene.id == id)
            return &scene;
    }
    return nullptr;
}

void build_shape_scene(Scene &scene, object_type tp)
{
    const auto data = create_obj_base(tp);
    if (!data)
        return;
    if (data->dim == 4) {
        auto obj = std::make_shared<Object4D>(scene.mesh(data));
        if (tp != object_type::COORD_4D)
            obj->normalize(1.0f);
        scene.add(obj);
    }
    else {
        auto obj = std::make_shared<Object3D>(scene.mesh(data));
        scene.add(obj);
    }
    add_reference(scene, 0.5f);
}

} // namespace app
