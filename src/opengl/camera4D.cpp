#include "opengl/camera4D.hpp"

#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float kSensitivity = 30.0f;
}

ogl::Camera4D::Camera4D()
{
    Yaw1 = Yaw2 = 0.0f;
    Pitch = 0.0f;
    Zoom = 45.0f;
    distance = 2.0f;
    updateCameraVectors();
}

void ogl::Camera4D::reset(float yaw1, float yaw2, float pitch, float dist)
{
    Yaw1 = yaw1;
    Yaw2 = yaw2;
    Pitch = pitch;
    distance = dist;
    rotation_dirty_ = true;
    updateCameraVectors();
}

glm::mat4 ogl::Camera4D::GetRotationMat() const
{
    if (!rotation_dirty_)
        return rotation_;

    const float y1 = glm::radians(Yaw1);
    const float y2 = glm::radians(Yaw2);
    const float p = glm::radians(Pitch);

    // Rotations in the (x, w), (z, w) and (y, w) planes of 4D space.
    glm::mat4 rotYaw1 = glm::identity<glm::mat4>();
    rotYaw1[0][0] = cos(y1);
    rotYaw1[0][3] = sin(y1);
    rotYaw1[3][0] = -sin(y1);
    rotYaw1[3][3] = cos(y1);

    glm::mat4 rotYaw2 = glm::identity<glm::mat4>();
    rotYaw2[2][2] = cos(y2);
    rotYaw2[2][3] = sin(y2);
    rotYaw2[3][2] = -sin(y2);
    rotYaw2[3][3] = cos(y2);

    glm::mat4 rotPitch = glm::identity<glm::mat4>();
    rotPitch[1][1] = cos(p);
    rotPitch[1][3] = sin(p);
    rotPitch[3][1] = -sin(p);
    rotPitch[3][3] = cos(p);

    rotation_ = rotYaw1 * rotYaw2 * rotPitch;
    rotation_dirty_ = false;
    return rotation_;
}

std::pair<glm::mat4, glm::vec4> ogl::Camera4D::GetViewMatrix() const { return {GetRotationMat(), Position}; }

void ogl::Camera4D::SetUniform(const std::shared_ptr<ogl::Program> &p) const
{
    const auto view = GetViewMatrix();
    p->setUniform("view4D_rot", view.first);
    p->setUniform("view4D_off", view.second);
    // Orthographic 4D projection: no 4D perspective, no near clipping.
    p->setUniform("view4D_near", 0.0f);
    p->setUniform("view4D_depth_near", 0.0f);
    p->setUniform("view4D_depth_range", glm::vec2(0.0f)); // the scene fills this in
}

void ogl::Camera4D::SetDepthRange(const std::shared_ptr<ogl::Program> &p, const glm::vec2 &range) const
{
    p->setUniform("view4D_depth_range", range);
}

void ogl::Camera4D::SetUniform3D(const std::shared_ptr<ogl::Program> &p) const
{
    const auto view = GetViewMatrix();
    p->setUniform("view4D_rot", view.first);
    p->setUniform("view4D_off", view.second);
    p->setUniform("view4D_depth_near", 0.0f);
    p->setUniform("view4D_depth_range", glm::vec2(0.0f));
}

void ogl::Camera4D::addDistance(float u)
{
    distance += u;
    updateCameraVectors();
}

void ogl::Camera4D::updateCameraVectors()
{
    const glm::vec4 front(0.0, 0.0, 0.0, 1.0);
    Front = front * GetRotationMat();
    Position = Front * -distance;
}

void ogl::Camera4D::ProcessKeyboard(Camera4D_Movement direction, float deltaTime)
{
    const float velocity = kSensitivity * deltaTime;
    switch (direction) {
    case FORWARD_4D: Pitch += velocity; break;
    case BACKWARD_4D: Pitch -= velocity; break;
    case LEFT1: Yaw1 -= velocity; break;
    case RIGHT1: Yaw1 += velocity; break;
    case LEFT2: Yaw2 -= velocity; break;
    case RIGHT2: Yaw2 += velocity; break;
    }
    rotation_dirty_ = true;
    updateCameraVectors();
}

