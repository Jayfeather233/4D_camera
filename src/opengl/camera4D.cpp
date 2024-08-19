#include "camera4D.hpp"

static const float SENSITIVITY = 10.0f;
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