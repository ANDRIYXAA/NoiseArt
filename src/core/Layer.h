// ============================================================================
// NoiseArt — Клас Layer (Шар)
// ============================================================================
// Один шар = ефект + метадані (назва, видимість, непрозорість, режим змішування).
//
// Шар "обгортає" ефект і додає до нього:
// - Opacity (непрозорість): 0% = повністю прозорий, 100% = повний ефект
// - BlendMode (режим змішування): як шар комбінується з попереднім
// - Enabled (увімкнений): можна тимчасово "вимкнути" шар без видалення
// ============================================================================

#pragma once

#include <string>
#include <memory>
#include "effects/Effect.h"
#include "core/Image.h"

namespace NoiseArt {

// ============================================================================
// BlendMode — режими змішування
// ============================================================================
// Визначає, ЯК результат ефекту комбінується з оригіналом.
//
// Приклад (для одного пікселя, opacity = 100%):
//   Original = 100, Effect = 200
//   Normal:   200                    (просто заміна)
//   Add:      min(100 + 200, 255) = 255  (додавання — яскравіше)
//   Multiply: 100 * 200 / 255 = 78   (множення — темніше)
//   Screen:   255 - (155 * 55 / 255) = 221  (екран — яскравіше)
enum class BlendMode {
    Normal,     // Просто замінює (з урахуванням opacity)
    Add,        // Додавання — робить яскравіше
    Multiply,   // Множення — робить темніше
    Screen,     // Екран — м'яке освітлення
    Overlay     // Накладення — збільшує контраст
};

// Масив назв для відображення в UI
inline const char* BlendModeNames[] = {
    "Normal", "Add", "Multiply", "Screen", "Overlay"
};
inline constexpr int BlendModeCount = 5;

// ============================================================================
// Клас Layer
// ============================================================================
class Layer {
public:
    /// Конструктор: створює шар з ефектом та необов'язковою назвою
    Layer(std::unique_ptr<Effect> effect, const std::string& name = "");

    /// Застосувати ефект з урахуванням opacity та blend mode
    void process(const Image& input, Image& output);

    // ----- Властивості -----
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    Effect* getEffect() { return m_effect.get(); }
    const Effect* getEffect() const { return m_effect.get(); }

    float getOpacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = std::max(0.0f, std::min(1.0f, opacity)); }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    BlendMode getBlendMode() const { return m_blendMode; }
    void setBlendMode(BlendMode mode) { m_blendMode = mode; }

    /// Дублювання шару (з копією ефекту)
    std::unique_ptr<Layer> clone() const;

private:
    /// Змішати два пікселі відповідно до blend mode та opacity
    void blendPixel(const uint8_t* base, const uint8_t* effect,
                    uint8_t* result, float opacity, BlendMode mode);

    std::string m_name;
    std::unique_ptr<Effect> m_effect;
    float m_opacity = 1.0f;
    bool m_enabled = true;
    BlendMode m_blendMode = BlendMode::Normal;
};

} // namespace NoiseArt
