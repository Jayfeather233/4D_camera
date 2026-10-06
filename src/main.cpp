#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include <fmt/core.h>

#include "objects/mesh_data.hpp"
#include "objects/object.hpp"
#include "opengl/camera3D.hpp"
#include "opengl/camera4D.hpp"
#include "opengl/mesh.hpp"
#include "opengl/shader.hpp"
#include "overlay.hpp"
#include "scene.hpp"

namespace {

// Framebuffer size in pixels, kept up to date by the resize callback.
int g_width = 800;
int g_height = 600;
ogl::Camera3D g_cam3d;
ogl::Camera4D g_cam4d;
ogl::Camera4DFree g_cam4d_free;
bool g_free_camera = false;

struct Options {
    ogl::RenderMode mode = ogl::RenderMode::Wireframe;
    std::string scene_id = "arms";
    bool single_shape = false;
    object_type shape = object_type::CUBE_4D;
    bool show_help = false;
    bool list_shapes = false;
    bool list_scenes = false;
    bool hud = true;
    /// Fragment order: the projected 3D depth by default (the original
    /// behaviour); --depth4d switches to the real 4D distance. P toggles.
    bool depth4d = false;
    /// -1: use the monitor refresh rate (120 when unknown), 0: unlimited.
    int target_fps = -1;
};

void framebuffer_size_callback(GLFWwindow *, int width, int height)
{
    g_width = width;
    g_height = height;
}

void scroll_callback(GLFWwindow *, double, double yoffset) { g_cam3d.ProcessMouseScroll(static_cast<float>(yoffset)); }

void print_usage()
{
    std::cout << "4DCam - render 4D objects with a 4D camera\n\n"
                 "usage: 4DCam [options]\n\n"
                 "options:\n"
                 "  --scene <name>   start on a scene (see --list-scenes, default: arms)\n"
                 "  --shape <name>   show a single shape instead of a scene (see --list-shapes)\n"
                 "  --mode <name>    render mode: wireframe, solid, solid-wireframe, triangle-wireframe\n"
                 "  --fps <n>        frame limit; 0 = unlimited, default = screen refresh rate (or 120)\n"
                 "  --no-hud         start with the overlay hidden\n"
                 "  --depth4d        order fragments by the real 4D distance instead of the\n"
                 "                   projected 3D depth, which is the default (P toggles)\n"
                 "  --list-scenes    print the available scenes and exit\n"
                 "  --list-shapes    print the available shapes and exit\n"
                 "  -h, --help       print this help and exit\n\n"
                 "keys:\n"
                 "  TAB / SHIFT+TAB  next / previous scene\n"
                 "  1 wireframe  2 solid  3 solid+wireframe  4 triangle-wireframe   F cycle\n"
                 "  V                toggle the 3D reference layer (always wireframe)\n"
                 "  H                toggle this overlay\n"
                 "  P                depth: projected 3D (default) / 4D distance\n"
                 "  W A S D Q E      rotate the 4D camera        Z X   move the 4D camera\n"
                 "  I J K L          rotate the 3D camera        N M   move the 3D camera\n"
                 "  R                reset the cameras of the current scene\n"
                 "  ESC              quit\n\n"
                 "the free exploration scene replaces the camera keys above with:\n"
                 "  W/S forward/back   A/D strafe   Q/E 4D depth   N/M zoom\n"
                 "  SHIFT+CTRL+W/S    up / down (leaves the floor hyperplane)\n"
                 "  I/K, J/L, U/O     look around (SHIFT: the three 4D rotation planes)\n";
}

std::string render_mode_cli_name(ogl::RenderMode mode)
{
    switch (mode) {
    case ogl::RenderMode::Wireframe: return "wireframe";
    case ogl::RenderMode::Solid: return "solid";
    case ogl::RenderMode::SolidWireframe: return "solid-wireframe";
    case ogl::RenderMode::TriangleWireframe: return "triangle-wireframe";
    }
    return "wireframe";
}

bool parse_render_mode(std::string_view name, ogl::RenderMode &out)
{
    const ogl::RenderMode modes[] = {ogl::RenderMode::Wireframe, ogl::RenderMode::Solid,
                                     ogl::RenderMode::SolidWireframe, ogl::RenderMode::TriangleWireframe};
    for (ogl::RenderMode mode : modes) {
        if (name == render_mode_cli_name(mode)) {
            out = mode;
            return true;
        }
    }
    return false;
}

bool parse_options(int argc, char **argv, Options &opt)
{
    for (int i = 1; i < argc; ++i) {
        const std::string_view arg = argv[i];
        auto value = [&](std::string_view what) -> const char * {
            if (i + 1 >= argc) {
                fmt::print(stderr, "error: {} expects a value\n", what);
                return nullptr;
            }
            return argv[++i];
        };
        if (arg == "-h" || arg == "--help") {
            opt.show_help = true;
        }
        else if (arg == "--list-shapes") {
            opt.list_shapes = true;
        }
        else if (arg == "--list-scenes") {
            opt.list_scenes = true;
        }
        else if (arg == "--no-hud") {
            opt.hud = false;
        }
        else if (arg == "--depth3d") {
            opt.depth4d = false;
        }
        else if (arg == "--depth4d") {
            opt.depth4d = true;
        }
        else if (arg == "--scene") {
            const char *v = value(arg);
            if (!v || !app::find_scene(v)) {
                fmt::print(stderr, "error: unknown scene '{}'\n", v ? v : "");
                return false;
            }
            opt.scene_id = v;
        }
        else if (arg == "--mode") {
            const char *v = value(arg);
            if (!v || !parse_render_mode(v, opt.mode)) {
                fmt::print(stderr, "error: unknown render mode '{}'\n", v ? v : "");
                return false;
            }
        }
        else if (arg == "--shape") {
            const char *v = value(arg);
            if (!v || !object_type_from_name(v, opt.shape)) {
                fmt::print(stderr, "error: unknown shape '{}'\n", v ? v : "");
                return false;
            }
            opt.single_shape = true;
        }
        else if (arg == "--fps") {
            const char *v = value(arg);
            if (!v)
                return false;
            opt.target_fps = std::max(0, std::atoi(v));
        }
        else {
            fmt::print(stderr, "error: unknown option '{}'\n", arg);
            return false;
        }
    }
    return true;
}

void print_shapes()
{
    std::cout << "shapes:\n";
    for (object_type tp : all_object_types())
        std::cout << "  " << object_type_name(tp) << '\n';
}

void print_scenes()
{
    std::cout << "scenes:\n";
    for (const app::SceneInfo &scene : app::scene_list())
        std::cout << "  " << scene.id << " - " << scene.title << '\n';
}

/// Refresh rate of the monitor the window is on; 0 when it cannot be determined.
int monitor_refresh_rate(GLFWwindow *window)
{
    GLFWmonitor *monitor = glfwGetWindowMonitor(window);
    if (monitor == nullptr)
        monitor = glfwGetPrimaryMonitor();
    if (monitor == nullptr)
        return 0;
    const GLFWvidmode *video_mode = glfwGetVideoMode(monitor);
    return video_mode != nullptr ? video_mode->refreshRate : 0;
}

void process_input(GLFWwindow *window, float delta_time, Options &opt, bool &hud_dirty, bool &hud_visible,
                   app::Scene &scene, std::size_t &scene_index, const std::vector<app::SceneInfo> &scenes,
                   const std::function<void(std::size_t)> &load_scene)
{
    ogl::RenderMode &mode = opt.mode;
    const auto pressed = [window](int key) { return glfwGetKey(window, key) == GLFW_PRESS; };
    const auto just_pressed = [](bool now, bool &was) {
        const bool edge = now && !was;
        was = now;
        return edge;
    };

    if (pressed(GLFW_KEY_ESCAPE))
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // --- scene selector -----------------------------------------------------
    static bool tab_was_pressed = false;
    if (just_pressed(pressed(GLFW_KEY_TAB), tab_was_pressed)) {
        const bool backwards = pressed(GLFW_KEY_LEFT_SHIFT) || pressed(GLFW_KEY_RIGHT_SHIFT);
        const std::size_t count = scenes.size();
        scene_index = backwards ? (scene_index + count - 1) % count : (scene_index + 1) % count;
        load_scene(scene_index);
    }

    // --- render mode --------------------------------------------------------
    const int mode_keys[] = {GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4};
    const ogl::RenderMode modes[] = {ogl::RenderMode::Wireframe, ogl::RenderMode::Solid,
                                     ogl::RenderMode::SolidWireframe, ogl::RenderMode::TriangleWireframe};
    for (int i = 0; i < 4; ++i) {
        if (pressed(mode_keys[i]) && mode != modes[i]) {
            mode = modes[i];
            hud_dirty = true;
        }
    }
    static bool cycle_was_pressed = false;
    if (just_pressed(pressed(GLFW_KEY_F), cycle_was_pressed)) {
        mode = ogl::next_render_mode(mode);
        hud_dirty = true;
    }

    static bool layer_was_pressed = false;
    if (just_pressed(pressed(GLFW_KEY_V), layer_was_pressed)) {
        scene.setObjects3DVisible(!scene.objects3DVisible());
        hud_dirty = true;
    }

    static bool hud_was_pressed = false;
    if (just_pressed(pressed(GLFW_KEY_H), hud_was_pressed))
        hud_visible = !hud_visible;

    // Depth source: the real 4D distance or the projected 3D depth.
    static bool depth_was_pressed = false;
    if (just_pressed(pressed(GLFW_KEY_P), depth_was_pressed)) {
        opt.depth4d = !opt.depth4d;
        scene.setDepthBy4D(opt.depth4d);
        hud_dirty = true;
    }

    const app::SceneInfo &info = scenes[scene_index];
    if (pressed(GLFW_KEY_R)) {
        if (info.free_camera)
            g_cam4d_free.reset(info.spawn, info.spawn_tilt);
        g_cam4d.reset(info.yaw1, info.yaw2, info.pitch, info.distance4);
        g_cam3d.reset(info.yaw3, 0.0f, info.distance3);
    }

    if (info.free_camera) {
        // --- free 4D camera ------------------------------------------------
        const float step = 6.0f * delta_time;
        // SHIFT+CTRL swaps W/S to the one axis the walker keys leave out: the
        // camera's up axis, so all four axes of 4D space can be used.
        const bool vertical = pressed(GLFW_KEY_LEFT_SHIFT) || pressed(GLFW_KEY_RIGHT_SHIFT);
        const bool vertical_ctrl = pressed(GLFW_KEY_LEFT_CONTROL) || pressed(GLFW_KEY_RIGHT_CONTROL);
        const bool fly = vertical && vertical_ctrl;
        if (pressed(GLFW_KEY_W))
            g_cam4d_free.move(fly ? ogl::Camera4DFree::Axis::Up : ogl::Camera4DFree::Axis::Forward, step);
        if (pressed(GLFW_KEY_S))
            g_cam4d_free.move(fly ? ogl::Camera4DFree::Axis::Up : ogl::Camera4DFree::Axis::Forward, -step);
        if (pressed(GLFW_KEY_A))
            g_cam4d_free.move(ogl::Camera4DFree::Axis::Right, -step);
        if (pressed(GLFW_KEY_D))
            g_cam4d_free.move(ogl::Camera4DFree::Axis::Right, step);
        if (pressed(GLFW_KEY_Q))
            g_cam4d_free.move(ogl::Camera4DFree::Axis::Depth, -step);
        if (pressed(GLFW_KEY_E))
            g_cam4d_free.move(ogl::Camera4DFree::Axis::Depth, step);
        if (pressed(GLFW_KEY_N))
            g_cam4d_free.addZoom(-0.5f * delta_time);
        if (pressed(GLFW_KEY_M))
            g_cam4d_free.addZoom(0.5f * delta_time);

        const bool four_d = vertical;
        const float turn = 1.6f * delta_time;
        // Without SHIFT: the three rotations that look like ordinary 3D looking
        // around. With SHIFT: the three rotations that involve the 4th axis.
        const auto plane = [&](ogl::Camera4DFree::Plane plain, ogl::Camera4DFree::Plane four_d_plane) {
            return four_d ? four_d_plane : plain;
        };
        if (pressed(GLFW_KEY_I))
            g_cam4d_free.rotate(plane(ogl::Camera4DFree::Plane::YZ, ogl::Camera4DFree::Plane::YW), turn);
        if (pressed(GLFW_KEY_K))
            g_cam4d_free.rotate(plane(ogl::Camera4DFree::Plane::YZ, ogl::Camera4DFree::Plane::YW), -turn);
        if (pressed(GLFW_KEY_J))
            g_cam4d_free.rotate(plane(ogl::Camera4DFree::Plane::XZ, ogl::Camera4DFree::Plane::XW), -turn);
        if (pressed(GLFW_KEY_L))
            g_cam4d_free.rotate(plane(ogl::Camera4DFree::Plane::XZ, ogl::Camera4DFree::Plane::XW), turn);
        if (pressed(GLFW_KEY_U))
            g_cam4d_free.rotate(plane(ogl::Camera4DFree::Plane::XY, ogl::Camera4DFree::Plane::ZW), -turn);
        if (pressed(GLFW_KEY_O))
            g_cam4d_free.rotate(plane(ogl::Camera4DFree::Plane::XY, ogl::Camera4DFree::Plane::ZW), turn);
        return;
    }

    // --- 4D camera ----------------------------------------------------------
    if (pressed(GLFW_KEY_W))
        g_cam4d.ProcessKeyboard(ogl::Camera4D_Movement::FORWARD_4D, delta_time);
    if (pressed(GLFW_KEY_S))
        g_cam4d.ProcessKeyboard(ogl::Camera4D_Movement::BACKWARD_4D, delta_time);
    if (pressed(GLFW_KEY_A))
        g_cam4d.ProcessKeyboard(ogl::Camera4D_Movement::LEFT1, delta_time);
    if (pressed(GLFW_KEY_D))
        g_cam4d.ProcessKeyboard(ogl::Camera4D_Movement::RIGHT1, delta_time);
    if (pressed(GLFW_KEY_Q))
        g_cam4d.ProcessKeyboard(ogl::Camera4D_Movement::LEFT2, delta_time);
    if (pressed(GLFW_KEY_E))
        g_cam4d.ProcessKeyboard(ogl::Camera4D_Movement::RIGHT2, delta_time);
    if (pressed(GLFW_KEY_Z))
        g_cam4d.addDistance(10.0f * delta_time);
    if (pressed(GLFW_KEY_X))
        g_cam4d.addDistance(-10.0f * delta_time);

    // --- 3D camera ----------------------------------------------------------
    const float turn = 1000.0f * delta_time;
    if (pressed(GLFW_KEY_I))
        g_cam3d.ProcessMouseMovement(0.0f, turn);
    if (pressed(GLFW_KEY_K))
        g_cam3d.ProcessMouseMovement(0.0f, -turn);
    if (pressed(GLFW_KEY_J))
        g_cam3d.ProcessMouseMovement(-turn, 0.0f);
    if (pressed(GLFW_KEY_L))
        g_cam3d.ProcessMouseMovement(turn, 0.0f);
    if (pressed(GLFW_KEY_N))
        g_cam3d.addDistance(10.0f * delta_time);
    if (pressed(GLFW_KEY_M))
        g_cam3d.addDistance(-10.0f * delta_time);
}

/// Paces the loop to `target_fps`; sleeps most of the wait, spins the last bit.
void wait_for_next_frame(double &next_frame, double frame_time, int target_fps)
{
    if (target_fps <= 0 || frame_time <= 0.0)
        return;
    next_frame += frame_time;
    const double now = glfwGetTime();
    if (next_frame > now) {
        const double remaining = next_frame - now;
        if (remaining > 0.0015)
            std::this_thread::sleep_for(std::chrono::duration<double>(remaining - 0.001));
        while (glfwGetTime() < next_frame) {
            // spin out the last millisecond for a stable frame time
        }
    }
    else if (now - next_frame > 0.25) {
        next_frame = now; // after a stall (resize, breakpoint): resync, do not catch up
    }
}

} // namespace

