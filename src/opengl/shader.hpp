#pragma once

#include <GL/glew.h>

#include <eigen3/Eigen/Core>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

namespace ogl {
enum class shader_type_t { VERT = GL_VERTEX_SHADER, FRAG = GL_FRAGMENT_SHADER, NONE = 0 };

class Shader {
public:
    // from content
    Shader(shader_type_t, const std::string &);
    // from file
    Shader(shader_type_t, const std::filesystem::path &);

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&) = delete;
    Shader &operator=(Shader &&) = delete;

    ~Shader();

    void compile();

    std::string source;
    std::filesystem::path shader_path;
    GLuint id = 0;
    bool is_compiled = false;
};

class Program {
public:
    Program();

    Program(const Program &) = delete;
    Program &operator=(const Program &) = delete;
    Program(Program &&) = delete;
    Program &operator=(Program &&) = delete;

    ~Program();

    void attach(const std::unique_ptr<Shader> &);
    void link();

    void bind();
    void release() const;

    /// Cached glGetUniformLocation; returns -1 for unknown names.
    GLint uniformLocation(const std::string &name);

    void setUniform(const std::string &, int);
    void setUniform(const std::string &, float);
    void setUniform(const std::string &, const Eigen::Vector2f &);
    void setUniform(const std::string &, const Eigen::Vector3f &);
    void setUniform(const std::string &, const Eigen::Vector4f &);
    void setUniform(const std::string &, const Eigen::Matrix3f &);
    void setUniform(const std::string &, const Eigen::Matrix4f &);
    void setUniform(const std::string &, const glm::vec2 &);
    void setUniform(const std::string &, const glm::vec3 &);
    void setUniform(const std::string &, const glm::vec4 &);
    void setUniform(const std::string &, const glm::mat3 &);
    void setUniform(const std::string &, const glm::mat4 &);

private:
    GLuint id = 0;
    bool is_linked = false;
    std::unordered_map<std::string, GLint> uniform_locations_;
};

std::shared_ptr<Program> programFromFiles(const std::filesystem::path &shaderDir, const std::string &vertShaderFilename,
                                          const std::string &fragShaderFilename);

/// Shader directory: $FOURDCAM_SHADER_DIR if set, otherwise the path baked in at build time.
std::filesystem::path defaultShaderDir();
} // namespace ogl
