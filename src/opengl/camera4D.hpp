#pragma once

#include <glm/glm.hpp>

#include <GL/glew.h>

#include "shader.hpp"

namespace ogl {

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific
// input methods
enum Camera4D_Movement { FORWARD_4D, BACKWARD_4D, LEFT1, LEFT2, RIGHT1, RIGHT2 };

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for
// use in OpenGL
class Camera4D {
public:
    // camera Attributes
    glm::vec4 Position;
    glm::vec4 Front;
    // euler Angles
    float Yaw1, Yaw2;
    float Pitch;
    float Zoom;
    float distance;

    // constructor with vectors
    Camera4D();

    glm::mat4 GetRotationMat() const;

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    std::pair<glm::mat4, glm::vec4> GetViewMatrix() const;

    void SetUniform(std::shared_ptr<ogl::Program> p) const;

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera
    // defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera4D_Movement direction, float deltaTime);
    void addDistance(float u);

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};

} // namespace ogl