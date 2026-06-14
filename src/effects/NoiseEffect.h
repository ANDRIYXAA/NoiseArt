// ============================================================================
// NoiseArt — Ефект "Шум" (Noise)
// ============================================================================
// Додає випадковий шум до зображення — основний ефект нашого редактора!
//
// Типи шуму:
// - Color Noise: різні випадкові значення для R, G, B (кольоровий "телевізійний" шум)
// - Mono Noise: однакове значення для R, G, B (чорно-білі крапки)
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>
#include <random>    // Генератор випадкових чисел C++11
#include <algorithm> // std::clamp

namespace NoiseArt {

class NoiseEffect : public Effect {
public:
    std::string getName() const override { return "Noise"; }
    std::string getCategory() const override { return "Generate"; }

    void apply(const Image& input, Image& output) override
    {
        output = input.clone();
        uint8_t* pixels = output.getData();
        int totalPixels = output.getWidth() * output.getHeight();

        // Створюємо генератор випадкових чисел
        // mt19937 — алгоритм Мерсенна-Твістера (швидкий і якісний)
        // m_seed — "зерно", яке визначає послідовність чисел
        // Одне й те саме зерно = одна й та сама послідовність (для відтворюваності)
        std::mt19937 rng(m_seed);

        // uniform_int_distribution — рівномірний розподіл цілих чисел
        // від -maxNoise до +maxNoise
        int maxNoise = static_cast<int>(m_intensity * 2.55f); // 0-255
        if (maxNoise == 0) return; // Інтенсивність 0 = нічого не робити

        std::uniform_int_distribution<int> dist(-maxNoise, maxNoise);

        for (int i = 0; i < totalPixels; ++i) {
            int idx = i * 4;

            if (m_monochrome) {
                // Монохромний шум: одне значення для всіх каналів
                int noise = dist(rng);
                for (int c = 0; c < 3; ++c) {
                    int value = static_cast<int>(pixels[idx + c]) + noise;
                    pixels[idx + c] = static_cast<uint8_t>(std::clamp(value, 0, 255));
                }
            } else {
                // Кольоровий шум: різне значення для кожного каналу
                for (int c = 0; c < 3; ++c) {
                    int noise = dist(rng);
                    int value = static_cast<int>(pixels[idx + c]) + noise;
                    pixels[idx + c] = static_cast<uint8_t>(std::clamp(value, 0, 255));
                }
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;

        ImGui::SliderFloat("Intensity", &m_intensity, 0.0f, 100.0f, "%.1f%%");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        
        changed |= ImGui::Checkbox("Monochrome", &m_monochrome);
        changed |= ImGui::InputInt("Seed", &m_seed);

        if (ImGui::Button("Random Seed")) {
            // Використовуємо random_device для отримання справді випадкового числа
            std::random_device rd;
            m_seed = static_cast<int>(rd());
            changed = true;
        }

        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<NoiseEffect>();
        copy->m_intensity = m_intensity;
        copy->m_monochrome = m_monochrome;
        copy->m_seed = m_seed;
        return copy;
    }

private:
    float m_intensity = 25.0f;  // 0-100%
    bool m_monochrome = false;
    int m_seed = 42;            // "Зерно" генератора
};

} // namespace NoiseArt
