// ============================================================================
// NoiseArt — Ефект "Інверсія кольорів" (Заголовок + Реалізація)
// ============================================================================
// Найпростіший ефект — інвертує кольори (негатив).
// Формула: новий_піксель = 255 - старий_піксель
// Білий стає чорним, червоний стає блакитним тощо.
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>

namespace NoiseArt {

class InvertEffect : public Effect {
public:
    // ----- Інформація -----
    std::string getName() const override { return "Invert"; }
    std::string getCategory() const override { return "Color"; }

    // ----- Застосування ефекту -----
    void apply(const Image& input, Image& output) override
    {
        // Створюємо копію вхідного зображення
        output = input.clone();

        // Отримуємо вказівник на масив пікселів
        uint8_t* pixels = output.getData();
        int totalPixels = output.getWidth() * output.getHeight();

        // Проходимо по кожному пікселю
        for (int i = 0; i < totalPixels; ++i) {
            int idx = i * 4; // Кожен піксель = 4 байти (R, G, B, A)

            // Інвертуємо тільки вибрані канали
            if (m_invertR) pixels[idx + 0] = 255 - pixels[idx + 0]; // Red
            if (m_invertG) pixels[idx + 1] = 255 - pixels[idx + 1]; // Green
            if (m_invertB) pixels[idx + 2] = 255 - pixels[idx + 2]; // Blue
            // Alpha (прозорість) НЕ інвертуємо — це зазвичай небажано
        }
    }

    // ----- UI -----
    bool renderUI() override
    {
        bool changed = false;

        // ImGui::Checkbox — галочка (чекбокс)
        // Повертає true, якщо стан змінився (натиснули)
        changed |= ImGui::Checkbox("Red (R)", &m_invertR);
        changed |= ImGui::Checkbox("Green (G)", &m_invertG);
        changed |= ImGui::Checkbox("Blue (B)", &m_invertB);

        // Кнопка "Інвертувати всі"
        if (ImGui::Button("Toggle All")) {
            bool allOn = m_invertR && m_invertG && m_invertB;
            m_invertR = m_invertG = m_invertB = !allOn;
            changed = true;
        }

        return changed;
    }

    // ----- Клонування -----
    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<InvertEffect>();
        copy->m_invertR = m_invertR;
        copy->m_invertG = m_invertG;
        copy->m_invertB = m_invertB;
        return copy;
    }

private:
    // Параметри: які канали інвертувати
    bool m_invertR = true;
    bool m_invertG = true;
    bool m_invertB = true;
};

} // namespace NoiseArt
