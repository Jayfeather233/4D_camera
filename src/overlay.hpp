#pragma once

#include <memory>
#include <string>
#include <vector>

#include "objects/object.hpp"
#include "opengl/mesh.hpp"
#include "opengl/shader.hpp"

namespace app {

/// Everything the HUD displays.
struct HudState {
    std::string scene_name;
    int scene_index = 0; // 1 based
    int scene_count = 0;
    ogl::RenderMode mode = ogl::RenderMode::Wireframe;
    bool objects3DVisible = true;
    bool depthBy4D = true;
    bool menu_open = false;
    float fps = 0.0f;
    /// Scene specific controls, '|' separated, shown above the generic ones.
    std::string help;
    /// Scene names shown when the selector menu is open.
    const std::vector<std::string> *scene_names = nullptr;
};

/// Builds the HUD text lines (upper case; the stroke font has no lower case).
std::vector<std::string> build_hud_lines(const HudState &state);

/**
 * Screen space text overlay drawn with a built in stroke font. The glyphs are
 * line segments, so the HUD reuses the scene shader and no font file, texture or
 * extra dependency is needed.
 */
class TextOverlay {
public:
    explicit TextOverlay(float pixel_scale = 2.0f);

    void setScreenSize(int width, int height);
    /// Rebuilds the geometry only when the text actually changed.
    void setLines(const std::vector<std::string> &lines);
    /// Draws over everything; leaves GL_DEPTH_TEST disabled state untouched.
    void draw(const std::shared_ptr<ogl::Program> &program3D);

private:
    void rebuild();

    float scale_;
    int width_ = 0;
    int height_ = 0;
    bool visible_ = true;
    std::vector<std::string> lines_;
    std::vector<std::string> built_;
    mesh_data data_;
    ogl::MeshPtr mesh_;
    std::unique_ptr<Object3D> object_;
};

} // namespace app
