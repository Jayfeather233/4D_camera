#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include "objects/object.hpp"
#include "opengl/camera3D.hpp"
#include "opengl/camera4D.hpp"
#include "opengl/mesh.hpp"
#include "opengl/shader.hpp"

namespace app {

struct RenderStats {
    int draw_calls = 0;
    std::size_t triangles = 0;
    std::size_t lines = 0;
    std::size_t meshes = 0;
};

/**
 * Owns the GPU meshes and the objects of the current scene. Objects that were
 * built from the same mesh_data share one set of GPU buffers.
 *
 * The 3D layer is a wireframe size reference: it is always drawn as lines, no
 * matter which render mode is active.
 */
class Scene {
public:
    /// Registers (and on first use uploads) the GPU buffers of `data`.
    ogl::MeshPtr mesh(const std::shared_ptr<const mesh_data> &data) { return meshes_.get(data); }

    void add(const std::shared_ptr<Object3D> &obj) { objects3D_.push_back(obj); }
    void add(const std::shared_ptr<Object4D> &obj) { objects4D_.push_back(obj); }

    const std::vector<std::shared_ptr<Object3D>> &objects3D() const { return objects3D_; }
    const std::vector<std::shared_ptr<Object4D>> &objects4D() const { return objects4D_; }

    /// Draws with the free 4D camera instead of the orbiting cameras.
    void drawFree(const std::shared_ptr<ogl::Program> &program3D, const std::shared_ptr<ogl::Program> &program4D,
                  const ogl::Camera4DFree &camera, ogl::RenderMode mode, std::uint32_t width, std::uint32_t height);

    /// The reference layer can be hidden (the 4D objects are then unobstructed).
    void setObjects3DVisible(bool visible) { objects3D_visible_ = visible; }
    bool objects3DVisible() const { return objects3D_visible_; }

    /// Fragment order: by default the projected 3D depth (what a plain 3D
    /// renderer would do with the projected scene). Turning this on orders by
    /// the real 4D distance instead, which is closer to 4D but makes surfaces
    /// that are almost equally far away in 4D compete for the same pixels.
    void setDepthBy4D(bool enabled) { depth_by_4d_ = enabled; }
    bool depthBy4D() const { return depth_by_4d_; }

    const RenderStats &stats() const { return stats_; }

    void draw(const std::shared_ptr<ogl::Program> &program3D, const std::shared_ptr<ogl::Program> &program4D,
              const ogl::Camera3D &cam3d, const ogl::Camera4D &cam4d, ogl::RenderMode mode, std::uint32_t width,
              std::uint32_t height);

private:
    void accumulate(const ogl::MeshPtr &mesh, ogl::RenderMode mode);
    /// Range of q.w (the 4D distance along the view axis) covered by the scene,
    /// used to order fragments by the real 4D distance. Returns false if empty.
    bool depthRange(const glm::mat4 &view_rot, const glm::vec4 &camera_pos, glm::vec2 &range) const;

    ogl::MeshLibrary meshes_;
    std::vector<std::shared_ptr<Object3D>> objects3D_;
    std::vector<std::shared_ptr<Object4D>> objects4D_;
    RenderStats stats_;
    bool objects3D_visible_ = true;
    bool depth_by_4d_ = false;
};

/// A named little scene plus the camera setup that frames it well.
struct SceneInfo {
    std::string_view id;    ///< name used on the command line
    std::string_view title; ///< name shown in the HUD
    float distance4 = 2.4f;
    float yaw1 = 30.0f;
    float yaw2 = -25.0f;
    float pitch = 20.0f;
    float distance3 = 3.0f;
    /// The 3D camera looks along -Z at yaw = -90, which puts the 4D x axis on
    /// the screen's horizontal axis.
    float yaw3 = -90.0f;
    void (*build)(Scene &) = nullptr;
    /// When set, the scene is explored with a free 4D camera instead.
    bool free_camera = false;
    /// Start position of the free camera, the tilt of its view and its zoom.
    glm::vec4 spawn{0.0f, 0.0f, 0.0f, -12.0f};
    float spawn_tilt = 0.0f;
    float spawn_zoom = 0.35f;
    /// Extra overlay lines describing scene specific controls ('|' separated).
    const char *help = nullptr;
    /// Render mode this scene looks best in; nullptr keeps the current one.
    const char *preferred_mode = nullptr;
};

const std::vector<SceneInfo> &scene_list();
const SceneInfo *find_scene(std::string_view id);

/// Builds a single centered shape (used by --shape).
void build_shape_scene(Scene &scene, object_type tp);

} // namespace app
