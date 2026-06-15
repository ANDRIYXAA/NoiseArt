#pragma once

#include "effects/Effect.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "effects/ClipShape.h"
#include <glad/glad.h>
#include <memory>
#include <vector>
#include <utility>
#include <cstdint>
#include <cstddef>

namespace NoiseArt {

class ShaderLayerEffect : public Effect, public ITransformable {
public:
    ShaderLayerEffect();
    ~ShaderLayerEffect() override;

    std::string getName() const override { return "Live Shader"; }
    std::string getCategory() const override { return "Backgrounds"; }

    void apply(const Image& input, Image& output) override;

    bool isShader() const override { return true; }
    void renderShader(int width, int height, float time) override;

    bool renderUI() override;

    std::unique_ptr<Effect> clone() const override;

    ITransformable* getTransformable() override { return this; }

    // ITransformable
    float getX() const override { return m_x; }
    float getY() const override { return m_y; }
    float getWidth() const override { return m_width; }
    float getHeight() const override { return m_height; }
    void setPosition(float x, float y) override { m_x = x; m_y = y; }
    void setSize(float w, float h) override { m_width = w; m_height = h; }

    /// Опції оптимізації рендеру (керуються з AppSettings + стану viewport)
    struct ShaderRenderOpts {
        bool     cache            = true;          // кешувати й перераховувати лише при зміні
        bool     throttle         = false;         // анімовані шейдери — не частіше throttleInterval
        float    throttleInterval = 1.0f / 30.0f;  // ~30 FPS
        bool     lowResInteract   = false;         // нижча роздільність під час драгу
        bool     interacting      = false;         // зараз триває перетягування/масштабування
        uint64_t editGen          = 0;             // лічильник правок (для інвалідації кешу)
    };

    /// Рендерить шейдер; за наявності фільтрів/обрізання зчитує пікселі й обробляє на CPU.
    /// flipV=true → повернуто сиру FBO-текстуру (малювати з V-flip); false → оброблену (top-down).
    /// За opts вмикає кеш / тротлінг / нижчу роздільність.
    unsigned int renderAndGetTexture(float time, float opacity, const ClipShape& clip,
                                     const std::vector<std::pair<Effect*, float>>& filters, bool& flipV,
                                     const ShaderRenderOpts& opts);

    // ===== Bake (precompute): заморозити поточний вигляд у статичну текстуру =====
    void requestBake() { m_bakeRequested = true; }
    void unbake()      { m_baked = false; m_bakeRequested = false; m_bakedTex.reset(); m_cacheValid = false; }
    bool isBaked() const { return m_baked; }

    enum class ShaderType {
        Plasma,
        Voronoi,
        Perlin
    };

private:
    void initGL();
    void compileCurrentShader();
    const char* getFragmentSource() const;

    ShaderType m_type = ShaderType::Plasma;
    std::unique_ptr<Shader> m_shader;
    
    GLuint m_vao = 0;
    GLuint m_vbo = 0;

    Framebuffer m_renderFbo;   // власний FBO для рендеру шейдера як текстури

    // Трансформація на полотні
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 400.0f;
    float m_height = 300.0f;
    
    // Shader params
    float m_scale = 5.0f;
    float m_speed = 1.0f;
    float m_color[3] = {1.0f, 0.5f, 0.2f};
    float m_renderOpacity = 1.0f;
    bool m_stretchPattern = true;   // true = патерн тягнеться з рамкою; false = тримає форму

    std::shared_ptr<Texture> m_processed;   // кеш: оброблений фільтрами/обрізанням результат

    // ===== Кеш рендеру (оптимізації) =====
    size_t       m_cacheKey        = 0;       // хеш усіх вхідних даних (окрім часу анімації)
    bool         m_cacheValid      = false;
    unsigned int m_cacheTex        = 0;       // ID текстури, що віддавали востаннє
    bool         m_cacheFlip       = false;   // flipV для кешованої текстури
    float        m_lastComputeTime = -1.0f;   // час останнього перерахунку (для тротлінгу)

    // ===== Bake =====
    bool m_baked         = false;   // якщо true — віддаємо лише m_bakedTex (0 витрат)
    bool m_bakeRequested = false;   // запит зробити знімок наступного кадру
    std::shared_ptr<Texture> m_bakedTex;
};

} // namespace NoiseArt