int main(int argc, char **argv)
{
    Options opt;
    if (!parse_options(argc, argv, opt)) {
        print_usage();
        return EXIT_FAILURE;
    }
    if (opt.show_help) {
        print_usage();
        return EXIT_SUCCESS;
    }
    if (opt.list_shapes) {
        print_shapes();
        return EXIT_SUCCESS;
    }
    if (opt.list_scenes) {
        print_scenes();
        return EXIT_SUCCESS;
    }

    glfwSetErrorCallback([](int code, const char *description) {
        fmt::print(stderr, "glfw error {}: {}\n", code, description);
    });
    if (!glfwInit()) {
        fmt::print(stderr, "error: failed to initialize GLFW\n");
        return EXIT_FAILURE;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(g_width, g_height, "4DCam", nullptr, nullptr);
    if (window == nullptr) {
        fmt::print(stderr, "error: failed to create a GLFW window\n");
        glfwTerminate();
        return EXIT_FAILURE;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Vsync is deliberately off: the limiter below paces the loop, which also
    // avoids the blocking swap that can freeze the window while resizing under
    // WSLg / remote desktops.
    glfwSwapInterval(0);

    const GLenum err = glewInit();
    if (err != GLEW_OK) {
        fmt::print(stderr, "error: failed to initialize GLEW: {}\n",
                   reinterpret_cast<const char *>(glewGetErrorString(err)));
        glfwDestroyWindow(window);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.06f, 1.0f);

    const std::vector<app::SceneInfo> &scenes = app::scene_list();
    std::vector<std::string> scene_names;
    scene_names.reserve(scenes.size());
    for (const app::SceneInfo &info : scenes)
        scene_names.emplace_back(info.title);

    // Frame limit: screen refresh rate, or 120 Hz when it cannot be queried.
    int target_fps = opt.target_fps;
    if (target_fps < 0) {
        const int refresh = monitor_refresh_rate(window);
        target_fps = refresh > 0 ? refresh : 120;
        std::cout << "frame limit: " << target_fps << " fps"
                  << (refresh > 0 ? " (monitor refresh rate)" : " (refresh rate unknown, using 120)") << '\n';
    }
    else if (target_fps == 0) {
        std::cout << "frame limit: unlimited\n";
    }
    const double frame_time = target_fps > 0 ? 1.0 / target_fps : 0.0;

    auto scene = std::make_unique<app::Scene>();
    std::size_t scene_index = 0;
    if (opt.single_shape) {
        app::build_shape_scene(*scene, opt.shape);
        scene->setDepthBy4D(opt.depth4d);
        g_cam4d.reset(30.0f, -25.0f, 20.0f, 2.4f);
        g_cam3d.reset(-90.0f, 0.0f, 3.0f);
    }
    else {
        for (std::size_t i = 0; i < scenes.size(); ++i) {
            if (scenes[i].id == opt.scene_id) {
                scene_index = i;
                break;
            }
        }
        scenes[scene_index].build(*scene);
        scene->setDepthBy4D(opt.depth4d);
        g_cam4d.reset(scenes[scene_index].yaw1, scenes[scene_index].yaw2, scenes[scene_index].pitch,
                      scenes[scene_index].distance4);
        g_cam3d.reset(scenes[scene_index].yaw3, 0.0f, scenes[scene_index].distance3);
        g_free_camera = scenes[scene_index].free_camera;
        if (scenes[scene_index].free_camera)
            g_cam4d_free.reset(scenes[scene_index].spawn, scenes[scene_index].spawn_tilt);
        if (scenes[scene_index].preferred_mode != nullptr)
            parse_render_mode(scenes[scene_index].preferred_mode, opt.mode);
    }

    const auto dir = ogl::defaultShaderDir();
    const auto program3D = ogl::programFromFiles(dir, "simple3D.vs", "simple.fs");
    const auto program4D = ogl::programFromFiles(dir, "simple4D.vs", "simple.fs");

    app::TextOverlay overlay;
    overlay.setScreenSize(g_width, g_height);
    bool hud_visible = opt.hud;
    bool hud_dirty = true;
    double menu_until = 0.0;
    double last_hud_update = 0.0;

    // Rebuilds the world for another scene; kept small so switching is instant.
    const auto load_scene = [&](std::size_t index) {
        scene = std::make_unique<app::Scene>();
        if (opt.single_shape)
            app::build_shape_scene(*scene, opt.shape);
        else
            scenes[index].build(*scene);
        scene->setDepthBy4D(opt.depth4d);
        if (!opt.single_shape) {
            const app::SceneInfo &next = scenes[index];
            g_cam4d.reset(next.yaw1, next.yaw2, next.pitch, next.distance4);
            g_cam3d.reset(next.yaw3, 0.0f, next.distance3);
            g_free_camera = next.free_camera;
            if (next.free_camera)
                g_cam4d_free.reset(next.spawn, next.spawn_tilt);
            if (next.preferred_mode != nullptr)
                parse_render_mode(next.preferred_mode, opt.mode);
        }
        menu_until = glfwGetTime() + 2.5;
        hud_dirty = true;
        const std::string title =
            opt.single_shape ? std::string(object_type_name(opt.shape)) : std::string(scenes[index].title);
        glfwSetWindowTitle(window, fmt::format("4DCam - {}", title).c_str());
    };
    const std::string initial_title =
        opt.single_shape ? std::string(object_type_name(opt.shape)) : std::string(scenes[scene_index].title);
    glfwSetWindowTitle(window, fmt::format("4DCam - {}", initial_title).c_str());

    double last_time = glfwGetTime();
    double next_frame = last_time;
    int viewport_width = 0;
    int viewport_height = 0;
    float fps = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        const double now = glfwGetTime();
        const float delta_time = std::min(static_cast<float>(now - last_time), 0.1f);
        last_time = now;
        fps = delta_time > 0.0f ? 0.9f * fps + 0.1f * (1.0f / delta_time) : fps;

        process_input(window, delta_time, opt, hud_dirty, hud_visible, *scene, scene_index, scenes, load_scene);
        glfwPollEvents();

        // Minimized or hidden: do not render, just wait for events.
        if (g_width <= 0 || g_height <= 0 || glfwGetWindowAttrib(window, GLFW_ICONIFIED)) {
            glfwWaitEventsTimeout(0.1);
            last_time = glfwGetTime();
            next_frame = last_time;
            continue;
        }

        if (g_width != viewport_width || g_height != viewport_height) {
            glViewport(0, 0, g_width, g_height);
            viewport_width = g_width;
            viewport_height = g_height;
            overlay.setScreenSize(g_width, g_height);
            hud_dirty = true;
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (g_free_camera) {
            scene->drawFree(program3D, program4D, g_cam4d_free, opt.mode, static_cast<std::uint32_t>(g_width),
                            static_cast<std::uint32_t>(g_height));
        }
        else {
            scene->draw(program3D, program4D, g_cam3d, g_cam4d, opt.mode, static_cast<std::uint32_t>(g_width),
                        static_cast<std::uint32_t>(g_height));
        }

        if (hud_visible) {
            // The FPS number changes constantly, so refresh the text a few times
            // per second instead of every frame.
            if (hud_dirty || now - last_hud_update > 0.25) {
                app::HudState state;
                state.scene_name =
                    opt.single_shape ? object_type_name(opt.shape) : std::string(scenes[scene_index].title);
                state.scene_index = static_cast<int>(scene_index) + 1;
                state.scene_count = static_cast<int>(scenes.size());
                state.mode = opt.mode;
                state.objects3DVisible = scene->objects3DVisible();
                state.depthBy4D = scene->depthBy4D();
                state.menu_open = now < menu_until;
                state.fps = fps;
                state.scene_names = &scene_names;
                if (scenes[scene_index].help != nullptr && !opt.single_shape)
                    state.help = scenes[scene_index].help;
                overlay.setLines(app::build_hud_lines(state));
                hud_dirty = false;
                last_hud_update = now;
            }
            overlay.draw(program3D);
        }

        glfwSwapBuffers(window);
        wait_for_next_frame(next_frame, frame_time, target_fps);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
