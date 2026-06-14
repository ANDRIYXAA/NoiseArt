// ============================================================================
// NoiseArt — AdjustmentsEffect (Коригування)
// ============================================================================
// Об'єднує базові налаштування:
// - Saturation (Насиченість)
// - Hue Rotation (Зсув відтінку)
// - Sharpness (Різкість через матрицю згортки 3x3)
// - Gamma (Гамма-корекція)
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace NoiseArt {

class AdjustmentsEffect : public Effect {
public:
    std::string getName() const override { return "Adjustments"; }
    std::string getCategory() const override { return "Color"; }

    void apply(const Image& input, Image& output) override
    {
        int w = input.getWidth();
        int h = input.getHeight();
        int ch = input.getChannels();
        
        output = input.clone();
        uint8_t* outData = output.getData();
        const uint8_t* inData = input.getData();

        // Попередньо обчислюємо гамму для швидкості (Lookup Table)
        uint8_t gammaLUT[256];
        float invGamma = 1.0f / (m_gamma > 0.01f ? m_gamma : 0.01f);
        for (int i = 0; i < 256; ++i) {
            float val = i / 255.0f;
            gammaLUT[i] = static_cast<uint8_t>(std::clamp(std::pow(val, invGamma) * 255.0f, 0.0f, 255.0f));
        }

        // Ядро різкості (Laplacian / Unsharp Mask)
        // [  0, -a,  0 ]
        // [ -a, 1+4a, -a ]
        // [  0, -a,  0 ]
        float a = m_sharpness; // 0.0 to 2.0
        float centerWeight = 1.0f + 4.0f * a;

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                int idx = (y * w + x) * ch;

                float r = 0, g = 0, b = 0;

                // 1. Sharpness (Різкість)
                if (m_sharpness > 0.01f) {
                    float sumR = 0, sumG = 0, sumB = 0;
                    
                    // Сусіди
                    int xLeft = std::max(0, x - 1);
                    int xRight = std::min(w - 1, x + 1);
                    int yTop = std::max(0, y - 1);
                    int yBottom = std::min(h - 1, y + 1);

                    // Центр
                    sumR += inData[(y * w + x) * ch + 0] * centerWeight;
                    sumG += inData[(y * w + x) * ch + 1] * centerWeight;
                    sumB += inData[(y * w + x) * ch + 2] * centerWeight;

                    // Зверху, знизу, зліва, справа
                    auto addNeighbor = [&](int nx, int ny) {
                        int nidx = (ny * w + nx) * ch;
                        sumR -= inData[nidx + 0] * a;
                        sumG -= inData[nidx + 1] * a;
                        sumB -= inData[nidx + 2] * a;
                    };
                    addNeighbor(x, yTop);
                    addNeighbor(x, yBottom);
                    addNeighbor(xLeft, y);
                    addNeighbor(xRight, y);

                    r = std::clamp(sumR, 0.0f, 255.0f);
                    g = std::clamp(sumG, 0.0f, 255.0f);
                    b = std::clamp(sumB, 0.0f, 255.0f);
                } else {
                    r = inData[idx + 0];
                    g = inData[idx + 1];
                    b = inData[idx + 2];
                }

                // 2. Hue & Saturation (через HSV)
                if (std::abs(m_hue) > 0.1f || std::abs(m_saturation - 1.0f) > 0.01f) {
                    float h_hsv, s_hsv, v_hsv;
                    RGBtoHSV(r / 255.0f, g / 255.0f, b / 255.0f, h_hsv, s_hsv, v_hsv);

                    h_hsv += m_hue;
                    if (h_hsv >= 360.0f) h_hsv -= 360.0f;
                    if (h_hsv < 0.0f) h_hsv += 360.0f;

                    s_hsv *= m_saturation;
                    s_hsv = std::clamp(s_hsv, 0.0f, 1.0f);

                    float r_out, g_out, b_out;
                    HSVtoRGB(h_hsv, s_hsv, v_hsv, r_out, g_out, b_out);
                    
                    r = r_out * 255.0f;
                    g = g_out * 255.0f;
                    b = b_out * 255.0f;
                }

                // 3. Gamma
                outData[idx + 0] = gammaLUT[static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f))];
                outData[idx + 1] = gammaLUT[static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f))];
                outData[idx + 2] = gammaLUT[static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f))];
                outData[idx + 3] = inData[idx + 3]; // Alpha
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;
        
        ImGui::SliderFloat("Saturation", &m_saturation, 0.0f, 3.0f, "%.2f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        
        ImGui::SliderFloat("Hue Rotation", &m_hue, -180.0f, 180.0f, "%.0f deg");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        
        ImGui::SliderFloat("Sharpness", &m_sharpness, 0.0f, 2.0f, "%.2f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        
        ImGui::SliderFloat("Gamma", &m_gamma, 0.1f, 3.0f, "%.2f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        
        if (ImGui::Button("Reset All")) {
            m_saturation = 1.0f;
            m_hue = 0.0f;
            m_sharpness = 0.0f;
            m_gamma = 1.0f;
            changed = true;
        }
        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<AdjustmentsEffect>();
        copy->m_saturation = m_saturation;
        copy->m_hue = m_hue;
        copy->m_sharpness = m_sharpness;
        copy->m_gamma = m_gamma;
        return copy;
    }

private:
    float m_saturation = 1.0f;
    float m_hue = 0.0f;
    float m_sharpness = 0.0f;
    float m_gamma = 1.0f;

    // Допоміжні функції конвертації
    static void RGBtoHSV(float r, float g, float b, float& h, float& s, float& v) {
        float min = std::min({r, g, b});
        float max = std::max({r, g, b});
        v = max;
        float delta = max - min;
        if (max != 0) s = delta / max; else { s = 0; h = -1; return; }
        if (r == max) h = (g - b) / delta;
        else if (g == max) h = 2 + (b - r) / delta;
        else h = 4 + (r - g) / delta;
        h *= 60;
        if (h < 0) h += 360;
    }

    static void HSVtoRGB(float h, float s, float v, float& r, float& g, float& b) {
        if (s == 0) { r = g = b = v; return; }
        h /= 60;
        int i = static_cast<int>(std::floor(h));
        float f = h - i;
        float p = v * (1 - s);
        float q = v * (1 - s * f);
        float t = v * (1 - s * (1 - f));
        switch (i) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            default: r = v; g = p; b = q; break;
        }
    }
};

} // namespace NoiseArt
