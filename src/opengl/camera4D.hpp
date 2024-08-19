#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <GL/glew.h>

#include "shader.hpp"

namespace ogl
{

// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera4D_Movement
{
    FORWARD_4D,
    BACKWARD_4D,
    LEFT1,
    LEFT2,
    RIGHT1,
    RIGHT2
};

// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera4D
{
public:
    // camera Attributes
    glm::vec4 Position;
    glm::vec4 Front;
    // euler Angles
    float Yaw1, Yaw2;
    float Pitch;
    float Zoom;

    // constructor with vectors
    Camera4D()
    {
        Yaw1 = Yaw2 = 0.0f;
        Pitch = 0.0f;
        Zoom = 45.0f;
        updateCameraVectors();
    }

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    std::pair<glm::mat4, glm::vec4> GetViewMatrix()
    {
        // Create a rotation matrix based on Yaw1, Yaw2, and Pitch.
        // These angles rotate around the 4D axes: X, Y, Z, W respectively.

        glm::mat4 rotYaw1 = glm::identity<glm::mat4>();
        rotYaw1[0][0] = cos(glm::radians(Yaw1));
        rotYaw1[0][3] = sin(glm::radians(Yaw1));
        rotYaw1[3][3] = cos(glm::radians(Yaw1));
        rotYaw1[3][0] = -sin(glm::radians(Yaw1));
        glm::mat4 rotYaw2 = glm::identity<glm::mat4>();
        rotYaw1[1][1] = cos(glm::radians(Yaw2));
        rotYaw1[1][3] = sin(glm::radians(Yaw2));
        rotYaw1[3][3] = cos(glm::radians(Yaw2));
        rotYaw1[3][1] = -sin(glm::radians(Yaw2));
        glm::mat4 rotPitch = glm::identity<glm::mat4>();
        rotYaw1[2][2] = cos(glm::radians(Pitch));
        rotYaw1[2][3] = sin(glm::radians(Pitch));
        rotYaw1[3][3] = cos(glm::radians(Pitch));
        rotYaw1[3][2] = -sin(glm::radians(Pitch));

        // Combine the rotations to form the full rotation matrix
        glm::mat4 rotation = rotYaw1 * rotYaw2 * rotPitch;

        glm::vec4 offset = Position;

        // Return the 4D rotation matrix and the offset vector
        return { rotation, offset };
    }

    void SetUniform(std::shared_ptr<ogl::Program> p){
        auto ret = GetViewMatrix();
        p->setUniform("view4D_rot", ret.first);
        p->setUniform("view4D_off", ret.second);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera4D_Movement direction, float deltaTime);
private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // 计算新的 Front 向量
        glm::vec4 front;
        front.x = cos(glm::radians(Yaw1)) * sin(glm::radians(Pitch));
        front.y = sin(glm::radians(Yaw1)) * sin(glm::radians(Pitch));
        front.z = cos(glm::radians(Yaw2)) * cos(glm::radians(Pitch));
        front.w = sin(glm::radians(Yaw2)) * cos(glm::radians(Pitch));
        Front = front;
        Position = -Front * 3.0f;
    }

};

} // namespace ogl