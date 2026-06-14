#include "VectorLayerEffect.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstring>

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

// ============================================================================
// Растеризація фігури у RGBA (CPU) — щоб можна було накласти растрові фільтри
// ============================================================================
void VectorLayerEffect::rasterize(Image& out) const {
    int w = static_cast<int>(m_width);
    int h = static_cast<int>(m_height);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    // Пропорційно обмежуємо роздільність, щоб не алокувати величезні буфери
    int maxDim = (w > h) ? w : h;
    if (maxDim > 1024) {
        float s = 1024.0f / static_cast<float>(maxDim);
        w = (std::max)(1, static_cast<int>(w * s));
        h = (std::max)(1, static_cast<int>(h * s));
    }
    out.create(w, h, 4);
    out.clear(0, 0, 0, 0);  // прозорий фон

    auto toByte = [](float v) { return static_cast<uint8_t>(std::clamp(v, 0.0f, 1.0f) * 255.0f); };
    uint8_t fr = toByte(m_color[0]), fg = toByte(m_color[1]), fb = toByte(m_color[2]), fa = toByte(m_color[3]);
    uint8_t sr = toByte(m_strokeColor[0]), sg = toByte(m_strokeColor[1]), sb = toByte(m_strokeColor[2]), sa = toByte(m_strokeColor[3]);

    float cx = w * 0.5f, cy = h * 0.5f;
    float rx = w * 0.5f, ry = h * 0.5f;
    float pxPerUnit = static_cast<float>(w) / (m_width > 1.0f ? m_width : 1.0f);
    float strokePx = m_strokeWidth * pxPerUnit;
    float radPx = m_radius * pxPerUnit;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            float px = x + 0.5f, py = y + 0.5f;
            bool inside = false, onStroke = false;

            switch (m_shapeType) {
                case ShapeType::Rectangle:
                    inside = true;
                    if (m_stroke) onStroke = (px < strokePx || py < strokePx || px > w - strokePx || py > h - strokePx);
                    break;
                case ShapeType::Circle: {
                    float nx = (px - cx) / rx, ny = (py - cy) / ry;
                    float d = nx * nx + ny * ny;
                    inside = (d <= 1.0f);
                    if (m_stroke && rx > 0.0f) {
                        float inner = (rx - strokePx) / rx;
                        onStroke = inside && d >= inner * inner;
                    }
                    break;
                }
                case ShapeType::RoundedRectangle: {
                    float r = (std::min)(radPx, (std::min)(w * 0.5f, h * 0.5f));
                    float qx = std::fabs(px - cx) - (w * 0.5f - r);
                    float qy = std::fabs(py - cy) - (h * 0.5f - r);
                    float ax = (std::max)(qx, 0.0f), ay = (std::max)(qy, 0.0f);
                    float sdf = std::sqrt(ax * ax + ay * ay) + (std::min)((std::max)(qx, qy), 0.0f) - r;
                    inside = (sdf <= 0.0f);
                    if (m_stroke) onStroke = inside && sdf >= -strokePx;
                    break;
                }
            }

            uint8_t* p = out.getPixel(x, y);
            if (!p) continue;
            if (onStroke && m_stroke) {
                p[0] = sr; p[1] = sg; p[2] = sb; p[3] = sa;
            } else if (inside && m_fill) {
                p[0] = fr; p[1] = fg; p[2] = fb; p[3] = fa;
            }
            // інакше — прозорий
        }
    }
}

// ============================================================================
// Застосування ланцюжка дочірніх фільтрів до растеризованої фігури
// ============================================================================
void VectorLayerEffect::applyFilters(const std::vector<std::pair<Effect*, float>>& filters, const ClipShape& clip) {
    if (filters.empty() && clip.type == ClipShape::None) { m_processed.reset(); return; }
    Image img;
    rasterize(img);
    if (img.isEmpty()) { m_processed.reset(); return; }
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
    applyClipMask(img, clip, getX(), getY(), getWidth(), getHeight());
    if (!m_processed) m_processed = std::make_shared<Texture>();
    m_processed->update(img);
}

} // namespace NoiseArt
