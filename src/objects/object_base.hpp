#pragma once

#include <glm/gtc/matrix_transform.hpp>

#include "obj_base.hpp"
#include "shader.hpp"

template <unsigned int D>
class object_base
{
protected:
    obj_base *base_model;
    glm::vec<D, float, glm::packed_highp> offset;
    glm::mat<D, D, float, glm::packed_highp> rotate;
    GLuint VBO, VAO, EBO;
    bool is_buffered;

public:
    object_base(obj_base *bs, const glm::vec<D, float, glm::packed_highp> &off = glm::vec<D, float, glm::packed_highp>(0.0f),
                const glm::mat<D, D, glm::f32, glm::packed_highp> &rot = glm::identity<glm::mat<D, D, float, glm::packed_highp>>()) : base_model(bs), offset(off), rotate(rot), is_buffered(false)
    {
    }

    glm::vec<D, float, glm::packed_highp> get_offset() { return offset; };
    glm::mat<D, D, float, glm::packed_highp> get_rotate() { return rotate; };

    virtual void set_VertexAttrPointer() = 0;

    void gen_buffer()
    {
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glGenVertexArrays(1, &VAO);

        glBindVertexArray(VAO);

        // 将顶点数据传输到 GPU
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, base_model->points.size() * sizeof(float), base_model->points.data(), GL_STATIC_DRAW);

        // 将索引数据传输到 GPU
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, base_model->edges.size() * sizeof(GLuint), base_model->edges.data(), GL_STATIC_DRAW);

        // 设置顶点属性指针
        set_VertexAttrPointer();

        // 解绑 VAO
        glBindVertexArray(0);

        is_buffered = true;
    }

    virtual void setUniform(std::shared_ptr<ogl::Program> p) = 0;

    void draw()
    {
        if(!is_buffered){
            gen_buffer();
        }
        glBindVertexArray(VAO);
        glDrawElements(GL_LINES, base_model->edges.size(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void destroy()
    {
        if(!is_buffered) return;
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
        is_buffered = false;
    }

    ~object_base()
    {
        destroy();
    }

    void setOffset(const glm::vec<D, float, glm::packed_highp> &off)
    {
        offset = off;
    }
    void addOffset(const glm::vec<D, float, glm::packed_highp> &off)
    {
        offset += off;
    }
    void setRotate(const glm::mat<D, D, float, glm::packed_highp> &ro)
    {
        rotate = ro;
    }
    void addRotate(const glm::mat<D, D, float, glm::packed_highp> &ro)
    {
        rotate = ro * rotate;
    }
    void addScale(const float &ro)
    {
        rotate = (glm::identity<glm::mat<D, D, float, glm::packed_highp>>() * ro) * rotate;
    }
};