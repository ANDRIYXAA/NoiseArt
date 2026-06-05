// ============================================================================
// NoiseArt — Ефект "Сяйво" (Bloom)
// ============================================================================
// Bloom створює ефект "свічення" навколо яскравих ділянок зображення.
// Часто використовується в іграх та кіно для надання "мрійливого" вигляду.
//
// Алгоритм:
// 1. Витягнути яскраві пікселі (вище порогу threshold)
// 2. Розмити їх (blur)
// 3. Додати розмиті яскраві пікселі назад до оригіналу
// ============================================================================

#pragma once

#include "Effect.h"
#include "BlurEffect.h" // Використовуємо вже готовий BlurEffect!
#include <imgui.h>
#include <algorithm>

namespace NoiseArt {

class BloomEffect : public Effect {
public:
    std::string getName() const override { return "Bloom"; }
    std::string getCategory() const override { return "Stylize"; }

    void apply(const Image& input, Image& output) override
    {
        int w = input.getWidth();
        int h = input.getHeight();
        int ch = input.getChannels();

        // Крок 1: Витягнути яскраві пікселі
        Image bright;
        bright.create(w, h, ch);
        uint8_t* brightData = bright.getData();
        const uint8_t* srcData = input.getData();

        float threshold = m_threshold * 2.55f; // 0-255

        for (int i = 0; i < w * h; ++i) {
            int idx = i * ch;
            // Яскравість пікселя = середнє R, G, B
            float luminance = (srcData[idx] + srcData[idx + 1] + srcData[idx + 2]) / 3.0f;

            if (luminance > threshold) {
                // Яскравий піксель — залишаємо
                brightData[idx + 0] = srcData[idx + 0];
                brightData[idx + 1] = srcData[idx + 1];
                brightData[idx + 2] = srcData[idx + 2];
                brightData[idx + 3] = 255;
            } else {
                // Темний піксель — чорний
                brightData[idx + 0] = 0;
                brightData[idx + 1] = 0;
                brightData[idx + 2] = 0;
                brightData[idx + 3] = 255;
            }
        }

        // Крок 2: Розмити яскраві пікселі
        // Створюємо тимчасовий BlurEffect з нашим радіусом
        BlurEffect blur;
        // Використовуємо рефлексію для встановлення радіусу через UI-метод
        Image blurred;
        // Прямо застосовуємо простий бокс-блюр кілька разів для м'якості
        Image current = bright.clone();
        for (int pass = 0; pass < m_passes; ++pass) {
            blur.apply(current, blurred);
            current = blurred.clone();
        }

        // Крок 3: Додати розмиті яскраві пікселі до оригіналу
        output = input.clone();
        uint8_t* outData = output.getData();
        const uint8_t* blurData = current.getData();

        for (int i = 0; i < w * h; ++i) {
            int idx = i * ch;
            for (int c = 0; c < 3; ++c) {
                float original = static_cast<float>(outData[idx + c]);
                float bloom = static_cast<float>(blurData[idx + c]) * m_intensity;
                outData[idx + c] = static_cast<uint8_t>(std::clamp(original + bloom, 0.0f, 255.0f));
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;
        changed |= ImGui::SliderFloat("Threshold", &m_threshold, 0.0f, 100.0f, "%.1f%%");
        changed |= ImGui::SliderFloat("Intensity", &m_intensity, 0.0f, 2.0f, "%.2f");
        changed |= ImGui::SliderInt("Blur Passes", &m_passes, 1, 5);
        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<BloomEffect>();
        copy->m_threshold = m_threshold;
        copy->m_intensity = m_intensity;
        copy->m_passes = m_passes;
        return copy;
    }

private:
    float m_threshold = 50.0f;  // Поріг яскравості (0-100%)
    float m_intensity = 0.5f;   // Інтенсивність (0-2)
    int m_passes = 2;           // Кількість проходів розмиття
};

} // namespace NoiseArt
