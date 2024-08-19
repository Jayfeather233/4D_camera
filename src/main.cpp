#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Eigen/Core>

// #include <glad/glad.h>
#include <iostream>
#include <vector>
#include <cmath>

#include "object4D.hpp"
#include "camera3D.hpp"
#include "camera4D.hpp"
#include "shader.hpp"

int SCR_WIDTH=800, SCR_HEIGHT=600;

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}
void processInput(GLFWwindow *window, float deltaTime);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

ogl::Camera3D cam3d;
ogl::Camera4D cam4d;

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Nya", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);  // Enable vsync
    
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        throw std::runtime_error(
            "Failed to initialize GLEW: {}" + std::string(reinterpret_cast<char const *>(glewGetErrorString(err))));
    }
    
    auto shaderProgram2 = ogl::programFromFiles("./shaders", "simple3D.vs", "simple.fs");
    auto shaderProgram = ogl::programFromFiles("./shaders", "simple4D.vs", "simple.fs");

    // Vertex data & buffers
    object4D obj = create_4D_obj();
    obj.offset = glm::vec4(0.5);
    obj.gen_buffer();

    object3D obj2 = create_3D_obj();
    obj2.offset = glm::vec3(0.75);
    obj2.gen_buffer();

    // Rendering loop
    float lastTime = glfwGetTime();

    // Rendering loop
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT);

        shaderProgram2->bind();
        obj2.draw();
        obj2.SetUniform(shaderProgram2);
        cam3d.SetUniform(shaderProgram2, SCR_WIDTH, SCR_HEIGHT);
        shaderProgram2->release();



        shaderProgram->bind();
        obj.draw();
        obj.SetUniform(shaderProgram);
        cam3d.SetUniform(shaderProgram, SCR_WIDTH, SCR_HEIGHT);
        cam4d.SetUniform(shaderProgram);
        shaderProgram->release();


        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    obj.destroy();
    obj2.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::FORWARD_4D, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::BACKWARD_4D, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::LEFT1, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::RIGHT1, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS){
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::LEFT2, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS){
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::RIGHT2, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS){
        cam3d.ProcessMouseMovement(0.0f, 1000.0f * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS){
        cam3d.ProcessMouseMovement(0.0f, -1000.0f * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS){
        cam3d.ProcessMouseMovement(-1000.0f * deltaTime, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS){
        cam3d.ProcessMouseMovement(1000.0f * deltaTime, 0.0f);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    cam3d.ProcessMouseScroll(yoffset);
}