#include "ShaderLayerEffect.h"
#include <imgui.h>
#include <vector>
#include <utility>
#include <cstring>
#include <functional>
#include <cstddef>
#include <cstdint>

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

// Поєднання хешів (boost-style)
static inline void hashCombine(std::size_t& h, std::size_t v) {
    h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
}

unsigned int ShaderLayerEffect::renderAndGetTexture(float time, float opacity, const ClipShape& clip,
        const std::vector<std::pair<Effect*, float>>& filters, bool& flipV,
        const ShaderRenderOpts& opts) {
    m_renderOpacity = opacity;

    // Запечений (baked) шар — миттєво віддаємо статичну текстуру, без жодного рендеру
    if (m_baked && m_bakedTex) { flipV = false; return m_bakedTex->getID(); }

    const bool     hasPost      = !filters.empty() || clip.type != ClipShape::None;
    const bool     needReadback = hasPost || m_bakeRequested;   // bake завжди захоплює через readback
    // Нижча роздільність під час драгу — менший readback (256 замість 512); bake — у повній якості
    const uint32_t RES          = m_bakeRequested ? 512u
                                  : ((opts.lowResInteract && opts.interacting) ? 256u : 512u);
    const bool     animated     = (m_speed > 1e-4f);

    // --- Ключ кешу: усе, що впливає на результат (час анімації — окремо) ---
    std::size_t key = 0;
    auto hf = [&](float f) { hashCombine(key, std::hash<float>{}(f)); };
    hashCombine(key, std::hash<int>{}(static_cast<int>(m_type)));
    hf(m_scale); hf(m_speed); hf(m_color[0]); hf(m_color[1]); hf(m_color[2]);
    hashCombine(key, m_stretchPattern ? 1u : 2u);
    hf(m_x); hf(m_y); hf(m_width); hf(m_height);
    hf(opacity);
    hashCombine(key, std::hash<int>{}(clip.type));
    hf(clip.parentW); hf(clip.parentH); hf(clip.radius);
    hashCombine(key, static_cast<std::size_t>(RES));
    hashCombine(key, hasPost ? 1u : 2u);
    for (const auto& pr : filters) {                        // склад фільтрів: вказівник + opacity (лови додавання/видалення)
        hashCombine(key, std::hash<const void*>{}(pr.first));
        hf(pr.second);
    }
    hashCombine(key, std::hash<uint64_t>{}(opts.editGen));  // зміна ВНУТРІШНІХ параметрів фільтрів

    // --- Можливо, віддати кеш без жодної роботи (але не коли просять bake) ---
    if (opts.cache && m_cacheValid && key == m_cacheKey && !m_bakeRequested) {
        if (!animated) {                                    // статичний результат → кеш завжди дійсний
            flipV = m_cacheFlip;
            return m_cacheTex;
        }
        if (opts.throttle && m_lastComputeTime >= 0.0f &&
            (time - m_lastComputeTime) < opts.throttleInterval) {
            flipV = m_cacheFlip;                            // анімований, але оновлювати ще зарано
            return m_cacheTex;
        }
    }

    // ====== Повний перерахунок ======
    if (!m_shader || m_vao == 0) { flipV = m_cacheFlip; return m_cacheTex; }

    if (m_renderFbo.getWidth() != static_cast<int>(RES) || m_renderFbo.getHeight() != static_cast<int>(RES)) {
        m_renderFbo.create(RES, RES);
    }
    GLint prevVp[4];
    glGetIntegerv(GL_VIEWPORT, prevVp);
    m_renderFbo.bind();
    glViewport(0, 0, static_cast<int>(RES), static_cast<int>(RES));
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    renderShader(static_cast<int>(RES), static_cast<int>(RES), time);

    unsigned int resultTex;
    bool         resultFlip;

    if (!needReadback) {
        // Без постобробки — сира FBO-текстура (малюється з V-flip)
        m_renderFbo.unbind();
        glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);
        resultTex  = m_renderFbo.getColorAttachmentID();
        resultFlip = true;
    } else {
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

        // Bake: зберігаємо знімок і переходимо у статичний режим
        if (m_bakeRequested) {
            if (!m_bakedTex) m_bakedTex = std::make_shared<Texture>();
            m_bakedTex->update(img);
            m_baked = true;
            m_bakeRequested = false;
            m_cacheValid = false;   // кеш більше не потрібен
            flipV = false;
            return m_bakedTex->getID();
        }

        if (!m_processed) m_processed = std::make_shared<Texture>();
        m_processed->update(img);
        resultTex  = m_processed->getID();
        resultFlip = false;
    }

    // Оновлюємо кеш
    m_cacheKey        = key;
    m_cacheValid      = true;
    m_cacheTex        = resultTex;
    m_cacheFlip       = resultFlip;
    m_lastComputeTime = time;

    flipV = resultFlip;
    return resultTex;
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

    ImGui::Separator();
    if (m_baked) {
        ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "BAKED \xE2\x80\x94 \xD1\x81\xD1\x82\xD0\xB0\xD1\x82\xD0\xB8\xD1\x87\xD0\xBD\xD0\xB8\xD0\xB9 \xD0\xB7\xD0\xBD\xD1\x96\xD0\xBC\xD0\xBE\xD0\xBA");
        if (ImGui::Button("Un-bake (\xD0\xBF\xD0\xBE\xD0\xB2\xD0\xB5\xD1\x80\xD0\xBD\xD1\x83\xD1\x82\xD0\xB8 \xD0\xB6\xD0\xB8\xD0\xB2\xD0\xB8\xD0\xB9)")) { unbake(); changed = true; }
    } else {
        if (ImGui::Button("Bake to image")) { requestBake(); changed = true; }
        ImGui::SameLine();
        ImGui::TextDisabled("\xD0\xB7\xD0\xB0\xD0\xBC\xD0\xBE\xD1\x80\xD0\xBE\xD0\xB7\xD0\xB8\xD1\x82\xD0\xB8 \xD1\x83 \xD1\x81\xD1\x82\xD0\xB0\xD1\x82\xD0\xB8\xD1\x87\xD0\xBD\xD1\x83 \xD1\x82\xD0\xB5\xD0\xBA\xD1\x81\xD1\x82\xD1\x83\xD1\x80\xD1\x83");
    }

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
