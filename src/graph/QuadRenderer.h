// ============================================================================
// NoiseArt — QuadRenderer (спільний фулскрін-квад для нод-шейдерів)
// ============================================================================
// Один VAO/VBO на граф (NodeGraphEffect володіє ним і передає через
// NodeEvalContext.quad), щоб кожна ShaderNode не дублювала GL-буфери.
// ============================================================================
#pragma once

#include <glad/glad.h>

namespace NoiseArt {

class QuadRenderer {
public:
    QuadRenderer() {
        const float verts[] = {
            -1.0f,  1.0f,
            -1.0f, -1.0f,
             1.0f,  1.0f,
             1.0f, -1.0f,
        };
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glBindVertexArray(0);
    }
    ~QuadRenderer() {
        if (m_vbo) glDeleteBuffers(1, &m_vbo);
        if (m_vao) glDeleteVertexArrays(1, &m_vao);
    }

    QuadRenderer(const QuadRenderer&)            = delete;
    QuadRenderer& operator=(const QuadRenderer&) = delete;

    void draw() const {
        glBindVertexArray(m_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

private:
    GLuint m_vao = 0;
    GLuint m_vbo = 0;
};

} // namespace NoiseArt
