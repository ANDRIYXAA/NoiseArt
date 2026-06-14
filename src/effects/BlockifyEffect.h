// ============================================================================
// NoiseArt — BlockifyEffect (Пікселізація)
// ============================================================================
// Зменшує роздільну здатність зображення шляхом об'єднання пікселів у блоки.
// Ефект "ретро-ігор" або цензури.
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>
#include <algorithm>

namespace NoiseArt {

class BlockifyEffect : public Effect {
public:
    std::string getName() const override { return "Blockify"; }
    std::string getCategory() const override { return "Stylize"; }

    void apply(const Image& input, Image& output) override
    {
        if (m_blockSize <= 1) {
            output = input.clone();
            return;
        }

        output = input.clone();
        uint8_t* outData = output.getData();
        const uint8_t* inData = input.getData();
        
        int w = input.getWidth();
        int h = input.getHeight();
        int ch = input.getChannels();

        // Проходимо по зображенню з кроком blockSize
        for (int y = 0; y < h; y += m_blockSize) {
            for (int x = 0; x < w; x += m_blockSize) {
                
                // Знаходимо середній колір для блоку
                long sumR = 0, sumG = 0, sumB = 0, sumA = 0;
                int count = 0;
                
                int blockH = std::min(m_blockSize, h - y);
                int blockW = std::min(m_blockSize, w - x);

                for (int by = 0; by < blockH; ++by) {
                    for (int bx = 0; bx < blockW; ++bx) {
                        int idx = ((y + by) * w + (x + bx)) * ch;
                        sumR += inData[idx + 0];
                        sumG += inData[idx + 1];
                        sumB += inData[idx + 2];
                        sumA += inData[idx + 3];
                        count++;
                    }
                }

                uint8_t avgR = static_cast<uint8_t>(sumR / count);
                uint8_t avgG = static_cast<uint8_t>(sumG / count);
                uint8_t avgB = static_cast<uint8_t>(sumB / count);
                uint8_t avgA = static_cast<uint8_t>(sumA / count);

                // Заповнюємо блок цим середнім кольором
                for (int by = 0; by < blockH; ++by) {
                    for (int bx = 0; bx < blockW; ++bx) {
                        int idx = ((y + by) * w + (x + bx)) * ch;
                        outData[idx + 0] = avgR;
                        outData[idx + 1] = avgG;
                        outData[idx + 2] = avgB;
                        outData[idx + 3] = avgA;
                    }
                }
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;
        ImGui::SliderInt("Block Size", &m_blockSize, 1, 64);
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<BlockifyEffect>();
        copy->m_blockSize = m_blockSize;
        return copy;
    }

private:
    int m_blockSize = 8;
};

} // namespace NoiseArt
