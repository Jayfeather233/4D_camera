#include "opengl/camera3D.hpp"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {
constexpr float kSpeed = 2.5f;
constexpr float kSensitivity = 0.1f;
constexpr float kZoom = 45.0f;
constexpr float kNearPlane = 0.05f;
constexpr float kFarPlane = 500.0f;
} // namespace

ogl::Camera3D::Camera3D(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(kSpeed), MouseSensitivity(kSensitivity), Zoom(kZoom)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    distance = 3.0f;
    updateCameraVectors();
}

void ogl::Camera3D::reset(float yaw, float pitch, float dist)
{
    Yaw = yaw;
    Pitch = std::clamp(pitch, -89.0f, 89.0f);
    distance = dist;
    updateCameraVectors();
}

glm::mat4 ogl::Camera3D::GetViewMatrix() const { return glm::lookAt(Position, Position + Front, Up); }

glm::mat4 ogl::Camera3D::GetProjectionMat(uint32_t SCR_WIDTH, uint32_t SCR_HEIGHT) const
{
    const float aspect = SCR_HEIGHT == 0 ? 1.0f : static_cast<float>(SCR_WIDTH) / static_cast<float>(SCR_HEIGHT);
    return glm::perspective(glm::radians(Zoom), aspect, kNearPlane, kFarPlane);
}

void ogl::Camera3D::SetUniform(const std::shared_ptr<ogl::Program> &p, uint32_t SCR_WIDTH, uint32_t SCR_HEIGHT) const
{
    p->setUniform("view3D", GetViewMatrix());
    p->setUniform("projection", GetProjectionMat(SCR_WIDTH, SCR_HEIGHT));
}

void ogl::Camera3D::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
        Pitch = std::clamp(Pitch, -89.0f, 89.0f);

    updateCameraVectors();
}

void ogl::Camera3D::ProcessMouseScroll(float yoffset)
{
    Zoom = std::clamp(Zoom - yoffset, 1.0f, 60.0f);
    updateCameraVectors();
}

void ogl::Camera3D::addDistance(float u)
{
    distance += u;
    updateCameraVectors();
}

void ogl::Camera3D::updateCameraVectors()
{
    const float yaw = glm::radians(Yaw);
    const float pitch = glm::radians(Pitch);
    Front = glm::vec3(cos(yaw) * cos(pitch), sin(pitch), sin(yaw) * cos(pitch));

    // Right/Up: guard against a degenerate cross product when looking straight up/down.
    const glm::vec3 right = glm::cross(Front, WorldUp);
    Right = glm::length(right) > 1e-6f ? glm::normalize(right) : glm::vec3(1.0f, 0.0f, 0.0f);
    Up = glm::normalize(glm::cross(Right, Front));

    Position = -Front * distance;
}
