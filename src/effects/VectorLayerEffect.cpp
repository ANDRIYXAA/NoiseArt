#include "VectorLayerEffect.h"
#include <imgui.h>

namespace NoiseArt {

VectorLayerEffect::VectorLayerEffect() {}

void VectorLayerEffect::apply(const Image& input, Image& output) {
    // Векторний шар не змінює растрових пікселів,
    // він просто пропускає їх далі. Вектори малюються ПОВЕРХ.
    output = input;
}

void VectorLayerEffect::renderVector(NVGcontext* vg) {
    nvgBeginPath(vg);

    switch (m_shapeType) {
        case ShapeType::Rectangle:
            nvgRect(vg, m_x, m_y, m_width, m_height);
            break;
        case ShapeType::Circle:
            // В NanoVG nvgCircle малює коло відносно ЦЕНТРУ
            nvgCircle(vg, m_x + m_width / 2.0f, m_y + m_height / 2.0f, m_width / 2.0f);
            break;
        case ShapeType::RoundedRectangle:
            nvgRoundedRect(vg, m_x, m_y, m_width, m_height, m_radius);
            break;
    }

    if (m_fill) {
        nvgFillColor(vg, nvgRGBAf(m_color[0], m_color[1], m_color[2], m_color[3]));
        nvgFill(vg);
    }

    if (m_stroke) {
        nvgStrokeColor(vg, nvgRGBAf(m_strokeColor[0], m_strokeColor[1], m_strokeColor[2], m_strokeColor[3]));
        nvgStrokeWidth(vg, m_strokeWidth);
        nvgStroke(vg);
    }
}

bool VectorLayerEffect::renderUI() {
    bool changed = false;

    int shape = static_cast<int>(m_shapeType);
    const char* shapes[] = { "Rectangle", "Circle", "Rounded Rectangle" };
    if (ImGui::Combo("Shape", &shape, shapes, 3)) {
        m_shapeType = static_cast<ShapeType>(shape);
        changed = true;
    }

    ImGui::SeparatorText("Transform");
    if (ImGui::DragFloat2("Position", &m_x, 1.0f)) changed = true;
    if (ImGui::DragFloat2("Size", &m_width, 1.0f, 1.0f, 10000.0f)) changed = true;
    
    if (m_shapeType == ShapeType::RoundedRectangle) {
        if (ImGui::DragFloat("Corner Radius", &m_radius, 0.5f, 0.0f, 1000.0f)) changed = true;
    }

    ImGui::SeparatorText("Style");
    if (ImGui::Checkbox("Fill", &m_fill)) changed = true;
    if (m_fill) {
        if (ImGui::ColorEdit4("Fill Color", m_color)) changed = true;
    }

    if (ImGui::Checkbox("Stroke", &m_stroke)) changed = true;
    if (m_stroke) {
        if (ImGui::ColorEdit4("Stroke Color", m_strokeColor)) changed = true;
        if (ImGui::DragFloat("Stroke Width", &m_strokeWidth, 0.5f, 0.0f, 100.0f)) changed = true;
    }

    return changed;
}

std::unique_ptr<Effect> VectorLayerEffect::clone() const {
    auto copy = std::make_unique<VectorLayerEffect>();
    copy->m_shapeType = m_shapeType;
    copy->m_x = m_x;
    copy->m_y = m_y;
    copy->m_width = m_width;
    copy->m_height = m_height;
    copy->m_radius = m_radius;
    copy->m_fill = m_fill;
    copy->m_stroke = m_stroke;
    copy->m_strokeWidth = m_strokeWidth;
    for(int i=0; i<4; i++) {
        copy->m_color[i] = m_color[i];
        copy->m_strokeColor[i] = m_strokeColor[i];
    }
    return copy;
}

} // namespace NoiseArt
