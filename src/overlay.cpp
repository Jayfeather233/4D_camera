#include "overlay.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>

#include <fmt/core.h>
#include <glm/gtc/matrix_transform.hpp>

#include "objects/mesh_data.hpp"
#include "opengl/mesh.hpp"

namespace app {

namespace {

constexpr float kGlyphWidth = 4.0f;  // glyph box used by the font below
constexpr float kGlyphHeight = 6.0f;
constexpr float kAdvance = 6.0f;     // per character
constexpr float kLineHeight = 10.0f; // per text line

/**
 * Built in stroke font. Each glyph is a list of polylines inside a 4x6 box
 * (x to the right, y downwards); points are two digits, polylines are separated
 * by ';'. Only the characters the HUD needs are defined.
 */
const char *glyph_strokes(char c)
{
    switch (c) {
    case ' ': return "";
    case 'A': return "06 02 20 42 46;04 44";
    case 'B': return "00 06;00 30 41 42 33 03;03 33 44 45 36 06";
    case 'C': return "41 30 10 01 05 16 36 45";
    case 'D': return "00 20 42 44 26 06 00";
    case 'E': return "40 00 06 46;03 33";
    case 'F': return "40 00 06;03 33";
    case 'G': return "41 30 10 01 05 16 36 45 43 23";
    case 'H': return "00 06;40 46;03 43";
    case 'I': return "20 26;10 30;16 36";
    case 'J': return "40 45 36 16 05";
    case 'K': return "00 06;40 03;03 46";
    case 'L': return "00 06 46";
    case 'M': return "06 00 22 40 46";
    case 'N': return "06 00 46 40";
    case 'O': return "10 30 41 45 36 16 05 01 10";
    case 'P': return "06 00 30 41 42 33 03";
    case 'Q': return "10 30 41 45 36 16 05 01 10;24 46";
    case 'R': return "06 00 30 41 42 33 03;23 46";
    case 'S': return "41 30 10 01 02 13 33 44 45 36 16 05";
    case 'T': return "00 40;20 26";
    case 'U': return "00 05 16 36 45 40";
    case 'V': return "00 26 40";
    case 'W': return "00 16 23 36 40";
    case 'X': return "00 46;40 06";
    case 'Y': return "00 23 40;23 26";
    case 'Z': return "00 40 06 46";
    case '0': return "10 30 41 45 36 16 05 01 10;11 35";
    case '1': return "11 20 26;16 36";
    case '2': return "01 10 30 41 42 06 46";
    case '3': return "00 30 41 42 33 23;33 44 45 36 16 05";
    case '4': return "36 30 04 44";
    case '5': return "40 00 03 33 44 45 36 16 05";
    case '6': return "41 30 10 01 05 16 36 45 44 33 13 04";
    case '7': return "00 40 16";
    case '8': return "10 30 41 42 33 13 02 01 10;13 33 44 45 36 16 05 04 13";
    case '9': return "05 16 36 45 41 30 10 01 02 13 33 42";
    case '(': return "30 12 14 36";
    case ')': return "10 32 34 16";
    case '[': return "30 10 16 36";
    case ']': return "10 30 36 16";
    case '-': return "13 33";
    case '_': return "06 46";
    case '+': return "21 25;03 43";
    case '=': return "02 42;04 44";
    case '.': return "25 26";
    case ',': return "25 16";
    case ':': return "21 22;25 26";
    case ';': return "21 22;24 15";
    case '/': return "06 40";
    case '<': return "30 13 36";
    case '>': return "10 33 16";
    case '!': return "20 24";
    case '*': return "01 45;41 05";
    default: return "01 41 45 05 01"; // missing glyph: a box
    }
}

void add_glyph(mesh_data &m, char c, float x, float y, float scale, float r, float g, float b)
{
    const char *strokes = glyph_strokes(c);
    std::uint32_t previous = 0;
    bool has_previous = false;
    for (const char *p = strokes; *p != '\0';) {
        if (*p == ' ') {
            ++p;
            continue;
        }
        if (*p == ';') {
            has_previous = false;
            ++p;
            continue;
        }
        if (p[1] == '\0')
            break;
        const float gx = static_cast<float>(p[0] - '0');
        const float gy = static_cast<float>(p[1] - '0');
        p += 2;
        const std::uint32_t current =
            mesh_add_vertex(m, {x + gx * scale, y + gy * scale, 0.0f}, r, g, b);
        if (has_previous)
            mesh_add_line(m, previous, current);
        previous = current;
        has_previous = true;
    }
}

void add_quad(mesh_data &m, float x, float y, float width, float height, float r, float g, float b)
{
    const std::uint32_t a = mesh_add_vertex(m, {x, y, 0.0f}, r, g, b);
    const std::uint32_t c = mesh_add_vertex(m, {x + width, y, 0.0f}, r, g, b);
    const std::uint32_t d = mesh_add_vertex(m, {x + width, y + height, 0.0f}, r, g, b);
    const std::uint32_t e = mesh_add_vertex(m, {x, y + height, 0.0f}, r, g, b);
    mesh_add_triangle(m, a, c, d);
    mesh_add_triangle(m, a, d, e);
}

/// Splits "A|B" into {"A", "B"}.
std::vector<std::string> split_lines(const std::string &text)
{
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t bar = text.find('|', start);
        lines.push_back(text.substr(start, bar == std::string::npos ? std::string::npos : bar - start));
        if (bar == std::string::npos)
            break;
        start = bar + 1;
    }
    return lines;
}

std::string to_upper(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return text;
}

} // namespace

