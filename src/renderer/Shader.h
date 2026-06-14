#pragma once

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

namespace NoiseArt {

class Shader {
public:
    Shader();
    ~Shader();

    // Забороняємо копіювання
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Дозволяємо переміщення
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromMemory(const std::string& vertexSource, const std::string& fragmentSource);

    void bind() const;
    void unbind() const;

    GLuint getID() const { return m_programID; }

    // Uniforms
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;

private:
    GLuint compileShader(GLenum type, const std::string& source);
    GLint getUniformLocation(const std::string& name) const;

    GLuint m_programID = 0;
};

} // namespace NoiseArt
