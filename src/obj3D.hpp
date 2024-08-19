#pragma once

#include <vector>
#include <Eigen/Core>
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp"

struct obj3D_base
{
    std::vector<float> points;
    std::vector<GLuint> edges;

    obj3D_base(std::vector<float> pts, std::vector<GLuint> eg) : points(pts), edges(eg) {}
};

class obj3D
{
public:
    obj3D_base *base_model;
    glm::vec3 offset;
    glm::mat3 rotate;
    GLuint id, VBO, VAO, EBO;

    obj3D(obj3D_base *bs, const glm::vec3 &off = glm::vec3(0.0f), const glm::mat3 &rot = glm::identity<glm::mat3>()) : base_model(bs), offset(off), rotate(rot)
    {
        id = 0;
    }

    void gen_buffer(GLuint id)
    {
        this->id = id;
        glGenBuffers(id, &VBO);
        glGenBuffers(id, &EBO);
        glGenVertexArrays(id, &VAO);

        glBindVertexArray(VAO);

        // 将顶点数据传输到 GPU
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, base_model->points.size() * sizeof(float), base_model->points.data(), GL_STATIC_DRAW);

        // 将索引数据传输到 GPU
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, base_model->edges.size() * sizeof(GLuint), base_model->edges.data(), GL_STATIC_DRAW);

        // 设置顶点属性指针
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // 解绑 VAO
        glBindVertexArray(0);
    }

    void draw()
    {
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, base_model->edges.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void destroy()
    {
        glDeleteVertexArrays(id, &VAO);
        glDeleteBuffers(id, &VBO);
    }

    void SetUniform(std::shared_ptr<ogl::Program> p)
    {
        p->setUniform("model_rot", rotate);
        p->setUniform("model_off", offset);
    }
};

enum obj_3D_type
{
    CUBE_3D,

};

obj3D_base *create_3D_obj_base(obj_3D_type tp = obj_3D_type::CUBE_3D)
{
    if (tp == obj_3D_type::CUBE_3D)
    {
        obj3D_base *obj = new obj3D_base({
            0.0, 0.0, 0.0, 0.8f, 0.8f, 0.0f,
            1.5, 0.0, 0.0, 0.8f, 0.8f, 0.0f,
            0.0, 1.5, 0.0, 0.8f, 0.8f, 0.0f,
            1.5, 1.5, 0.0, 0.8f, 0.8f, 0.0f,
            0.0, 0.0, 1.5, 0.0f, 0.8f, 0.8f,
            1.5, 0.0, 1.5, 0.0f, 0.8f, 0.8f,
            0.0, 1.5, 1.5, 0.0f, 0.8f, 0.8f,
            1.5, 1.5, 1.5, 0.0f, 0.8f, 0.8f,
        },{
            0, 1,
            0, 2,
            1, 3,
            2, 3,
            
            4, 5,
            4, 6,
            5, 7,
            6, 7,
            
            0, 4,
            1, 5,
            2, 6,
            3, 7,
        });
        return obj;
    }
}

obj3D create_3D_obj(obj_3D_type tp = obj_3D_type::CUBE_3D)
{
    return obj3D(create_3D_obj_base(tp));
}
obj3D create_3D_obj(obj3D_base *bs)
{
    return obj3D(bs);
}