std::vector<std::string> build_hud_lines(const HudState &state)
{
    std::vector<std::string> lines;
    lines.push_back(to_upper(fmt::format("SCENE {}/{}  {}", state.scene_index, state.scene_count, state.scene_name)));
    lines.push_back(to_upper(fmt::format("MODE {}   FPS {:.0f}   DEPTH {}{}", ogl::render_mode_name(state.mode),
                                         state.fps, state.depthBy4D ? "4D" : "3D",
                                         state.objects3DVisible ? "" : "   3D LAYER OFF")));
    if (state.menu_open && state.scene_names != nullptr) {
        lines.push_back("SELECT SCENE  TAB NEXT  SHIFT+TAB PREV");
        for (std::size_t i = 0; i < state.scene_names->size(); ++i) {
            const bool current = static_cast<int>(i) + 1 == state.scene_index;
            lines.push_back(fmt::format("{} {}", current ? ">" : " ", to_upper((*state.scene_names)[i])));
        }
    }
    else {
        for (const std::string &line : split_lines(state.help))
            lines.push_back(to_upper(line));
        lines.push_back("TAB SCENE  1-4/F MODE  P DEPTH  V 3D  R RESET  H HUD");
    }
    return lines;
}

TextOverlay::TextOverlay(float pixel_scale) : scale_(pixel_scale)
{
    data_.dim = 3;
    rebuild();
}

void TextOverlay::setScreenSize(int width, int height)
{
    if (width == width_ && height == height_)
        return;
    width_ = width;
    height_ = height;
    rebuild();
}

void TextOverlay::setLines(const std::vector<std::string> &lines)
{
    if (lines == lines_)
        return;
    lines_ = lines;
    rebuild();
}

void TextOverlay::rebuild()
{
    built_ = lines_;
    data_ = mesh_data{};
    data_.dim = 3;

    std::size_t widest = 0;
    for (const std::string &line : lines_)
        widest = std::max(widest, line.size());

    // Pick a scale that is readable but always fits the window, whatever the
    // window size and the longest line are.
    const float margin = 8.0f;
    const float padding_units = 5.0f;
    const float preferred = std::clamp(static_cast<float>(height_) / 380.0f, 1.3f, 3.0f);
    const float units_wide = static_cast<float>(widest) * kAdvance + padding_units * 2.0f;
    const float units_high = static_cast<float>(lines_.size()) * kLineHeight + padding_units * 2.0f;
    const float fits_width = static_cast<float>(width_) - margin * 2.0f;
    const float fits_height = static_cast<float>(height_) - margin * 2.0f;
    float scale = preferred;
    if (units_wide > 0.0f)
        scale = std::min(scale, fits_width / units_wide);
    if (units_high > 0.0f)
        scale = std::min(scale, fits_height / units_high);
    scale_ = std::clamp(scale, 0.8f, 3.0f);

    const float padding = padding_units * scale_;
    const float line_height = kLineHeight * scale_;
    const float panel_width = static_cast<float>(widest) * kAdvance * scale_ + padding * 2.0f;
    const float panel_height = static_cast<float>(lines_.size()) * line_height + padding * 2.0f;

    // Opaque panel so the text stays readable on top of the scene.
    add_quad(data_, margin, margin, panel_width, panel_height, 0.02f, 0.02f, 0.035f);

    float y = margin + padding;
    for (const std::string &line : lines_) {
        float x = margin + padding;
        for (char c : line) {
            add_glyph(data_, c, x, y, scale_, 0.93f, 0.95f, 1.0f);
            x += kAdvance * scale_;
        }
        y += line_height;
    }

    if (!mesh_) {
        mesh_ = std::make_shared<ogl::Mesh>(std::make_shared<const mesh_data>(data_));
        object_ = std::make_unique<Object3D>(mesh_);
    }
    else {
        mesh_->update(data_);
    }
}

void TextOverlay::draw(const std::shared_ptr<ogl::Program> &program3D)
{
    if (!mesh_ || lines_.empty())
        return;
    if (built_ != lines_)
        rebuild();
    if (width_ <= 0 || height_ <= 0)
        return;

    glDisable(GL_DEPTH_TEST);
    program3D->bind();
    program3D->setUniform("depth_bias", 0.0f);
    program3D->setUniform("view3D", glm::mat4(1.0f));
    program3D->setUniform("projection",
                          glm::ortho(0.0f, static_cast<float>(width_), static_cast<float>(height_), 0.0f, -1.0f, 1.0f));
    object_->setUniform(program3D);
    object_->draw(ogl::RenderMode::SolidWireframe, program3D);
    program3D->release();
    glEnable(GL_DEPTH_TEST);
}

} // namespace app
