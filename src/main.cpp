#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Eigen/Core>

// #include <glad/glad.h>
#include <iostream>
#include <vector>
#include <cmath>

#include "objects.hpp"
#include "camera3D.hpp"
#include "camera4D.hpp"
#include "shader.hpp"

int SCR_WIDTH = 800, SCR_HEIGHT = 600;

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}
void processInput(GLFWwindow *window, float deltaTime);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

ogl::Camera3D cam3d;
ogl::Camera4D cam4d;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "4DCam", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1); // Enable vsync

    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        throw std::runtime_error(
            "Failed to initialize GLEW: {}" + std::string(reinterpret_cast<char const *>(glewGetErrorString(err))));
    }
    glEnable(GL_MULTISAMPLE);

    auto shaderProgram2 = ogl::programFromFiles("./shaders", "simple3D.vs", "simple.fs");
    auto shaderProgram = ogl::programFromFiles("./shaders", "simple4D.vs", "simple.fs");

    // Vertex data & buffers
    obj_base *cube4d = create_obj_base();
    std::vector<object4D*> objs;
    for (int i = 0; i < 9; i++)
    {
        if(i<4) continue;
        objs.push_back(new object4D(cube4d));
        object4D *u = objs[objs.size()-1];

        if (i != 8)
        {
            glm::vec4 off = glm::vec4(0.0);
            off[i / 2] = i % 2 ? 1 : -1;
            off += -0.5f;
            u->setOffset(off);
        } else {
            u->setOffset(glm::vec4(-0.5f));
        }
        u->gen_buffer();
    }

    // objs.push_back(new object4D(object_type::COORD_4D));
    // objs[objs.size()-1]->gen_buffer();

    object3D obj2 = object3D(object_type::CUBE_3D);
    obj2.addRotate(1.5f);
    obj2.setOffset(glm::vec3(-0.5f));
    obj2.gen_buffer();

    // Rendering loop
    float lastTime = glfwGetTime();

    // Rendering loop
    while (!glfwWindowShouldClose(window))
    {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        processInput(window, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT);

        shaderProgram2->bind();
        cam3d.SetUniform(shaderProgram2, SCR_WIDTH, SCR_HEIGHT);
        obj2.draw();
        shaderProgram2->release();

        shaderProgram->bind();
        cam3d.SetUniform(shaderProgram, SCR_WIDTH, SCR_HEIGHT);
        cam4d.SetUniform(shaderProgram);
        for (auto obj : objs)
        {
            obj->draw();
        }
        shaderProgram->release();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    for (auto obj : objs)
        obj->destroy();
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

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::FORWARD_4D, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::BACKWARD_4D, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::LEFT1, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::RIGHT1, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::LEFT2, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        cam4d.ProcessKeyboard(ogl::Camera4D_Movement::RIGHT2, deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
    {
        cam3d.ProcessMouseMovement(0.0f, 1000.0f * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
    {
        cam3d.ProcessMouseMovement(0.0f, -1000.0f * deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
    {
        cam3d.ProcessMouseMovement(-1000.0f * deltaTime, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        cam3d.ProcessMouseMovement(1000.0f * deltaTime, 0.0f);
    }
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
    {
        cam4d.addDistance(deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
    {
        cam4d.addDistance(-deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS)
    {
        cam3d.addDistance(deltaTime);
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        cam3d.addDistance(-deltaTime);
    }
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    cam3d.ProcessMouseScroll(yoffset);
}