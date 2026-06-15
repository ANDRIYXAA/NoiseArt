// ============================================================================
// NoiseArt — ShaderNode (процедурний шейдер як нода: Plasma / Perlin)
// ============================================================================
// Рендерить GLSL у власний FBO й видає його текстуру (bottom-up). GLSL
// перенесено з ShaderLayerEffect. Малює спільним квадом з NodeEvalContext.quad.
// ============================================================================
#pragma once

#include "graph/Node.h"
#include "graph/QuadRenderer.h"
#include "renderer/Shader.h"
#include <imgui.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <cstddef>
#include <functional>

namespace NoiseArt {

class ShaderNode : public Node {
public:
    enum class Kind { Plasma, Perlin };

    explicit ShaderNode(int id, Kind kind = Kind::Plasma) : Node(id), m_kind(kind) {
        addOutput("out", SocketType::Texture);
        compile();
    }

    std::string getTypeName() const override { return m_kind == Kind::Perlin ? "Perlin" : "Plasma"; }
    std::string getCategory() const override { return "Shader"; }

    bool isAnimated() const override { return m_speed > 1e-4f; }

    std::size_t paramHash() const override {
        std::size_t h = 0;
        auto mix = [&](std::size_t v) { h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); };
        mix(std::hash<int>{}(static_cast<int>(m_kind)));
        mix(std::hash<float>{}(m_scale));
        mix(std::hash<float>{}(m_speed));
        mix(std::hash<float>{}(m_color[0]));
        mix(std::hash<float>{}(m_color[1]));
        mix(std::hash<float>{}(m_color[2]));
        mix(m_stretch ? 1u : 2u);
        return h;
    }

    unsigned int evaluate(NodeEvalContext& ctx, const std::vector<unsigned int>&) override {
        if (!m_shader || !ctx.quad) return 0;
        const uint32_t RES = (ctx.lowResInteract && ctx.interacting) ? 256u : 512u;
        if (m_fbo.getWidth() != RES || m_fbo.getHeight() != RES) m_fbo.create(RES, RES);

        GLint prevVp[4];
        glGetIntegerv(GL_VIEWPORT, prevVp);
        m_fbo.bind();
        glViewport(0, 0, static_cast<int>(RES), static_cast<int>(RES));
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Без блендингу — альфа = u_opacity (прозорість застосує viewport через tint).
        glDisable(GL_BLEND);
        m_shader->bind();
        m_shader->setFloat("u_time", ctx.time);
        m_shader->setVec2 ("u_resolution", glm::vec2(ctx.layerW, ctx.layerH));
        m_shader->setFloat("u_stretch", m_stretch ? 1.0f : 0.0f);
        m_shader->setFloat("u_scale", m_scale);
        m_shader->setFloat("u_speed", m_speed);
        m_shader->setVec3 ("u_color", glm::vec3(m_color[0], m_color[1], m_color[2]));
        m_shader->setFloat("u_opacity", 1.0f);
        ctx.quad->draw();
        m_shader->unbind();

        m_fbo.unbind();
        glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
        return m_fbo.getColorAttachmentID();
    }

    bool renderParamsUI() override {
        bool changed = false;
        int k = static_cast<int>(m_kind);
        const char* kinds[] = { "Plasma", "Perlin" };
        if (ImGui::Combo("Type", &k, kinds, 2)) { m_kind = static_cast<Kind>(k); compile(); changed = true; }
        if (ImGui::DragFloat("Scale", &m_scale, 0.1f, 0.1f, 100.0f)) changed = true;
        if (ImGui::DragFloat("Speed", &m_speed, 0.01f, 0.0f, 10.0f)) changed = true;
        if (ImGui::ColorEdit3("Color", m_color)) changed = true;
        if (ImGui::Checkbox("Stretch", &m_stretch)) changed = true;
        return changed;
    }

    std::unique_ptr<Node> clone() const override {
        auto n = std::make_unique<ShaderNode>(m_id, m_kind);
        n->copyBaseFrom(*this);
        n->m_scale = m_scale;
        n->m_speed = m_speed;
        n->m_color[0] = m_color[0]; n->m_color[1] = m_color[1]; n->m_color[2] = m_color[2];
        n->m_stretch = m_stretch;
        n->compile();
        return n;
    }

private:
    void compile() {
        m_shader = std::make_unique<Shader>();
        m_shader->loadFromMemory(vertexSrc(), fragSrc());
    }

    static const char* vertexSrc() {
        return R"(#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 TexCoords;
void main() {
    TexCoords = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
)";
    }

    const char* fragSrc() const {
        if (m_kind == Kind::Perlin) {
            return R"(#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform float u_time;
uniform vec2 u_resolution;
uniform float u_scale;
uniform float u_speed;
uniform vec3 u_color;
uniform float u_opacity;
uniform float u_stretch;
float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}
float noise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a)* u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}
void main() {
    vec2 uv = (u_stretch > 0.5)
        ? TexCoords * u_scale
        : TexCoords * u_resolution * (u_scale / 256.0);
    uv.x += u_time * u_speed;
    float n = noise(uv) * 0.5 + 0.5 * noise(uv * 2.0);
    FragColor = vec4(n * u_color.r, n * u_color.g, n * u_color.b, u_opacity);
}
)";
        }
        return R"(#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform float u_time;
uniform vec2 u_resolution;
uniform float u_scale;
uniform float u_speed;
uniform vec3 u_color;
uniform float u_opacity;
uniform float u_stretch;
void main() {
    vec2 uv = (u_stretch > 0.5)
        ? TexCoords * u_scale
        : TexCoords * u_resolution * (u_scale / 256.0);
    float time = u_time * u_speed;
    float v1 = sin(uv.x + time);
    float v2 = sin(uv.y + time);
    float v3 = sin(uv.x + uv.y + time);
    float v4 = sin(sqrt(uv.x * uv.x + uv.y * uv.y) + time);
    float v = v1 + v2 + v3 + v4;
    float r = sin(v * 3.1415) * 0.5 + 0.5;
    float g = sin(v * 3.1415 + 2.0) * 0.5 + 0.5;
    float b = sin(v * 3.1415 + 4.0) * 0.5 + 0.5;
    FragColor = vec4(r * u_color.r, g * u_color.g, b * u_color.b, u_opacity);
}
)";
    }

    Kind                    m_kind  = Kind::Plasma;
    std::unique_ptr<Shader> m_shader;
    float                   m_scale = 5.0f;
    float                   m_speed = 1.0f;
    float                   m_color[3] = { 1.0f, 0.5f, 0.2f };
    bool                    m_stretch = true;
};

} // namespace NoiseArt
