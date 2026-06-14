#include "TextLayerEffect.h"
#include <imgui.h>
#include <cstring>
#include <cmath>
#include <vector>

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

void TextLayerEffect::rasterizeAndProcess(NVGcontext* vg, const std::vector<std::pair<Effect*, float>>& filters, const ClipShape& clip) {
    if (m_text.empty() || !vg) { m_processed.reset(); return; }

    const char* fontName = (m_fontIndex >= 0 && m_fontIndex < FontCount) ? FontNames[m_fontIndex] : "Inter";

    // Вимірюємо розмір тексту вибраним шрифтом
    nvgFontSize(vg, m_fontSize);
    nvgFontFace(vg, fontName);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    float bounds[4] = { 0, 0, 0, 0 };
    nvgTextBounds(vg, 0.0f, 0.0f, m_text.c_str(), nullptr, bounds);
    const float pad = 4.0f;
    float tw = (bounds[2] - bounds[0]) + pad * 2.0f;
    float th = (bounds[3] - bounds[1]) + pad * 2.0f;
    if (tw < 1.0f) tw = 1.0f;
    if (th < 1.0f) th = 1.0f;
    m_measuredW = tw;
    m_measuredH = th;

    int fw = static_cast<int>(std::ceil(tw));
    int fh = static_cast<int>(std::ceil(th));
    if (fw > 2048) fw = 2048;
    if (fh > 2048) fh = 2048;
    if (fw < 1) fw = 1;
    if (fh < 1) fh = 1;

    if (m_fbo.getWidth() != static_cast<uint32_t>(fw) || m_fbo.getHeight() != static_cast<uint32_t>(fh)) {
        m_fbo.create(static_cast<uint32_t>(fw), static_cast<uint32_t>(fh));
    }

    GLint prevVp[4];
    glGetIntegerv(GL_VIEWPORT, prevVp);
    m_fbo.bind();
    glViewport(0, 0, fw, fh);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    nvgBeginFrame(vg, static_cast<float>(fw), static_cast<float>(fh), 1.0f);
    nvgFontSize(vg, m_fontSize);
    nvgFontFace(vg, fontName);
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
    nvgFillColor(vg, nvgRGBAf(m_color[0], m_color[1], m_color[2], m_color[3]));
    nvgText(vg, pad - bounds[0], pad - bounds[1], m_text.c_str(), nullptr);
    nvgEndFrame(vg);

    // Зчитуємо пікселі (FBO ще прив'язаний)
    m_fbo.bind();
    Image img;
    img.create(fw, fh, 4);
    glReadPixels(0, 0, fw, fh, GL_RGBA, GL_UNSIGNED_BYTE, img.getData());
    m_fbo.unbind();
    glViewport(prevVp[0], prevVp[1], prevVp[2], prevVp[3]);

    // glReadPixels дає bottom-up → перевертаємо у top-down
    int rowBytes = fw * 4;
    uint8_t* d = img.getData();
    std::vector<uint8_t> tmp(rowBytes);
    for (int y = 0; y < fh / 2; ++y) {
        uint8_t* r0 = d + static_cast<size_t>(y) * rowBytes;
        uint8_t* r1 = d + static_cast<size_t>(fh - 1 - y) * rowBytes;
        std::memcpy(tmp.data(), r0, rowBytes);
        std::memcpy(r0, r1, rowBytes);
        std::memcpy(r1, tmp.data(), rowBytes);
    }

    // Фільтри (як для фото/вектора/шейдера)
    for (const auto& pr : filters) {
        Effect* f = pr.first;
        float op = pr.second;
        if (!f || op <= 0.0f) continue;
        Image res;
        f->apply(img, res);
        if (op >= 1.0f || res.getWidth() != img.getWidth() || res.getHeight() != img.getHeight()) {
            img = std::move(res);
        } else {
            uint8_t* a = img.getData();
            const uint8_t* b = res.getData();
            int n = img.getWidth() * img.getHeight() * 4;
            for (int i = 0; i < n; ++i)
                a[i] = static_cast<uint8_t>(a[i] + op * (static_cast<float>(b[i]) - a[i]));
        }
    }

    // Обрізання по формі батька
    applyClipMask(img, clip, getX(), getY(), getWidth(), getHeight());

    if (!m_processed) m_processed = std::make_shared<Texture>();
    m_processed->update(img);
}

float TextLayerEffect::getWidth() const {
    return (m_measuredW > 0.0f) ? m_measuredW : m_text.length() * m_fontSize * 0.5f;
}

float TextLayerEffect::getHeight() const {
    return (m_measuredH > 0.0f) ? m_measuredH : m_fontSize;
}

void TextLayerEffect::setSize(float w, float h) {
    // При зміні розміру тексту через ручки, ми просто міняємо розмір шрифту
    m_fontSize = h;
    if (m_fontSize < 8.0f) m_fontSize = 8.0f;
}

} // namespace NoiseArt
