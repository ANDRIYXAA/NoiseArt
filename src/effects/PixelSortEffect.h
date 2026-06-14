// ============================================================================
// NoiseArt — PixelSortEffect (Глітч сортування пікселів)
// ============================================================================
// Сортує пікселі за яскравістю у вертикальних або горизонтальних лініях.
// ============================================================================

#pragma once

#include "Effect.h"
#include <imgui.h>
#include <algorithm>
#include <vector>

namespace NoiseArt {

class PixelSortEffect : public Effect {
public:
    std::string getName() const override { return "Pixel Sort"; }
    std::string getCategory() const override { return "Glitch"; }

    void apply(const Image& input, Image& output) override
    {
        output = input.clone();
        uint8_t* outData = output.getData();
        int w = input.getWidth();
        int h = input.getHeight();
        int ch = input.getChannels();

        struct Pixel {
            uint8_t r, g, b, a;
            float lum;
            bool operator<(const Pixel& other) const {
                return lum < other.lum;
            }
        };

        float thresh = m_threshold * 255.0f;

        if (m_vertical) {
            std::vector<Pixel> col(h);
            for (int x = 0; x < w; ++x) {
                // Копіюємо колонку
                for (int y = 0; y < h; ++y) {
                    int idx = (y * w + x) * ch;
                    col[y].r = outData[idx+0];
                    col[y].g = outData[idx+1];
                    col[y].b = outData[idx+2];
                    col[y].a = outData[idx+3];
                    col[y].lum = 0.299f*col[y].r + 0.587f*col[y].g + 0.114f*col[y].b;
                }

                // Сортуємо сегменти
                int startY = -1;
                for (int y = 0; y < h; ++y) {
                    if (col[y].lum > thresh && startY == -1) startY = y;
                    if (col[y].lum <= thresh && startY != -1) {
                        std::sort(col.begin() + startY, col.begin() + y);
                        startY = -1;
                    }
                }
                if (startY != -1) std::sort(col.begin() + startY, col.end());

                // Записуємо назад
                for (int y = 0; y < h; ++y) {
                    int idx = (y * w + x) * ch;
                    outData[idx+0] = col[y].r;
                    outData[idx+1] = col[y].g;
                    outData[idx+2] = col[y].b;
                    outData[idx+3] = col[y].a;
                }
            }
        } else {
            std::vector<Pixel> row(w);
            for (int y = 0; y < h; ++y) {
                // Копіюємо рядок
                for (int x = 0; x < w; ++x) {
                    int idx = (y * w + x) * ch;
                    row[x].r = outData[idx+0];
                    row[x].g = outData[idx+1];
                    row[x].b = outData[idx+2];
                    row[x].a = outData[idx+3];
                    row[x].lum = 0.299f*row[x].r + 0.587f*row[x].g + 0.114f*row[x].b;
                }

                // Сортуємо сегменти
                int startX = -1;
                for (int x = 0; x < w; ++x) {
                    if (row[x].lum > thresh && startX == -1) startX = x;
                    if (row[x].lum <= thresh && startX != -1) {
                        std::sort(row.begin() + startX, row.begin() + x);
                        startX = -1;
                    }
                }
                if (startX != -1) std::sort(row.begin() + startX, row.end());

                // Записуємо назад
                for (int x = 0; x < w; ++x) {
                    int idx = (y * w + x) * ch;
                    outData[idx+0] = row[x].r;
                    outData[idx+1] = row[x].g;
                    outData[idx+2] = row[x].b;
                    outData[idx+3] = row[x].a;
                }
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;
        ImGui::SliderFloat("Threshold", &m_threshold, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();
        if (ImGui::Checkbox("Vertical Sort", &m_vertical)) changed = true;
        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<PixelSortEffect>();
        copy->m_threshold = m_threshold;
        copy->m_vertical = m_vertical;
        return copy;
    }

private:
    float m_threshold = 0.5f;
    bool m_vertical = true;
};

} // namespace NoiseArt
