/**
 * Headless preview tool.
 *
 * Renders a scene into an offscreen OpenGL context (EGL surfaceless, no window
 * needed) and writes one PPM image per render mode. Useful for checking geometry
 * and for machines without a display.
 *
 *   cmake -S . -B build -D4DCAM_BUILD_TOOLS=ON
 *   ./build/4dcam_preview --scene arms --mode all --out preview
 *   ./build/4dcam_preview --shape spring4d --mode solid --hud --out preview
 *   convert preview/arms-solid.ppm preview/arms-solid.png
 */
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/glew.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/core.h>
#include <glm/glm.hpp>

#include "objects/mesh_data.hpp"
#include "opengl/camera3D.hpp"
#include "opengl/camera4D.hpp"
#include "opengl/mesh.hpp"
#include "opengl/shader.hpp"
#include "overlay.hpp"
#include "scene.hpp"

namespace fs = std::filesystem;

namespace {

struct Options {
    fs::path out = "preview";
    std::string scene_id = "arms";
    bool single_shape = false;
    object_type shape = object_type::CUBE_4D;
    std::vector<ogl::RenderMode> modes;
    int width = 800;
    int height = 600;
    int samples = 4;
    bool show3d = true;
    bool hud = false;
    bool hud_menu = false;
    bool dump = false;
    bool depth4d = false;
    // Camera overrides; when unset the scene's own framing is used.
    std::optional<float> yaw1, yaw2, pitch, dist4, yaw3, pitch3, dist3;
    // Free camera overrides for the free exploration scenes.
    std::optional<glm::vec4> free_pos;
    std::optional<float> free_zoom;
    struct FreeRotation {
        std::string plane;
        float degrees = 0.0f;
    };
    std::vector<FreeRotation> free_rotations;
};

void print_usage()
{
    std::cout << "4dcam_preview - render a 4D scene to PPM files without a window\n\n"
                 "usage: 4dcam_preview [options]\n\n"
                 "options:\n"
                 "  --out <dir>       output directory (default: preview)\n"
                 "  --scene <name>    scene to render (see --list-scenes, default: arms)\n"
                 "  --shape <name>    render a single shape instead of a scene\n"
                 "  --mode <list>     wireframe|solid|solid-wireframe|triangle-wireframe|all (default all)\n"
                 "  --size WxH        image size (default 800x600)\n"
                 "  --samples N       MSAA samples, 1 disables (default 4)\n"
                 "  --hud             draw the text overlay\n"
                 "  --hud-menu        draw the overlay with the scene selector open\n"
                 "  --no-3d           hide the 3D reference layer\n"
                 "  --free-pos x,y,z,w    free camera position (exploration scenes)\n"
                 "  --free-rot p:deg      rotate the free camera, p = xy|xz|xw|yz|yw|zw (repeatable)\n"
                 "  --free-zoom <f>       free camera orthographic zoom\n"
                 "  --dump            print the objects of the scene and their transforms\n"
                 "  --depth4d         order fragments by the real 4D distance (default: projected 3D depth)\n"
                 "  --yaw1/--yaw2/--pitch <deg>   4D camera rotation (default: the scene's)\n"
                 "  --dist4 <f>       4D camera distance (default: the scene's)\n"
                 "  --yaw3/--pitch3 <deg>         3D camera rotation (default: 0/0)\n"
                 "  --dist3 <f>       3D camera distance (default: the scene's)\n"
                 "  --list-scenes     print the available scenes and exit\n"
                 "  --list-shapes     print the available shapes and exit\n"
                 "  -h, --help        print this help\n\n"
                 "Images are written as binary PPM; convert with e.g. `convert x.ppm x.png`.\n";
}

std::string mode_filename(ogl::RenderMode mode)
{
    switch (mode) {
    case ogl::RenderMode::Wireframe: return "wireframe";
    case ogl::RenderMode::Solid: return "solid";
    case ogl::RenderMode::SolidWireframe: return "solid-wireframe";
    case ogl::RenderMode::TriangleWireframe: return "triangle-wireframe";
    }
    return "unknown";
}

bool parse_mode_list(std::string_view list, std::vector<ogl::RenderMode> &out)
{
    const ogl::RenderMode all[] = {ogl::RenderMode::Wireframe, ogl::RenderMode::Solid, ogl::RenderMode::SolidWireframe,
                                   ogl::RenderMode::TriangleWireframe};
    if (list == "all") {
        out.assign(std::begin(all), std::end(all));
        return true;
    }
    std::size_t start = 0;
    while (start <= list.size()) {
        const std::size_t comma = list.find(',', start);
        const std::string_view name =
            list.substr(start, comma == std::string_view::npos ? std::string_view::npos : comma - start);
        bool found = false;
        for (ogl::RenderMode mode : all) {
            if (name == mode_filename(mode)) {
                out.push_back(mode);
                found = true;
            }
        }
        if (!found) {
            fmt::print(stderr, "error: unknown render mode '{}'\n", name);
            return false;
        }
        if (comma == std::string_view::npos)
            break;
        start = comma + 1;
    }
    return true;
}

bool parse_options(int argc, char **argv, Options &opt)
{
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        auto value = [&]() -> const char * {
            if (i + 1 >= argc) {
                fmt::print(stderr, "error: {} expects a value\n", arg);
                return nullptr;
            }
            return argv[++i];
        };
        const auto to_float = [&](std::optional<float> &dst) {
            if (const char *v = value())
                dst = std::strtof(v, nullptr);
        };
        if (arg == "-h" || arg == "--help") {
            print_usage();
            std::exit(0);
        }
        else if (arg == "--list-shapes") {
            for (object_type tp : all_object_types())
                std::cout << object_type_name(tp) << '\n';
            std::exit(0);
        }
        else if (arg == "--list-scenes") {
            for (const app::SceneInfo &scene : app::scene_list())
                std::cout << scene.id << " - " << scene.title << '\n';
            std::exit(0);
        }
        else if (arg == "--out") {
            if (const char *v = value())
                opt.out = v;
        }
        else if (arg == "--scene") {
            const char *v = value();
            if (!v || !app::find_scene(v)) {
                fmt::print(stderr, "error: unknown scene '{}'\n", v ? v : "");
                return false;
            }
            opt.scene_id = v;
            opt.single_shape = false;
        }
        else if (arg == "--shape") {
            const char *v = value();
            if (!v || !object_type_from_name(v, opt.shape)) {
                fmt::print(stderr, "error: unknown shape '{}'\n", v ? v : "");
                return false;
            }
            opt.single_shape = true;
        }
        else if (arg == "--mode") {
            const char *v = value();
            if (!v || !parse_mode_list(v, opt.modes))
                return false;
        }
        else if (arg == "--size") {
            const char *v = value();
            if (!v || std::sscanf(v, "%dx%d", &opt.width, &opt.height) != 2) {
                fmt::print(stderr, "error: --size expects WxH\n");
                return false;
            }
        }
        else if (arg == "--samples") {
            if (const char *v = value())
                opt.samples = std::max(1, std::atoi(v));
        }
        else if (arg == "--hud") {
            opt.hud = true;
        }
        else if (arg == "--hud-menu") {
            opt.hud = true;
            opt.hud_menu = true;
        }
        else if (arg == "--no-3d") {
            opt.show3d = false;
        }
        else if (arg == "--free-pos") {
            const char *v = value();
            float x = 0, y = 0, z = 0, w = 0;
            if (!v || std::sscanf(v, "%f,%f,%f,%f", &x, &y, &z, &w) != 4) {
                fmt::print(stderr, "error: --free-pos expects x,y,z,w\n");
                return false;
            }
            opt.free_pos = glm::vec4(x, y, z, w);
        }
        else if (arg == "--free-rot") {
            const char *v = value();
            char plane[8] = {};
            float degrees = 0.0f;
            if (!v || std::sscanf(v, "%7[^:]:%f", plane, &degrees) != 2) {
                fmt::print(stderr, "error: --free-rot expects <plane>:<degrees>\n");
                return false;
            }
            opt.free_rotations.push_back({plane, degrees});
        }
        else if (arg == "--free-zoom") {
            to_float(opt.free_zoom);
        }
        else if (arg == "--dump") {
            opt.dump = true;
        }
        else if (arg == "--depth4d") {
            opt.depth4d = true;
        }
        else if (arg == "--depth3d") {
            opt.depth4d = false; // the default, accepted for symmetry
        }
        else if (arg == "--yaw1") {
            to_float(opt.yaw1);
        }
        else if (arg == "--yaw2") {
            to_float(opt.yaw2);
        }
        else if (arg == "--pitch") {
            to_float(opt.pitch);
        }
        else if (arg == "--dist4") {
            to_float(opt.dist4);
        }
        else if (arg == "--yaw3") {
            to_float(opt.yaw3);
        }
        else if (arg == "--pitch3") {
            to_float(opt.pitch3);
        }
        else if (arg == "--dist3") {
            to_float(opt.dist3);
        }
        else {
            fmt::print(stderr, "error: unknown option '{}'\n", arg);
            return false;
        }
    }
    if (opt.modes.empty())
        parse_mode_list("all", opt.modes);
    return true;
}

