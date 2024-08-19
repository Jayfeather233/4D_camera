#pragma once

#include <vector>
#include <Eigen/Core>
#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader.hpp"

struct obj4D_base
{
    std::vector<float> points;
    std::vector<GLuint> edges;

    obj4D_base(std::vector<float> pts, std::vector<GLuint> eg) : points(pts), edges(eg) {}
};

class obj4D
{
public:
    obj4D_base *base_model;
    glm::vec4 offset;
    glm::mat4 rotate;
    GLuint id, VBO, VAO, EBO;

    obj4D(obj4D_base *bs, const glm::vec4 &off = glm::vec4(0.0f), const glm::mat4 &rot = glm::identity<glm::mat4>()) : base_model(bs), offset(off), rotate(rot)
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
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void *)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 7, (void *)(4 * sizeof(float)));
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

enum obj_4D_type
{
    CUBE,

};

obj4D_base *create_4D_obj_base(obj_4D_type tp = obj_4D_type::CUBE)
{
    if (tp == obj_4D_type::CUBE)
    {
        obj4D_base *obj = new obj4D_base({
            0, 0, 0, 0, 0.8f, 0.8f, 0.0f,
            1, 0, 0, 0, 0.8f, 0.8f, 0.0f,
            0, 1, 0, 0, 0.8f, 0.8f, 0.0f,
            1, 1, 0, 0, 0.8f, 0.8f, 0.0f,
            0, 0, 1, 0, 0.0f, 0.8f, 0.8f,
            1, 0, 1, 0, 0.0f, 0.8f, 0.8f,
            0, 1, 1, 0, 0.0f, 0.8f, 0.8f,
            1, 1, 1, 0, 0.0f, 0.8f, 0.8f,
            0, 0, 0, 1, 0.8f, 0.0f, 0.8f,
            1, 0, 0, 1, 0.8f, 0.0f, 0.8f,
            0, 1, 0, 1, 0.8f, 0.0f, 0.8f,
            1, 1, 0, 1, 0.8f, 0.0f, 0.8f,
            0, 0, 1, 1, 0.8f, 0.0f, 0.0f,
            1, 0, 1, 1, 0.8f, 0.0f, 0.0f,
            0, 1, 1, 1, 0.8f, 0.0f, 0.0f,
            1, 1, 1, 1, 0.8f, 0.0f, 0.0f,
        },{
            0, 1,
            0, 2,
            1, 3,
            2, 3,
            
            4, 5,
            4, 6,
            5, 7,
            6, 7,
            
            8, 9,
            8, 10,
            9, 11,
            10, 11,
            
            12, 13,
            12, 14,
            13, 15,
            14, 15,

            0, 4,
            1, 5,
            2, 6,
            3, 7,

            0, 8,
            1, 9,
            2, 10,
            3, 11,

            8, 12,
            9, 13,
            10, 14,
            11, 15,

            4, 12,
            5, 13,
            6, 14,
            7, 15
        });
        return obj;
    }
}

obj4D create_4D_obj(obj_4D_type tp = obj_4D_type::CUBE)
{
    return obj4D(create_4D_obj_base(tp));
}
obj4D create_4D_obj(obj4D_base *bs)
{
    return obj4D(bs);
}