#include "ShaderLayerEffect.h"
#include <imgui.h>
#include <vector>
#include <utility>
#include <cstring>

namespace NoiseArt {

static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
out vec2 TexCoords;
void main() {
    TexCoords = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}
)";

static const char* plasmaFragmentSource = R"(
#version 330 core
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
        ? TexCoords * u_scale                            // патерн тягнеться разом з рамкою
        : TexCoords * u_resolution * (u_scale / 256.0);  // сталий розмір клітинок по обох осях
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

static const char* perlinFragmentSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform float u_time;
uniform vec2 u_resolution;
uniform float u_scale;
uniform float u_speed;
uniform vec3 u_color;
uniform float u_opacity;
uniform float u_stretch;

// Simple 2D noise
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
        ? TexCoords * u_scale                            // патерн тягнеться разом з рамкою
        : TexCoords * u_resolution * (u_scale / 256.0);  // сталий розмір клітинок по обох осях
    uv.x += u_time * u_speed;
    
    float n = noise(uv) * 0.5 + 0.5 * noise(uv * 2.0);
    FragColor = vec4(n * u_color.r, n * u_color.g, n * u_color.b, u_opacity);
}
)";

ShaderLayerEffect::ShaderLayerEffect() {
    initGL();
    compileCurrentShader();
}

ShaderLayerEffect::~ShaderLayerEffect() {
    if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
}

void ShaderLayerEffect::initGL() {
    float vertices[] = {
        // pos 
        -1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
         1.0f, -1.0f,
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

const char* ShaderLayerEffect::getFragmentSource() const {
    switch(m_type) {
        case ShaderType::Perlin: return perlinFragmentSource;
        case ShaderType::Plasma: default: return plasmaFragmentSource;
    }
}

void ShaderLayerEffect::compileCurrentShader() {
    m_shader = std::make_unique<Shader>();
    m_shader->loadFromMemory(vertexShaderSource, getFragmentSource());
}

void ShaderLayerEffect::apply(const Image& input, Image& output) {
    output = input; // Raster not affected
}

void ShaderLayerEffect::renderShader(int width, int height, float time) {
    if (!m_shader || m_vao == 0) return;

    m_shader->bind();
    m_shader->setFloat("u_time", time);
    // u_resolution = розмір рамки (для сталого розміру клітинок); u_stretch = режим патерна
    m_shader->setVec2("u_resolution", glm::vec2(m_width, m_height));
    m_shader->setFloat("u_stretch", m_stretchPattern ? 1.0f : 0.0f);
    m_shader->setFloat("u_scale", m_scale);
    m_shader->setFloat("u_speed", m_speed);
    m_shader->setVec3("u_color", glm::vec3(m_color[0], m_color[1], m_color[2]));
    m_shader->setFloat("u_opacity", m_renderOpacity);

    // Пишемо фрагмент напряму у FBO (без блендингу) — щоб альфа дорівнювала u_opacity,
    // а не множилась сама на себе. Прозорість застосує вже ImGui при малюванні текстури.
    glDisable(GL_BLEND);

    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    m_shader->unbind();
}

unsigned int ShaderLayerEffect::renderAndGetTexture(float time, float opacity, const ClipShape& clip,
        const std::vector<std::pair<Effect*, float>>& filters, bool& flipV) {
    m_renderOpacity = opacity;
    // Фіксована роздільність FBO: масштаб робиться при малюванні текстури у viewport.
    const uint32_t RES = 512;
    if (m_renderFbo.getWidth() != RES || m_renderFbo.getHeight() != RES) {
        m_renderFbo.create(RES, RES);
    }
    GLint prevVp[4];
    glGetIntegerv(GL_VIEWPORT, prevVp);
    m_renderFbo.bind();
    glViewport(0, 0, static_cast<int>(RES), static_cast<int>(RES));
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    renderShader(static_cast<int>(RES), static_cast<int>(RES), time);

    // Без фільтрів і без обрізання — повертаємо сиру FBO-текстуру (малюється з V-flip)
    if (filters.empty() && clip.type == ClipShape::None) {
        m_renderFbo.unbind();
        glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
        flipV = true;
        return m_renderFbo.getColorAttachmentID();
    }

    // Зчитуємо пікселі з FBO
    Image img;
    img.create(static_cast<int>(RES), static_cast<int>(RES), 4);
    glReadPixels(0, 0, static_cast<int>(RES), static_cast<int>(RES), GL_RGBA, GL_UNSIGNED_BYTE, img.getData());
    m_renderFbo.unbind();
    glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);

    // glReadPixels дає bottom-up → перевертаємо рядки у top-down (як фото/вектор)
    int W = img.getWidth(), H = img.getHeight();
    int rowBytes = W * 4;
    uint8_t* d = img.getData();
    std::vector<uint8_t> tmp(rowBytes);
    for (int y = 0; y < H / 2; ++y) {
        uint8_t* r0 = d + static_cast<size_t>(y) * rowBytes;
        uint8_t* r1 = d + static_cast<size_t>(H - 1 - y) * rowBytes;
        std::memcpy(tmp.data(), r0, rowBytes);
        std::memcpy(r0, r1, rowBytes);
        std::memcpy(r1, tmp.data(), rowBytes);
    }

    // Фільтри (як для фото/вектора)
    for (const auto& pr : filters) {
        Effect* f = pr.first;
        float op = pr.second;
        if (!f || op <= 0.0f) continue;
        Image res;
        f->apply(img, res);
        if (op >= 1.0f || res.getWidth() != img.getWidth() || res.getHeight() != img.getHeight()) {
            img = std::move(res);
        } else {
            uint8_t* a = img.getData();
            const uint8_t* b = res.getData();
            int n = img.getWidth() * img.getHeight() * 4;
            for (int i = 0; i < n; ++i)
                a[i] = static_cast<uint8_t>(a[i] + op * (static_cast<float>(b[i]) - a[i]));
        }
    }

    // Обрізання по формі батька (top-down)
    applyClipMask(img, clip, getX(), getY(), getWidth(), getHeight());

    if (!m_processed) m_processed = std::make_shared<Texture>();
    m_processed->update(img);
    flipV = false;
    return m_processed->getID();
}

bool ShaderLayerEffect::renderUI() {
    bool changed = false;

    int type = static_cast<int>(m_type);
    const char* types[] = { "Plasma", "Voronoi (TBD)", "Perlin Noise" };
    if (ImGui::Combo("Type", &type, types, 3)) {
        m_type = static_cast<ShaderType>(type);
        compileCurrentShader();
        changed = true;
    }

    if (ImGui::DragFloat("Scale", &m_scale, 0.1f, 0.1f, 100.0f)) changed = true;
    if (ImGui::DragFloat("Speed", &m_speed, 0.01f, 0.0f, 10.0f)) changed = true;
    if (ImGui::ColorEdit3("Color", m_color)) changed = true;
    if (ImGui::Checkbox("Stretch pattern", &m_stretchPattern)) changed = true;

    return changed;
}

std::unique_ptr<Effect> ShaderLayerEffect::clone() const {
    auto copy = std::make_unique<ShaderLayerEffect>();
    copy->m_type = m_type;
    copy->m_scale = m_scale;
    copy->m_speed = m_speed;
    copy->m_color[0] = m_color[0];
    copy->m_color[1] = m_color[1];
    copy->m_color[2] = m_color[2];
    copy->m_stretchPattern = m_stretchPattern;
    copy->compileCurrentShader();
    return copy;
}

} // namespace NoiseArt