/// EGL surfaceless context: OpenGL without a window system.
EGLDisplay init_egl()
{
    auto get_platform_display =
        reinterpret_cast<PFNEGLGETPLATFORMDISPLAYEXTPROC>(eglGetProcAddress("eglGetPlatformDisplayEXT"));
    EGLDisplay display = EGL_NO_DISPLAY;
    if (get_platform_display)
        display = get_platform_display(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
    if (display == EGL_NO_DISPLAY)
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY)
        return EGL_NO_DISPLAY;

    EGLint major = 0, minor = 0;
    if (!eglInitialize(display, &major, &minor))
        return EGL_NO_DISPLAY;
    eglBindAPI(EGL_OPENGL_API);

    const EGLint config_attributes[] = {EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
                                        EGL_RED_SIZE,     8,                EGL_GREEN_SIZE,     8,
                                        EGL_BLUE_SIZE,    8,                EGL_DEPTH_SIZE,     24,
                                        EGL_NONE};
    EGLConfig config = nullptr;
    EGLint config_count = 0;
    if (!eglChooseConfig(display, config_attributes, &config, 1, &config_count) || config_count < 1)
        return EGL_NO_DISPLAY;

    const EGLint context_attributes[] = {EGL_CONTEXT_MAJOR_VERSION, 4,
                                         EGL_CONTEXT_MINOR_VERSION, 0,
                                         EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
                                         EGL_NONE};
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attributes);
    if (context == EGL_NO_CONTEXT)
        return EGL_NO_DISPLAY;
    if (!eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context))
        return EGL_NO_DISPLAY;
    return display;
}

