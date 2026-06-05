// ============================================================================
// NoiseArt — Клас Layer (Реалізація)
// ============================================================================

#include "Layer.h"
#include <algorithm>
#include <cmath>

namespace NoiseArt {

// ============================================================================
// Конструктор
// ============================================================================
Layer::Layer(std::unique_ptr<Effect> effect, const std::string& name)
    : m_effect(std::move(effect))  // std::move "переміщує" володіння об'єктом
{
    // Якщо ім'я не задано — використовуємо назву ефекту
    if (name.empty() && m_effect) {
        m_name = m_effect->getName();
    } else {
        m_name = name;
    }
}

// ============================================================================
// process() — Застосувати ефект з blend mode та opacity
// ============================================================================
void Layer::process(const Image& input, Image& output)
{
    // Якщо шар вимкнений — просто копіюємо вхід на вихід
    if (!m_enabled || !m_effect || m_opacity <= 0.0f) {
        output = input.clone();
        return;
    }

    // Крок 1: Застосувати ефект
    Image effectResult;
    m_effect->apply(input, effectResult);

    // Крок 2: Якщо opacity == 1.0 і blend mode == Normal — просто повертаємо результат
    if (m_opacity >= 1.0f && m_blendMode == BlendMode::Normal) {
        output = std::move(effectResult);
        return;
    }

    // Крок 3: Змішати результат ефекту з оригіналом
    output = input.clone();
    uint8_t* outData = output.getData();
    const uint8_t* baseData = input.getData();
    const uint8_t* effectData = effectResult.getData();
    int totalPixels = input.getWidth() * input.getHeight();

    for (int i = 0; i < totalPixels; ++i) {
        int idx = i * 4;
        blendPixel(
            &baseData[idx],
            &effectData[idx],
            &outData[idx],
            m_opacity,
            m_blendMode
        );
    }
}

// ============================================================================
// blendPixel() — Змішати один піксель
// ============================================================================
void Layer::blendPixel(const uint8_t* base, const uint8_t* effect,
                       uint8_t* result, float opacity, BlendMode mode)
{
    for (int c = 0; c < 3; ++c) { // R, G, B
        float b = static_cast<float>(base[c]) / 255.0f;   // Базовий (0-1)
        float e = static_cast<float>(effect[c]) / 255.0f;  // Ефект (0-1)
        float blended = 0.0f;

        switch (mode) {
            case BlendMode::Normal:
                blended = e;
                break;

            case BlendMode::Add:
                // Додавання: base + effect (обмежене до 1.0)
                blended = std::min(b + e, 1.0f);
                break;

            case BlendMode::Multiply:
                // Множення: base * effect (завжди темніше)
                blended = b * e;
                break;

            case BlendMode::Screen:
                // Екран: 1 - (1-base) * (1-effect) (завжди яскравіше)
                blended = 1.0f - (1.0f - b) * (1.0f - e);
                break;

            case BlendMode::Overlay:
                // Накладення: комбінація Multiply та Screen
                if (b < 0.5f) {
                    blended = 2.0f * b * e;
                } else {
                    blended = 1.0f - 2.0f * (1.0f - b) * (1.0f - e);
                }
                break;
        }

        // Застосовуємо opacity: lerp між base та blended
        // lerp(a, b, t) = a + t * (b - a)
        float final_value = b + opacity * (blended - b);

        result[c] = static_cast<uint8_t>(std::clamp(final_value * 255.0f, 0.0f, 255.0f));
    }

    // Alpha канал — завжди від ефекту
    result[3] = effect[3];
}

// ============================================================================
// clone() — Дублювання шару
// ============================================================================
std::unique_ptr<Layer> Layer::clone() const
{
    auto copy = std::make_unique<Layer>(m_effect->clone(), m_name);
    copy->setOpacity(m_opacity);
    copy->setEnabled(m_enabled);
    copy->setBlendMode(m_blendMode);
    return copy;
}

} // namespace NoiseArt
