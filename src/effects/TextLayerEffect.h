#pragma once

#include "effects/Effect.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "effects/ClipShape.h"
#include <nanovg.h>
#include <string>
#include <memory>
#include <vector>
#include <utility>

namespace NoiseArt {

class TextLayerEffect : public Effect, public ITransformable {
public:
    TextLayerEffect();
    ~TextLayerEffect() override = default;

    std::string getName() const override { return "Text"; }
    std::string getCategory() const override { return "Vector"; }

    void apply(const Image& input, Image& output) override;

    bool isVector() const override { return true; }
    void renderVector(NVGcontext* vg) override;

    bool renderUI() override;

    std::unique_ptr<Effect> clone() const override;

    ITransformable* getTransformable() override { return this; }

    // ITransformable
    float getX() const override { return m_x; }
    float getY() const override { return m_y; }
    float getWidth() const override;
    float getHeight() const override;
    void setPosition(float x, float y) override { m_x = x; m_y = y; }
    void setSize(float w, float h) override;

    // Геттери для візуалізації у viewport
    const std::string& getText() const { return m_text; }
    float getFontSize() const { return m_fontSize; }
    const float* getTextColor() const { return m_color; }

    /// Рендерить текст через NanoVG у власний FBO + застосовує фільтри/обрізання → кеш-текстура
    void rasterizeAndProcess(NVGcontext* vg, const std::vector<std::pair<Effect*, float>>& filters, const ClipShape& clip, float renderScale = 1.0f);
    /// Чи треба переростеризувати текст під поточний зум (для чіткості)
    bool needsRerasterAtZoom(float zoom) const;
    /// Готова текстура тексту (nullptr якщо ще не растеризовано)
    const Texture* getProcessedTexture() const {
        return (m_processed && m_processed->isValid()) ? m_processed.get() : nullptr;
    }

    // Динамічний список шрифтів (стартує з вбудованих, поповнюється через "Add Font")
    static std::vector<std::string>& fonts();
    /// Прапорець-запит на додавання шрифту з файлу (обробляє App — у нього є NanoVG-контекст)
    static bool& fontAddRequested();
    /// URL для завантаження шрифту (непорожній → App завантажить і зареєструє)
    static std::string& pendingFontUrl();
    /// Статус останнього додавання шрифту (показується в UI)
    static std::string& fontStatus();

    // Вбудовані шрифти (ті ж імена, що зареєстровані в NanoVG) — стартовий список
    static constexpr const char* FontNames[] = {
        "Inter", "Arial", "Times New Roman", "Courier New",
        "Georgia", "Verdana", "Trebuchet MS", "Impact",
        "Comic Sans MS", "Segoe UI", "Consolas"
    };
    static constexpr int FontCount = 11;

private:
    std::string m_text = "Hello NoiseArt!";
    char m_textBuffer[256];
    
    float m_x = 200.0f;
    float m_y = 200.0f;
    float m_fontSize = 48.0f;
    
    int m_fontIndex = 0;  // Індекс шрифту в FontNames
    
    float m_color[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    
    // Вирівнювання
    enum class Align { Left, Center, Right };
    Align m_align = Align::Left;
    
    // Bold/Italic (емуляція через NanoVG)
    bool m_bold = false;

    Framebuffer m_fbo;                      // для рендеру тексту через NanoVG
    std::shared_ptr<Texture> m_processed;   // кеш растеризованого/обробленого тексту
    float m_measuredW = 0.0f;               // виміряні розміри (з останньої растеризації)
    float m_measuredH = 0.0f;
    float m_lastScale = 0.0f;               // масштаб останньої растеризації (для зум-чіткості)
};

} // namespace NoiseArt