/// Multisampled offscreen target plus a single sample target for readback.
class OffscreenTarget {
public:
    OffscreenTarget(int width, int height, int samples) : width_(width), height_(height), samples_(samples)
    {
        glGenFramebuffers(1, &msaa_fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo_);
        glGenRenderbuffers(1, &msaa_color_);
        glBindRenderbuffer(GL_RENDERBUFFER, msaa_color_);
        if (samples_ > 1)
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_RGBA8, width_, height_);
        else
            glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaa_color_);

        glGenRenderbuffers(1, &msaa_depth_);
        glBindRenderbuffer(GL_RENDERBUFFER, msaa_depth_);
        if (samples_ > 1)
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples_, GL_DEPTH_COMPONENT24, width_, height_);
        else
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, msaa_depth_);

        glGenFramebuffers(1, &resolve_fbo_);
        glBindFramebuffer(GL_FRAMEBUFFER, resolve_fbo_);
        glGenRenderbuffers(1, &resolve_color_);
        glBindRenderbuffer(GL_RENDERBUFFER, resolve_color_);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width_, height_);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, resolve_color_);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~OffscreenTarget()
    {
        glDeleteRenderbuffers(1, &resolve_color_);
        glDeleteFramebuffers(1, &resolve_fbo_);
        glDeleteRenderbuffers(1, &msaa_depth_);
        glDeleteRenderbuffers(1, &msaa_color_);
        glDeleteFramebuffers(1, &msaa_fbo_);
    }

    void begin()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, msaa_fbo_);
        glViewport(0, 0, width_, height_);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    std::vector<unsigned char> read()
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, msaa_fbo_);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolve_fbo_);
        glBlitFramebuffer(0, 0, width_, height_, 0, 0, width_, height_, GL_COLOR_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, resolve_fbo_);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        std::vector<unsigned char> pixels(static_cast<std::size_t>(width_) * height_ * 4);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width_, height_, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return pixels;
    }