namespace {

/// Rotation of `radians` inside the plane spanned by axes i and j.
glm::mat4 plane_rotation(int i, int j, float radians)
{
    glm::mat4 m = glm::identity<glm::mat4>();
    const float c = glm::cos(radians);
    const float s = glm::sin(radians);
    m[i][i] = c;
    m[j][j] = c;
    m[j][i] = -s;
    m[i][j] = s;
    return m;
}

void plane_axes(ogl::Camera4DFree::Plane plane, int &i, int &j)
{
    switch (plane) {
    case ogl::Camera4DFree::Plane::XY: i = 0; j = 1; break;
    case ogl::Camera4DFree::Plane::XZ: i = 0; j = 2; break;
    case ogl::Camera4DFree::Plane::XW: i = 0; j = 3; break;
    case ogl::Camera4DFree::Plane::YZ: i = 1; j = 2; break;
    case ogl::Camera4DFree::Plane::YW: i = 1; j = 3; break;
    case ogl::Camera4DFree::Plane::ZW: i = 2; j = 3; break;
    }
}

} // namespace

ogl::Camera4DFree::Camera4DFree() { reset(position_, 0.0f); }

void ogl::Camera4DFree::reset(const glm::vec4 &position, float tilt_degrees)
{
    position_ = position;
    orientation_ = glm::identity<glm::mat4>();
    if (tilt_degrees != 0.0f)
        rotate(Plane::YW, glm::radians(tilt_degrees));
}

void ogl::Camera4DFree::rotate(Plane plane, float radians)
{
    int i = 0, j = 1;
    plane_axes(plane, i, j);
    // Post multiply: the rotation happens in the camera's own frame.
    orientation_ = orientation_ * plane_rotation(i, j, radians);
    orthonormalize();
}

void ogl::Camera4DFree::orthonormalize()
{
    // Repeated rotations drift; a Gram-Schmidt pass keeps the frame usable.
    glm::vec4 x(orientation_[0]);
    glm::vec4 y(orientation_[1]);
    glm::vec4 z(orientation_[2]);
    glm::vec4 w(orientation_[3]);
    x = glm::normalize(x);
    y = glm::normalize(y - x * glm::dot(x, y));
    z = glm::normalize(z - x * glm::dot(x, z) - y * glm::dot(y, z));
    w = glm::normalize(w - x * glm::dot(x, w) - y * glm::dot(y, w) - z * glm::dot(z, w));
    orientation_[0] = x;
    orientation_[1] = y;
    orientation_[2] = z;
    orientation_[3] = w;
}

glm::vec4 ogl::Camera4DFree::axisVector(Axis axis) const
{
    switch (axis) {
    case Axis::Right: return orientation_[0];
    case Axis::Depth: return orientation_[2];
    case Axis::Forward: return orientation_[3];
    case Axis::Up: return orientation_[1];
    }
    return orientation_[0];
}

void ogl::Camera4DFree::move(Axis axis, float distance)
{
    glm::vec4 direction = axisVector(axis);
    // Walking keeps the camera on the y = const hyperplane; flying up and down
    // is the explicit way out of it.
    if (axis != Axis::Up)
        direction.y = 0.0f;
    const float length = glm::length(direction);
    if (length < 1e-4f)
        return;
    position_ += direction * (distance / length);
}

void ogl::Camera4DFree::addZoom(float amount) { zoom_ = glm::clamp(zoom_ + amount, 0.02f, 8.0f); }

void ogl::Camera4DFree::setUniform3D(const std::shared_ptr<Program> &program, uint32_t width, uint32_t height) const
{
    // The 3D reference layer shares the 4D distance depth of the 4D layer.
    program->setUniform("view4D_rot", glm::transpose(orientation_));
    program->setUniform("view4D_off", position_);
    program->setUniform("view4D_depth_near", near_plane_);
    program->setUniform("view4D_depth_range", glm::vec2(0.0f));

    const float aspect = height == 0 ? 1.0f : static_cast<float>(width) / static_cast<float>(height);
    // A 3D camera very far away with a narrow field of view: the projection is
    // screen = f*q.xyz/(D*q.w - q.z) with a large D, so the 4D perspective
    // dominates, but moving along the projected depth axis still has a visible
    // effect, which is what makes that movement direction useful.
    const float eye = 60.0f;
    const float fov = 2.0f * glm::atan(zoom_ / eye);
    program->setUniform("view3D",
                        glm::lookAt(glm::vec3(0.0f, 0.0f, eye), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
    program->setUniform("projection", glm::perspective(fov, aspect, 0.01f, 10000.0f));
}

void ogl::Camera4DFree::setUniform(const std::shared_ptr<Program> &program, uint32_t width, uint32_t height) const
{
    // The shader computes view4D_rot * (position - view4D_off); the camera frame
    // maps camera space to world space, so the view rotation is its transpose.
    program->setUniform("view4D_rot", glm::transpose(orientation_));
    program->setUniform("view4D_off", position_);
    program->setUniform("view4D_near", near_plane_);
    program->setUniform("view4D_depth_near", near_plane_);
    program->setUniform("view4D_depth_range", glm::vec2(0.0f));
    setUniform3D(program, width, height);
}
