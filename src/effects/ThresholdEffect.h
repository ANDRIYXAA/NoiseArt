// ============================================================================
// NoiseArt — ThresholdEffect (Чорно-білий поріг)
// ============================================================================
// Перетворює зображення на чисто чорно-біле (без відтінків сірого).
// Якщо яскравість пікселя > threshold, він стає білим, інакше — чорним.
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>

namespace NoiseArt {

class ThresholdEffect : public Effect {
public:
    std::string getName() const override { return "Threshold"; }
    std::string getCategory() const override { return "Color"; }

    void apply(const Image& input, Image& output) override
    {
        output = input.clone();
        uint8_t* outData = output.getData();
        int totalPixels = input.getWidth() * input.getHeight();
        int ch = input.getChannels();

        float thresholdVal = m_threshold * 2.55f; // 0-100 -> 0-255

        for (int i = 0; i < totalPixels; ++i) {
            int idx = i * ch;
            
            // Обчислюємо яскравість (Luminance) з урахуванням сприйняття оком
            // Око найбільш чутливе до зеленого кольору
            float luminance = 0.299f * outData[idx + 0] + 
                              0.587f * outData[idx + 1] + 
                              0.114f * outData[idx + 2];

            uint8_t result = (luminance >= thresholdVal) ? 255 : 0;

            outData[idx + 0] = result;
            outData[idx + 1] = result;
            outData[idx + 2] = result;
            // Альфа-канал залишаємо без змін
        }
    }

    bool renderUI() override
    {
        bool changed = false;
        ImGui::SliderFloat("Threshold", &m_threshold, 0.0f, 100.0f, "%.1f%%");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<ThresholdEffect>();
        copy->m_threshold = m_threshold;
        return copy;
    }

private:
    float m_threshold = 50.0f;
};

} // namespace NoiseArt