private:
    int width_, height_, samples_;
    GLuint msaa_fbo_ = 0, msaa_color_ = 0, msaa_depth_ = 0;
    GLuint resolve_fbo_ = 0, resolve_color_ = 0;
};

bool write_ppm(const fs::path &path, int width, int height, const std::vector<unsigned char> &rgba)
{
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        fmt::print(stderr, "error: cannot write {}\n", path.string());
        return false;
    }
    out << "P6\n" << width << ' ' << height << "\n255\n";
    for (int y = height - 1; y >= 0; --y) { // OpenGL's origin is bottom left
        for (int x = 0; x < width; ++x)
            out.write(reinterpret_cast<const char *>(&rgba[(static_cast<std::size_t>(y) * width + x) * 4]), 3);
    }
    return true;
}

} // namespace

int main(int argc, char **argv)
{
    Options opt;
    if (!parse_options(argc, argv, opt))
        return 1;

    EGLDisplay display = init_egl();
    if (display == EGL_NO_DISPLAY) {
        fmt::print(stderr, "error: could not create an EGL OpenGL context (is libEGL/mesa available?)\n");
        return 1;
    }
    glewExperimental = GL_TRUE;
    glewInit(); // GLEW_ERROR_NO_GLX_DISPLAY is expected here, function pointers still resolve through EGL/libGL
    if (!glGenVertexArrays || !glCreateShader || !glBlitFramebuffer) {
        fmt::print(stderr, "error: OpenGL entry points are missing\n");
        return 1;
    }
    std::cout << "renderer: " << reinterpret_cast<const char *>(glGetString(GL_RENDERER)) << " | OpenGL "
              << reinterpret_cast<const char *>(glGetString(GL_VERSION)) << '\n';

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glClearColor(0.05f, 0.05f, 0.06f, 1.0f);

    const std::vector<app::SceneInfo> &scenes = app::scene_list();
    std::vector<std::string> scene_names;
    scene_names.reserve(scenes.size());
    for (const app::SceneInfo &info : scenes)
        scene_names.emplace_back(info.title);

    app::Scene scene;
    std::size_t scene_index = 0;
    std::string name;
    if (opt.single_shape) {
        app::build_shape_scene(scene, opt.shape);
        name = object_type_name(opt.shape);
    }
    else {
        for (std::size_t i = 0; i < scenes.size(); ++i) {
            if (scenes[i].id == opt.scene_id) {
                scene_index = i;
                break;
            }
        }
        scenes[scene_index].build(scene);
        name = scenes[scene_index].id;
    }
    scene.setObjects3DVisible(opt.show3d);
    scene.setDepthBy4D(opt.depth4d);

    // Handy while tuning a scene: show where every object ended up.
    if (opt.dump) {
        for (const auto &obj : scene.objects4D()) {
            const glm::vec4 &o = obj->offset();
            std::printf("  4D offset (%6.3f %6.3f %6.3f %6.3f)  scale %.4f\n", o.x, o.y, o.z, o.w,
                        obj->rotate()[0][0]);
        }
        for (const auto &obj : scene.objects3D()) {
            const glm::vec3 &o = obj->offset();
            std::printf("  3D offset (%6.3f %6.3f %6.3f)  scale %.4f\n", o.x, o.y, o.z, obj->rotate()[0][0]);
        }
    }

    const app::SceneInfo &info = scenes[scene_index];
    const bool use_scene_camera = !opt.single_shape;

    ogl::Camera3D cam3d;
    cam3d.reset(opt.yaw3.value_or(use_scene_camera ? info.yaw3 : -90.0f), opt.pitch3.value_or(0.0f),
                opt.dist3.value_or(use_scene_camera ? info.distance3 : 3.0f));
    ogl::Camera4D cam4d;
    cam4d.reset(opt.yaw1.value_or(use_scene_camera ? info.yaw1 : 30.0f),
                opt.yaw2.value_or(use_scene_camera ? info.yaw2 : -25.0f),
                opt.pitch.value_or(use_scene_camera ? info.pitch : 20.0f),
                opt.dist4.value_or(use_scene_camera ? info.distance4 : 2.4f));

    const auto dir = ogl::defaultShaderDir();
    const auto program3D = ogl::programFromFiles(dir, "simple3D.vs", "simple.fs");
    const auto program4D = ogl::programFromFiles(dir, "simple4D.vs", "simple.fs");

    // Free camera: start from the scene's spawn, then apply the overrides.
    ogl::Camera4DFree free_camera;
    free_camera.reset(info.free_camera ? info.spawn : glm::vec4(0.0f, 0.0f, 0.0f, -13.0f),
                      info.free_camera ? info.spawn_tilt : 0.0f);
    if (opt.free_pos)
        free_camera.reset(*opt.free_pos, info.spawn_tilt);
    if (opt.free_zoom)
        free_camera.addZoom(*opt.free_zoom - free_camera.zoom());
    for (const auto &rotation : opt.free_rotations) {
        const std::map<std::string, ogl::Camera4DFree::Plane> planes = {
            {"xy", ogl::Camera4DFree::Plane::XY}, {"xz", ogl::Camera4DFree::Plane::XZ},
            {"xw", ogl::Camera4DFree::Plane::XW}, {"yz", ogl::Camera4DFree::Plane::YZ},
            {"yw", ogl::Camera4DFree::Plane::YW}, {"zw", ogl::Camera4DFree::Plane::ZW}};
        const auto it = planes.find(rotation.plane);
        if (it == planes.end()) {
            fmt::print(stderr, "error: unknown rotation plane '{}'\n", rotation.plane);
            return 1;
        }
        free_camera.rotate(it->second, glm::radians(rotation.degrees));
    }

    app::TextOverlay overlay;
    overlay.setScreenSize(opt.width, opt.height);

    OffscreenTarget target(opt.width, opt.height, opt.samples);
    std::error_code ec;
    fs::create_directories(opt.out, ec);
    if (ec) {
        fmt::print(stderr, "error: cannot create {}\n", opt.out.string());
        return 1;
    }

    int failures = 0;
    for (ogl::RenderMode mode : opt.modes) {
        target.begin();
        if (info.free_camera)
            scene.drawFree(program3D, program4D, free_camera, mode, static_cast<std::uint32_t>(opt.width),
                           static_cast<std::uint32_t>(opt.height));
        else
            scene.draw(program3D, program4D, cam3d, cam4d, mode, static_cast<std::uint32_t>(opt.width),
                       static_cast<std::uint32_t>(opt.height));
        if (opt.hud) {
            app::HudState state;
            state.scene_name = use_scene_camera ? std::string(info.title) : name;
            state.scene_index = static_cast<int>(scene_index) + 1;
            state.scene_count = static_cast<int>(scenes.size());
            state.mode = mode;
            state.objects3DVisible = opt.show3d;
            state.depthBy4D = opt.depth4d;
            state.menu_open = opt.hud_menu;
            if (info.help != nullptr)
                state.help = info.help;
            state.fps = 60.0f;
            state.scene_names = &scene_names;
            overlay.setLines(app::build_hud_lines(state));
            overlay.draw(program3D);
        }
        const GLenum err = glGetError();
        if (err != GL_NO_ERROR)
            fmt::print(stderr, "warning: GL error 0x{:x} while drawing {}\n", err, ogl::render_mode_name(mode));

        const fs::path file = opt.out / fmt::format("{}-{}.ppm", name, mode_filename(mode));
        if (!write_ppm(file, opt.width, opt.height, target.read()))
            ++failures;
        else
            fmt::print("{:<18} {} ({} draws, {} tris, {} lines)\n", ogl::render_mode_name(mode), file.string(),
                       scene.stats().draw_calls, scene.stats().triangles, scene.stats().lines);
    }

    eglTerminate(display);
    return failures == 0 ? 0 : 1;
}
