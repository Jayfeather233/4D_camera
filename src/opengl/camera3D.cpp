#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/glew.h>

#include "shader.hpp"
#include "camera3D.hpp"

static const float SPEED = 2.5f;
static const float SENSITIVITY = 0.1f;
static const float ZOOM = 45.0f;
// constructor with vectors
ogl::Camera3D::Camera3D(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM)
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    distance = 3.0f;
    updateCameraVectors();
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 ogl::Camera3D::GetViewMatrix() const
{
    return glm::lookAt(Position, Position + Front, Up);
}

glm::mat4 ogl::Camera3D::GetProjectionMat(uint32_t SCR_WIDTH, uint32_t SCR_HEIGHT) const
{
    return glm::perspective(glm::radians(Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.01f, 100.0f);
}

void ogl::Camera3D::SetUniform(std::shared_ptr<ogl::Program> p, uint32_t SCR_WIDTH, uint32_t SCR_HEIGHT) const
{
    p->setUniform("view3D", GetViewMatrix());
    p->setUniform("projection", GetProjectionMat(SCR_WIDTH, SCR_HEIGHT));
}

// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void ogl::Camera3D::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch)
{
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch)
    {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    // update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void ogl::Camera3D::ProcessMouseScroll(float yoffset)
{
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 60.0f)
        Zoom = 60.0f;
    updateCameraVectors();
}
void ogl::Camera3D::addDistance(float u)
{
    distance += u;
    updateCameraVectors();
}

// calculates the front vector from the Camera's (updated) Euler Angles
void ogl::Camera3D::updateCameraVectors()
{
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    // also re-calculate the Right and Up vector
    Right = glm::normalize(glm::cross(Front, WorldUp)); // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    Up = glm::normalize(glm::cross(Right, Front));

    Position = Front * -distance;
    glm::vec4 Pos2 = GetViewMatrix() * glm::vec4(Front, 1.0);
    // printf("3D: %.2f %.2f %.2f\n", Position.x, Position.y, Position.z);
    // printf("    %.2f %.2f %.2f\n", Pos2.x, Pos2.y, Pos2.z);
}