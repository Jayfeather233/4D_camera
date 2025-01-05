#pragma once

#include <glm/glm.hpp>

#include <GL/glew.h>

#include "shader.hpp"

namespace ogl {
// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific
// input methods
enum Camera3D_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for
// use in OpenGL
class Camera3D {
public:
    // camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    // euler Angles
    float Yaw;
    float Pitch;
    // camera options
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;
    float distance;

    // constructor with vectors
    Camera3D(glm::vec3 position = glm::vec3(0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = 0.0f,
             float pitch = 0.0f);

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 GetViewMatrix() const;

    glm::mat4 GetProjectionMat(uint32_t SCR_WIDTH, uint32_t SCR_HEIGHT) const;

    void SetUniform(std::shared_ptr<ogl::Program> p, uint32_t SCR_WIDTH, uint32_t SCR_HEIGHT) const;

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void ProcessMouseScroll(float yoffset);
    void addDistance(float u);

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};
} // namespace ogl