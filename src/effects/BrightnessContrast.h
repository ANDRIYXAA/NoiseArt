// ============================================================================
// NoiseArt — Ефект "Яскравість / Контраст"
// ============================================================================
// Два найбазовіших параметри обробки зображень:
//
// Яскравість (Brightness): додаємо або віднімаємо значення від кожного пікселя.
//   +50 → все стає світлішим
//   -50 → все стає темнішим
//
// Контраст (Contrast): збільшуємо або зменшуємо різницю між кольорами.
//   +50 → темні стають темнішими, світлі — світлішими
//   -50 → все стає більш "сірим"
//
// Формула: result = clamp(factor * (pixel - 128) + 128 + brightness, 0, 255)
// де factor = (259 * (contrast + 255)) / (255 * (259 - contrast))
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>
#include <algorithm> // std::clamp

namespace NoiseArt {

class BrightnessContrast : public Effect {
public:
    std::string getName() const override { return "Brightness/Contrast"; }
    std::string getCategory() const override { return "Color"; }

    void apply(const Image& input, Image& output) override
    {
        output = input.clone();
        uint8_t* pixels = output.getData();
        int totalPixels = output.getWidth() * output.getHeight();

        // Розраховуємо фактор контрасту
        // Ця формула перетворює діапазон [-100, +100] у множник
        float contrastMapped = m_contrast * 2.55f; // -255..+255
        float factor = (259.0f * (contrastMapped + 255.0f)) / (255.0f * (259.0f - contrastMapped));

        for (int i = 0; i < totalPixels; ++i) {
            int idx = i * 4;

            for (int c = 0; c < 3; ++c) { // R, G, B (не A)
                float value = static_cast<float>(pixels[idx + c]);

                // 1. Контраст: відносно середнього (128)
                value = factor * (value - 128.0f) + 128.0f;

                // 2. Яскравість: просто додаємо
                value += m_brightness * 2.55f; // Масштабуємо до 0-255

                // 3. Обмежуємо діапазон 0-255
                // std::clamp(value, min, max) — якщо value < min → min,
                // якщо value > max → max, інакше → value
                pixels[idx + c] = static_cast<uint8_t>(std::clamp(value, 0.0f, 255.0f));
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;

        // ImGui::SliderFloat — повзунок (слайдер) для числа з комою
        // "%.1f" — формат виводу (одна цифра після коми)
        ImGui::SliderFloat("Brightness", &m_brightness, -100.0f, 100.0f, "%.1f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        ImGui::SliderFloat("Contrast", &m_contrast, -100.0f, 100.0f, "%.1f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();

        // Кнопка скидання
        if (ImGui::Button("Reset")) {
            m_brightness = 0.0f;
            m_contrast = 0.0f;
            changed = true;
        }

        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<BrightnessContrast>();
        copy->m_brightness = m_brightness;
        copy->m_contrast = m_contrast;
        return copy;
    }

private:
    float m_brightness = 0.0f;  // -100..+100
    float m_contrast = 0.0f;    // -100..+100
};

} // namespace NoiseArt
