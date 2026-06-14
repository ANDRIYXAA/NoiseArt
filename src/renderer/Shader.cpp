#include "Shader.h"
#include <iostream>
#include <vector>

namespace NoiseArt {

Shader::Shader() {}

Shader::~Shader() {
    if (m_programID != 0) {
        glDeleteProgram(m_programID);
    }
}

Shader::Shader(Shader&& other) noexcept : m_programID(other.m_programID) {
    other.m_programID = 0;
}

Shader& Shader::operator=(Shader&& other) noexcept {
    if (this != &other) {
        if (m_programID != 0) {
            glDeleteProgram(m_programID);
        }
        m_programID = other.m_programID;
        other.m_programID = 0;
    }
    return *this;
}

bool Shader::loadFromMemory(const std::string& vertexSource, const std::string& fragmentSource) {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexSource);
    if (vertexShader == 0) return false;

    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return false;
    }

    m_programID = glCreateProgram();
    glAttachShader(m_programID, vertexShader);
    glAttachShader(m_programID, fragmentShader);
    glLinkProgram(m_programID);

    GLint success;
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetProgramiv(m_programID, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> infoLog(logLength);
        glGetProgramInfoLog(m_programID, logLength, nullptr, infoLog.data());
        std::cerr << "Shader linking failed:\n" << infoLog.data() << std::endl;
        glDeleteProgram(m_programID);
        m_programID = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return m_programID != 0;
}

GLuint Shader::compileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLint logLength;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> infoLog(logLength);
        glGetShaderInfoLog(shader, logLength, nullptr, infoLog.data());
        std::cerr << "Shader compilation failed:\n" << infoLog.data() << "\nSource:\n" << source << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void Shader::bind() const {
    glUseProgram(m_programID);
}

void Shader::unbind() const {
    glUseProgram(0);
}

GLint Shader::getUniformLocation(const std::string& name) const {
    return glGetUniformLocation(m_programID, name.c_str());
}

void Shader::setInt(const std::string& name, int value) const {
    glUniform1i(getUniformLocation(name), value);
}

void Shader::setFloat(const std::string& name, float value) const {
    glUniform1f(getUniformLocation(name), value);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value) const {
    glUniform2fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value) const {
    glUniform3fv(getUniformLocation(name), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value) const {
    glUniform4fv(getUniformLocation(name), 1, &value[0]);
}

} // namespace NoiseArt
