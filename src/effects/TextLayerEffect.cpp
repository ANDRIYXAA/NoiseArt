#include "TextLayerEffect.h"
#include <imgui.h>
#include <cstring>

namespace NoiseArt {

TextLayerEffect::TextLayerEffect() {
    std::strncpy(m_textBuffer, m_text.c_str(), sizeof(m_textBuffer));
    m_textBuffer[sizeof(m_textBuffer) - 1] = '\0';
}

void TextLayerEffect::apply(const Image& input, Image& output) {
    output = input; // Vector layer, raster is untouched
}

void TextLayerEffect::renderVector(NVGcontext* vg) {
    if (m_text.empty()) return;

    nvgFontSize(vg, m_fontSize);
    
    // Використовуємо вибраний шрифт
    const char* fontName = (m_fontIndex >= 0 && m_fontIndex < FontCount) 
        ? FontNames[m_fontIndex] : "Inter";
    nvgFontFace(vg, fontName);
    
    int alignFlags = NVG_ALIGN_TOP;
    if (m_align == Align::Left) alignFlags |= NVG_ALIGN_LEFT;
    else if (m_align == Align::Center) alignFlags |= NVG_ALIGN_CENTER;
    else if (m_align == Align::Right) alignFlags |= NVG_ALIGN_RIGHT;
    
    nvgTextAlign(vg, alignFlags);
    nvgFillColor(vg, nvgRGBAf(m_color[0], m_color[1], m_color[2], m_color[3]));
    
    nvgText(vg, m_x, m_y, m_text.c_str(), nullptr);
}

bool TextLayerEffect::renderUI() {
    bool changed = false;

    if (ImGui::InputText("Text", m_textBuffer, sizeof(m_textBuffer))) {
        m_text = m_textBuffer;
        changed = true;
    }

    ImGui::SeparatorText("Font");
    
    // Вибір шрифту
    if (ImGui::Combo("Font", &m_fontIndex, FontNames, FontCount)) {
        changed = true;
    }
    
    if (ImGui::DragFloat("Font Size", &m_fontSize, 1.0f, 8.0f, 1000.0f)) changed = true;

    int align = static_cast<int>(m_align);
    const char* aligns[] = { "Left", "Center", "Right" };
    if (ImGui::Combo("Alignment", &align, aligns, 3)) {
        m_align = static_cast<Align>(align);
        changed = true;
    }

    ImGui::SeparatorText("Transform");
    if (ImGui::DragFloat2("Position", &m_x, 1.0f)) changed = true;

    ImGui::SeparatorText("Style");
    if (ImGui::ColorEdit4("Color", m_color)) changed = true;

    return changed;
}

std::unique_ptr<Effect> TextLayerEffect::clone() const {
    auto copy = std::make_unique<TextLayerEffect>();
    copy->m_text = m_text;
    std::strncpy(copy->m_textBuffer, m_text.c_str(), sizeof(copy->m_textBuffer));
    copy->m_textBuffer[sizeof(copy->m_textBuffer) - 1] = '\0';
    copy->m_x = m_x;
    copy->m_y = m_y;
    copy->m_fontSize = m_fontSize;
    copy->m_fontIndex = m_fontIndex;
    copy->m_align = m_align;
    copy->m_bold = m_bold;
    for(int i=0; i<4; ++i) copy->m_color[i] = m_color[i];
    return copy;
}

float TextLayerEffect::getWidth() const {
    // Приблизна оцінка ширини
    return m_text.length() * m_fontSize * 0.5f;
}

float TextLayerEffect::getHeight() const {
    return m_fontSize;
}

void TextLayerEffect::setSize(float w, float h) {
    // При зміні розміру тексту через ручки, ми просто міняємо розмір шрифту
    m_fontSize = h;
    if (m_fontSize < 8.0f) m_fontSize = 8.0f;
}

} // namespace NoiseArt
