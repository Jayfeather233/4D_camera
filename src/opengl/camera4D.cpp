#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "camera4D.hpp"

ogl::Camera4D::Camera4D()
{
    Yaw1 = Yaw2 = 0.0f;
    Pitch = 0.0f;
    Zoom = 45.0f;
    distance = 2.0f;
    updateCameraVectors();
}

glm::mat4 ogl::Camera4D::GetRotationMat() const
{
    glm::mat4 rotYaw1 = glm::identity<glm::mat4>();
    rotYaw1[0][0] = cos(glm::radians(Yaw1));
    rotYaw1[0][3] = sin(glm::radians(Yaw1));
    rotYaw1[3][3] = cos(glm::radians(Yaw1));
    rotYaw1[3][0] = -sin(glm::radians(Yaw1));
    glm::mat4 rotYaw2 = glm::identity<glm::mat4>();
    rotYaw2[2][2] = cos(glm::radians(Yaw2));
    rotYaw2[2][3] = sin(glm::radians(Yaw2));
    rotYaw2[3][3] = cos(glm::radians(Yaw2));
    rotYaw2[3][2] = -sin(glm::radians(Yaw2));
    glm::mat4 rotPitch = glm::identity<glm::mat4>();
    rotPitch[1][1] = cos(glm::radians(Pitch));
    rotPitch[1][3] = sin(glm::radians(Pitch));
    rotPitch[3][3] = cos(glm::radians(Pitch));
    rotPitch[3][1] = -sin(glm::radians(Pitch));

    // Combine the rotations to form the full rotation matrix
    return rotYaw1 * rotYaw2 * rotPitch;
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
std::pair<glm::mat4, glm::vec4> ogl::Camera4D::GetViewMatrix() const
{
    return {GetRotationMat(), Position};
}

void ogl::Camera4D::SetUniform(std::shared_ptr<ogl::Program> p) const
{
    auto ret = GetViewMatrix();
    p->setUniform("view4D_rot", ret.first);
    p->setUniform("view4D_off", ret.second);
}
void ogl::Camera4D::addDistance(float u)
{
    distance += u;
    updateCameraVectors();
}
void ogl::Camera4D::updateCameraVectors()
{
    // 计算新的 Front 向量
    // printf("4D:\n");
    glm::vec4 front = glm::vec4(0.0, 0.0, 0.0, 1.0);
    Front = front * GetRotationMat();
    // printf("front: %.2f %.2f %.2f %.2f\n", front.x, front.y, front.z, front.w);
    Position = Front * -distance;
    // printf("Position: %.2f %.2f %.2f %.2f\n", Position.x, Position.y, Position.z, Position.w);
}

static const float SENSITIVITY = 30.0f;
void ogl::Camera4D::ProcessKeyboard(Camera4D_Movement direction, float deltaTime)
{
    float velocity = SENSITIVITY * deltaTime;
    if (direction == FORWARD_4D)
        Pitch += velocity;
    if (direction == BACKWARD_4D)
        Pitch -= velocity;
    if (direction == LEFT1)
        Yaw1 -= velocity;
    if (direction == RIGHT1)
        Yaw1 += velocity;
    if (direction == LEFT2)
        Yaw2 -= velocity;
    if (direction == RIGHT2)
        Yaw2 += velocity;

    updateCameraVectors();
}