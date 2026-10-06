#pragma once

#include <memory>
#include <utility>

#include <glm/glm.hpp>

#include "opengl/shader.hpp"

namespace ogl {

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific
// input methods
enum Camera4D_Movement { FORWARD_4D, BACKWARD_4D, LEFT1, LEFT2, RIGHT1, RIGHT2 };

// A 4D camera: two yaw angles plus a pitch angle define the rotation of 4D space,
// `distance` moves the camera along its 4D front direction.
class Camera4D {
public:
    glm::vec4 Position;
    glm::vec4 Front;
    // euler Angles
    float Yaw1, Yaw2;
    float Pitch;
    float Zoom;
    float distance;

    Camera4D();

    /// Sets the three rotation angles and the 4D distance at once.
    void reset(float yaw1, float yaw2, float pitch, float dist);

    glm::mat4 GetRotationMat() const;

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    std::pair<glm::mat4, glm::vec4> GetViewMatrix() const;

    void SetUniform(const std::shared_ptr<ogl::Program> &p) const;

    /// The 4D distance depth is a linear map over the range the scene covers;
    /// the scene computes it each frame and uploads it with this.
    void SetDepthRange(const std::shared_ptr<ogl::Program> &p, const glm::vec2 &range) const;

    /// Uploads what the plain 3D shader needs to sort the 3D reference layer by
    /// the same 4D distance as the 4D objects.
    void SetUniform3D(const std::shared_ptr<ogl::Program> &p) const;

    void ProcessKeyboard(Camera4D_Movement direction, float deltaTime);
    void addDistance(float u);

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();

    mutable glm::mat4 rotation_;
    mutable bool rotation_dirty_ = true;
};

/**
 * A 4D camera you can fly around freely: a position in 4D plus a full 4D
 * orientation (4D space has 6 independent rotation planes).
 *
 * The view is a real 4D perspective: the camera looks along its 4th axis, so
 * moving along that axis makes the world grow and shrink, which is what makes
 * movement in the 4th dimension visible at all. Geometry behind the camera is
 * clipped by the shader (needs `view4D_near` > 0 and GL_CLIP_DISTANCE0).
 */
class Camera4DFree {
public:
    /// The 6 independent rotations of 4D space.
    enum class Plane { XY = 0, XZ, XW, YZ, YW, ZW };
    /// Movement axes. Right/Depth/Forward keep the camera on the y = const
    /// hyperplane (it walks on the map); Up breaks out of it, so the camera can
    /// leave the floor and use all four axes of 4D space.
    enum class Axis { Right = 0, Depth, Forward, Up };

    static constexpr int kPlaneCount = 6;
    static constexpr int kAxisCount = 4;

    Camera4DFree();

    /// Places the camera at `position` looking along +w with +y up, tilted down
    /// by `tilt_degrees` (a rotation in the up/4th-axis plane).
    void reset(const glm::vec4 &position, float tilt_degrees = 0.0f);

    void rotate(Plane plane, float radians);
    void move(Axis axis, float distance);
    void addZoom(float amount);

    const glm::vec4 &position() const { return position_; }
    glm::mat4 orientation() const { return orientation_; }
    float zoom() const { return zoom_; }
    float nearPlane() const { return near_plane_; }

    /// view4D_* plus the 3D part of the view (identity look-at + ortho zoom).
    void setUniform(const std::shared_ptr<Program> &program, uint32_t width, uint32_t height) const;
    /// Only the 3D part, for the plain 3D shader.
    void setUniform3D(const std::shared_ptr<Program> &program, uint32_t width, uint32_t height) const;

private:
    glm::vec4 axisVector(Axis axis) const;
    void orthonormalize();

    glm::vec4 position_{0.0f, 0.0f, 0.0f, -12.0f};
    /// Columns: right, up, depth, view (4th axis).
    glm::mat4 orientation_ = glm::mat4(1.0f);
    float near_plane_ = 0.12f;
    float zoom_ = 0.35f;
};

} // namespace ogl
