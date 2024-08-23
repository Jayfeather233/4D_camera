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
    float distance;

    // constructor with vectors
    Camera4D()
    {
        Yaw1 = Yaw2 = 0.0f;
        Pitch = 0.0f;
        Zoom = 45.0f;
        distance = 2.0f;
        updateCameraVectors();
    }

    glm::mat4 GetRotationMat(){
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
    std::pair<glm::mat4, glm::vec4> GetViewMatrix()
    {
        return { GetRotationMat(), Position };
    }

    void SetUniform(std::shared_ptr<ogl::Program> p){
        auto ret = GetViewMatrix();
        p->setUniform("view4D_rot", ret.first);
        p->setUniform("view4D_off", ret.second);
    }

    // processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void ProcessKeyboard(Camera4D_Movement direction, float deltaTime);
    void addDistance(float u){
        distance += u;
        updateCameraVectors();
    }
private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors()
    {
        // 计算新的 Front 向量
        // printf("4D:\n");
        glm::vec4 front = glm::vec4(0.0, 0.0, 0.0, 1.0);
        Front = front * GetRotationMat();
        // printf("front: %.2f %.2f %.2f %.2f\n", front.x, front.y, front.z, front.w);
        Position = Front * -distance;
        // printf("Position: %.2f %.2f %.2f %.2f\n", Position.x, Position.y, Position.z, Position.w);
    }


};

} // namespace ogl