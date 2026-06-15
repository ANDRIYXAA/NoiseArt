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
        int w = output.getWidth();
        int h = output.getHeight();

        int maxNoise = static_cast<int>(m_intensity * 2.55f); // 0-255
        if (maxNoise == 0) return; // Інтенсивність 0 = нічого не робити

        int grain = (m_grainSize < 1) ? 1 : m_grainSize;
        uint32_t seed = static_cast<uint32_t>(m_seed);
        uint32_t range = static_cast<uint32_t>(2 * maxNoise + 1);

        // Хеш координат ЗЕРНА → детерміноване значення шуму.
        // Усі пікселі в одній клітинці grain×grain отримують однаковий шум.
        auto noiseAt = [&](uint32_t gx, uint32_t gy, uint32_t c) -> int {
            uint32_t hsh = seed + 0x9E3779B9u;
            hsh ^= gx * 374761393u;  hsh = (hsh << 13) | (hsh >> 19);
            hsh ^= gy * 668265263u;  hsh = (hsh << 13) | (hsh >> 19);
            hsh ^= c  * 2246822519u; hsh *= 2654435761u;
            hsh ^= hsh >> 15;
            return static_cast<int>(hsh % range) - maxNoise;
        };

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = (y * w + x) * 4;
                uint32_t gx = static_cast<uint32_t>(x / grain);
                uint32_t gy = static_cast<uint32_t>(y / grain);

                if (m_monochrome) {
                    // Монохромний шум: одне значення на всі канали
                    int noise = noiseAt(gx, gy, 0u);
                    for (int c = 0; c < 3; ++c) {
                        int value = static_cast<int>(pixels[idx + c]) + noise;
                        pixels[idx + c] = static_cast<uint8_t>(std::clamp(value, 0, 255));
                    }
                } else {
                    // Кольоровий шум: своє значення для кожного каналу
                    for (int c = 0; c < 3; ++c) {
                        int noise = noiseAt(gx, gy, static_cast<uint32_t>(c) + 1u);
                        int value = static_cast<int>(pixels[idx + c]) + noise;
                        pixels[idx + c] = static_cast<uint8_t>(std::clamp(value, 0, 255));
                    }
                }
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;

        ImGui::SliderFloat("Intensity", &m_intensity, 0.0f, 100.0f, "%.1f%%");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        
        ImGui::SliderInt("Grain Size", &m_grainSize, 1, 64, "%d px");
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
        copy->m_grainSize = m_grainSize;
        return copy;
    }

private:
    float m_intensity = 25.0f;  // 0-100%
    bool m_monochrome = false;
    int m_seed = 42;            // "Зерно" генератора
    int m_grainSize = 1;        // Розмір зерна шуму в пікселях (1 = попіксельно)
};

} // namespace NoiseArt
