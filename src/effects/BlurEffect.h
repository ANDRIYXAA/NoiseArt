// ============================================================================
// NoiseArt — Ефект "Розмиття" (Gaussian Blur)
// ============================================================================
// Розмиває зображення, усереднюючи кольори сусідніх пікселів.
//
// Алгоритм: Box Blur (двопрохідний — спочатку горизонтально, потім вертикально).
// Це O(n) замість O(n²) — набагато швидше!
//
// Як це працює:
// 1. Для кожного пікселя беремо його сусідів зліва та справа (горизонтальний прохід)
// 2. Обчислюємо середнє значення
// 3. Повторюємо те саме вертикально
// Результат — зображення стає "м'якшим" (розмитим)
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>
#include <algorithm>
#include <vector>

namespace NoiseArt {

class BlurEffect : public Effect {
public:
    std::string getName() const override { return "Blur"; }
    std::string getCategory() const override { return "Blur"; }

    void apply(const Image& input, Image& output) override
    {
        if (m_radius <= 0) {
            output = input.clone();
            return;
        }

        int w = input.getWidth();
        int h = input.getHeight();
        int ch = input.getChannels();

        // Створюємо тимчасове зображення для проміжного результату
        Image temp;
        temp.create(w, h, ch);
        output = input.clone();

        const uint8_t* src = input.getData();
        uint8_t* tmp = temp.getData();
        uint8_t* dst = output.getData();

        // ===== Прохід 1: Горизонтальне розмиття =====
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float r = 0, g = 0, b = 0, a = 0;
                int count = 0;

                // Проходимо по сусідах зліва до справа
                for (int dx = -m_radius; dx <= m_radius; ++dx) {
                    int nx = std::clamp(x + dx, 0, w - 1);
                    int idx = (y * w + nx) * ch;
                    r += src[idx + 0];
                    g += src[idx + 1];
                    b += src[idx + 2];
                    a += src[idx + 3];
                    count++;
                }

                // Середнє значення
                int idx = (y * w + x) * ch;
                tmp[idx + 0] = static_cast<uint8_t>(r / count);
                tmp[idx + 1] = static_cast<uint8_t>(g / count);
                tmp[idx + 2] = static_cast<uint8_t>(b / count);
                tmp[idx + 3] = static_cast<uint8_t>(a / count);
            }
        }

        // ===== Прохід 2: Вертикальне розмиття =====
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                float r = 0, g = 0, b = 0, a = 0;
                int count = 0;

                for (int dy = -m_radius; dy <= m_radius; ++dy) {
                    int ny = std::clamp(y + dy, 0, h - 1);
                    int idx = (ny * w + x) * ch;
                    r += tmp[idx + 0];
                    g += tmp[idx + 1];
                    b += tmp[idx + 2];
                    a += tmp[idx + 3];
                    count++;
                }

                int idx = (y * w + x) * ch;
                dst[idx + 0] = static_cast<uint8_t>(r / count);
                dst[idx + 1] = static_cast<uint8_t>(g / count);
                dst[idx + 2] = static_cast<uint8_t>(b / count);
                dst[idx + 3] = static_cast<uint8_t>(a / count);
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;
        ImGui::SliderInt("Radius", &m_radius, 0, 50);
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<BlurEffect>();
        copy->m_radius = m_radius;
        return copy;
    }

private:
    int m_radius = 3; // Радіус розмиття (1-50 пікселів)
};

} // namespace NoiseArt
