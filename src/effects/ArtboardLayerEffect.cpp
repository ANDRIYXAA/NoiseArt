#include "ArtboardLayerEffect.h"
#include <imgui.h>
#include <cmath>

namespace NoiseArt {

ArtboardLayerEffect::ArtboardLayerEffect() {}

void ArtboardLayerEffect::apply(const Image& input, Image& output) {
    output = input;
}

void ArtboardLayerEffect::renderVector(NVGcontext* vg) {
    // Малюємо чекер
    float tileSize = 12.0f;
    
    // Тло (світлий колір)
    nvgBeginPath(vg);
    nvgRect(vg, m_x, m_y, m_width, m_height);
    nvgFillColor(vg, nvgRGBAf(200.0f/255.0f, 200.0f/255.0f, 200.0f/255.0f, m_opacity));
    nvgFill(vg);

    // Темні клітинки
    nvgBeginPath(vg);
    for (float y = m_y; y < m_y + m_height; y += tileSize * 2) {
        for (float x = m_x; x < m_x + m_width; x += tileSize * 2) {
            // Клітинка (0,0)
            float x1 = x;
            float y1 = y;
            if (x1 < m_x + m_width && y1 < m_y + m_height) {
                nvgRect(vg, x1, y1, std::min(tileSize, m_x + m_width - x1), std::min(tileSize, m_y + m_height - y1));
            }
            // Клітинка (1,1)
            float x2 = x + tileSize;
            float y2 = y + tileSize;
            if (x2 < m_x + m_width && y2 < m_y + m_height) {
                nvgRect(vg, x2, y2, std::min(tileSize, m_x + m_width - x2), std::min(tileSize, m_y + m_height - y2));
            }
        }
    }
    nvgFillColor(vg, nvgRGBAf(150.0f/255.0f, 150.0f/255.0f, 150.0f/255.0f, m_opacity));
    nvgFill(vg);

    // Рамка артборда
    nvgBeginPath(vg);
    nvgRect(vg, m_x, m_y, m_width, m_height);
    nvgStrokeColor(vg, nvgRGBAf(50.0f/255.0f, 50.0f/255.0f, 50.0f/255.0f, 1.0f));
    nvgStrokeWidth(vg, 2.0f);
    nvgStroke(vg);
}

bool ArtboardLayerEffect::renderUI() {
    bool changed = false;

    ImGui::SeparatorText("Artboard Size");
    if (ImGui::DragFloat2("Position", &m_x, 1.0f)) changed = true;
    if (ImGui::DragFloat2("Size", &m_width, 1.0f, 1.0f, 10000.0f)) changed = true;
    
    ImGui::SeparatorText("Display");
    if (ImGui::SliderFloat("Opacity", &m_opacity, 0.0f, 1.0f)) changed = true;

    return changed;
}

std::unique_ptr<Effect> ArtboardLayerEffect::clone() const {
    auto copy = std::make_unique<ArtboardLayerEffect>();
    copy->m_x = m_x;
    copy->m_y = m_y;
    copy->m_width = m_width;
    copy->m_height = m_height;
    copy->m_opacity = m_opacity;
    return copy;
}

} // namespace NoiseArt
