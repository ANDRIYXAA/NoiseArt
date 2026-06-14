// ============================================================================
// NoiseArt — VHSEffect (Ефект відеокасети)
// ============================================================================
// Додає хроматичну аберацію (розбіжність каналів RGB), шум та смуги сканування.
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>
#include <algorithm>
#include <random>

namespace NoiseArt {

class VHSEffect : public Effect {
public:
    std::string getName() const override { return "VHS / Glitch"; }
    std::string getCategory() const override { return "Retro"; }

    void apply(const Image& input, Image& output) override
    {
        output = input.clone();
        uint8_t* outData = output.getData();
        const uint8_t* inData = input.getData();
        int w = input.getWidth();
        int h = input.getHeight();
        int ch = input.getChannels();

        std::mt19937 rng(42); // фіксований сід для стабільності
        std::uniform_real_distribution<float> noiseDist(-m_noiseIntensity, m_noiseIntensity);

        for (int y = 0; y < h; ++y) {
            
            // Scanlines: затемнюємо кожен парний/непарний рядок
            float scanlineMult = 1.0f;
            if (m_scanlines > 0.0f && y % 3 == 0) {
                scanlineMult = 1.0f - m_scanlines;
            }

            for (int x = 0; x < w; ++x) {
                int outIdx = (y * w + x) * ch;

                // RGB Shift (Chromatic Aberration)
                int rx = std::clamp(x + m_shiftX, 0, w - 1);
                int ry = std::clamp(y + m_shiftY, 0, h - 1);
                
                int bx = std::clamp(x - m_shiftX, 0, w - 1);
                int by = std::clamp(y - m_shiftY, 0, h - 1);

                int rIdx = (ry * w + rx) * ch;
                int gIdx = (y * w + x) * ch;
                int bIdx = (by * w + bx) * ch;

                float r = inData[rIdx + 0];
                float g = inData[gIdx + 1];
                float b = inData[bIdx + 2];

                // Apply Scanlines
                r *= scanlineMult;
                g *= scanlineMult;
                b *= scanlineMult;

                // Apply Noise
                if (m_noiseIntensity > 0.0f) {
                    float n = noiseDist(rng) * 255.0f;
                    r += n; g += n; b += n;
                }

                outData[outIdx + 0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
                outData[outIdx + 1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
                outData[outIdx + 2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;
        ImGui::SliderInt("RGB Shift X", &m_shiftX, 0, 50);
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        ImGui::SliderInt("RGB Shift Y", &m_shiftY, 0, 50);
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        ImGui::SliderFloat("Scanlines", &m_scanlines, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        ImGui::SliderFloat("Noise", &m_noiseIntensity, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        
        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<VHSEffect>();
        copy->m_shiftX = m_shiftX;
        copy->m_shiftY = m_shiftY;
        copy->m_scanlines = m_scanlines;
        copy->m_noiseIntensity = m_noiseIntensity;
        return copy;
    }

private:
    int m_shiftX = 10;
    int m_shiftY = 0;
    float m_scanlines = 0.3f;
    float m_noiseIntensity = 0.1f;
};

} // namespace NoiseArt
