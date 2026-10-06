#include "opengl/shader.hpp"

#include <fmt/core.h>
#include <glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <utility>

#include <cstdlib>

#ifndef FOURDCAM_SHADER_DIR
#define FOURDCAM_SHADER_DIR "shaders"
#endif

namespace ogl {

Shader::Shader(shader_type_t type, const std::string &content) : source(content), is_compiled(false)
{
    id = glCreateShader(static_cast<GLenum>(type));
    const char *c = source.c_str();
    glShaderSource(id, 1, &c, nullptr);
    compile();
}

Shader::Shader(shader_type_t type, const std::filesystem::path &path) : shader_path(path)
{
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(fmt::format("Shader path {} not exist", path.string()));
    }
    std::stringstream contentstream;
    std::string line;
    std::ifstream f{path};
    while (std::getline(f, line)) {
        // TODO: simple preprocessor for #include
        contentstream << line << '\n';
    }
    source = contentstream.str();

    id = glCreateShader(static_cast<GLenum>(type));
    const char *c = source.c_str();
    glShaderSource(id, 1, &c, nullptr);
    compile();
}

Shader::~Shader() { glDeleteShader(id); }

void Shader::compile()
{
    glCompileShader(id);
    GLint success = GL_FALSE;
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infoLog[1024] = {};
        glGetShaderInfoLog(id, sizeof(infoLog), nullptr, infoLog);
        throw std::runtime_error(
            fmt::format("Shader Error:\nShader Path: {}\nError Info: {}\n", shader_path.string(), infoLog));
    }
    is_compiled = true;
}

Program::Program() { id = glCreateProgram(); }

Program::~Program() { glDeleteProgram(id); }

void Program::attach(const std::unique_ptr<Shader> &shader)
{
    if (!shader->is_compiled)
        shader->compile();
    glAttachShader(id, shader->id);
}

void Program::link()
{
    glLinkProgram(id);
    GLint success = GL_FALSE;
    glGetProgramiv(id, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        GLchar infoLog[1024] = {};
        glGetProgramInfoLog(id, sizeof(infoLog), nullptr, infoLog);
        throw std::runtime_error(infoLog);
    }
    is_linked = true;
}

void Program::bind()
{
    if (!is_linked)
        link();
    if (!is_linked)
        return;
    glUseProgram(id);
}

void Program::release() const { glUseProgram(0); }

GLint Program::uniformLocation(const std::string &name)
{
    if (!is_linked)
        link();
    auto it = uniform_locations_.find(name);
    if (it != uniform_locations_.end())
        return it->second;
    const GLint location = glGetUniformLocation(id, name.c_str());
    uniform_locations_.emplace(name, location);
    return location;
}

void Program::setUniform(const std::string &name, int v) { glUniform1i(uniformLocation(name), v); }

void Program::setUniform(const std::string &name, float v) { glUniform1f(uniformLocation(name), v); }

void Program::setUniform(const std::string &name, const Eigen::Vector2f &v)
{
    glUniform2f(uniformLocation(name), v[0], v[1]);
}

void Program::setUniform(const std::string &name, const Eigen::Vector3f &v)
{
    glUniform3f(uniformLocation(name), v[0], v[1], v[2]);
}

void Program::setUniform(const std::string &name, const Eigen::Vector4f &v)
{
    glUniform4f(uniformLocation(name), v[0], v[1], v[2], v[3]);
}

void Program::setUniform(const std::string &name, const Eigen::Matrix3f &v)
{
    glUniformMatrix3fv(uniformLocation(name), 1, GL_FALSE, v.data());
}

void Program::setUniform(const std::string &name, const Eigen::Matrix4f &v)
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, v.data());
}

void Program::setUniform(const std::string &name, const glm::vec2 &v)
{
    glUniform2fv(uniformLocation(name), 1, glm::value_ptr(v));
}

void Program::setUniform(const std::string &name, const glm::vec3 &v)
{
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(v));
}

void Program::setUniform(const std::string &name, const glm::vec4 &v)
{
    glUniform4fv(uniformLocation(name), 1, glm::value_ptr(v));
}

void Program::setUniform(const std::string &name, const glm::mat3 &v)
{
    glUniformMatrix3fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(v));
}

void Program::setUniform(const std::string &name, const glm::mat4 &v)
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(v));
}

std::shared_ptr<Program> programFromFiles(const std::filesystem::path &shaderDir, const std::string &vertShaderFilename,
                                          const std::string &fragShaderFilename)
{
    auto program = std::make_shared<Program>();
    program->attach(std::make_unique<Shader>(shader_type_t::VERT, shaderDir / vertShaderFilename));
    program->attach(std::make_unique<Shader>(shader_type_t::FRAG, shaderDir / fragShaderFilename));
    program->link();
    return program;
}

std::filesystem::path defaultShaderDir()
{
    if (const char *env = std::getenv("FOURDCAM_SHADER_DIR"))
        return env;
    return FOURDCAM_SHADER_DIR;
}

} // namespace ogl
