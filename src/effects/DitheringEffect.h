// ============================================================================
// NoiseArt — DitheringEffect (1-бітний ретро-стиль)
// ============================================================================
// Реалізує Ordered Dithering (матриця Байєра), як у старих DOS-іграх.
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>
#include <algorithm>

namespace NoiseArt {

class DitheringEffect : public Effect {
public:
    std::string getName() const override { return "Dithering"; }
    std::string getCategory() const override { return "Retro"; }

    void apply(const Image& input, Image& output) override
    {
        output = input.clone();
        uint8_t* outData = output.getData();
        int w = input.getWidth();
        int h = input.getHeight();
        int ch = input.getChannels();

        // 8x8 Bayer Matrix
        const int bayer[8][8] = {
            { 0, 32,  8, 40,  2, 34, 10, 42},
            {48, 16, 56, 24, 50, 18, 58, 26},
            {12, 44,  4, 36, 14, 46,  6, 38},
            {60, 28, 52, 20, 62, 30, 54, 22},
            { 3, 35, 11, 43,  1, 33,  9, 41},
            {51, 19, 59, 27, 49, 17, 57, 25},
            {15, 47,  7, 39, 13, 45,  5, 37},
            {63, 31, 55, 23, 61, 29, 53, 21}
        };

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = (y * w + x) * ch;

                int px = x / m_pixelSize;
                int py = y / m_pixelSize;

                float threshold = (bayer[py % 8][px % 8] + 0.5f) / 64.0f;
                // m_spread (0.0 to 1.0) controls how much the dither affects the image
                threshold = (threshold - 0.5f) * m_spread + 0.5f;

                if (m_monochrome) {
                    float lum = 0.299f * outData[idx+0] + 0.587f * outData[idx+1] + 0.114f * outData[idx+2];
                    lum /= 255.0f;
                    uint8_t val = (lum > threshold) ? 255 : 0;
                    outData[idx+0] = val;
                    outData[idx+1] = val;
                    outData[idx+2] = val;
                } else {
                    outData[idx+0] = ((outData[idx+0] / 255.0f) > threshold) ? 255 : 0;
                    outData[idx+1] = ((outData[idx+1] / 255.0f) > threshold) ? 255 : 0;
                    outData[idx+2] = ((outData[idx+2] / 255.0f) > threshold) ? 255 : 0;
                }
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;
        ImGui::SliderFloat("Spread", &m_spread, 0.0f, 2.0f, "%.2f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        
        ImGui::SliderInt("Pixel Size", &m_pixelSize, 1, 16);
        changed |= ImGui::IsItemDeactivatedAfterEdit();

        if (ImGui::Checkbox("Monochrome", &m_monochrome)) {
            changed = true;
        }
        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<DitheringEffect>();
        copy->m_spread = m_spread;
        copy->m_pixelSize = m_pixelSize;
        copy->m_monochrome = m_monochrome;
        return copy;
    }

private:
    float m_spread = 1.0f;
    int m_pixelSize = 1;
    bool m_monochrome = true;
};

} // namespace NoiseArt
