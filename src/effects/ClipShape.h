// ============================================================================
// NoiseArt — ClipShape: обрізання дочірнього контенту по формі батька
// ============================================================================
// ImGui вміє обрізати лише по прямокутнику (PushClipRect). Щоб обрізати по
// ФОРМІ (коло / заокруглений прямокутник), застосовуємо альфа-маску до
// растеризованого контенту дитини: пікселі поза формою батька стають прозорими.
// ============================================================================

#pragma once

#include "core/Image.h"
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace NoiseArt {

struct ClipShape {
    enum Type { None = 0, Ellipse, Rounded };
    int   type    = None;
    float parentW = 0.0f;   // розмір батьківської форми (локальні координати батька)
    float parentH = 0.0f;
    float radius  = 0.0f;   // для Rounded
};

// Обнуляє альфу пікселів зображення дитини, що лежать ПОЗА формою батька.
// childX/childY — зсув дитини в локальних координатах батька; childW/childH — її розмір.
inline void applyClipMask(Image& img, const ClipShape& clip,
                          float childX, float childY, float childW, float childH) {
    if (clip.type == ClipShape::None || img.isEmpty()) return;
    if (childW <= 0.0f || childH <= 0.0f || clip.parentW <= 0.0f || clip.parentH <= 0.0f) return;

    const int iw = img.getWidth();
    const int ih = img.getHeight();
    const float cx = clip.parentW * 0.5f, cy = clip.parentH * 0.5f;
    const float rx = clip.parentW * 0.5f, ry = clip.parentH * 0.5f;

    for (int y = 0; y < ih; ++y) {
        for (int x = 0; x < iw; ++x) {
            // піксель зображення → локальна координата батька
            float px = childX + (x + 0.5f) / iw * childW;
            float py = childY + (y + 0.5f) / ih * childH;

            bool inside;
            if (clip.type == ClipShape::Ellipse) {
                float nx = (px - cx) / rx, ny = (py - cy) / ry;
                inside = (nx * nx + ny * ny) <= 1.0f;
            } else { // Rounded
                float r  = (std::min)(clip.radius, (std::min)(clip.parentW * 0.5f, clip.parentH * 0.5f));
                float qx = std::fabs(px - cx) - (clip.parentW * 0.5f - r);
                float qy = std::fabs(py - cy) - (clip.parentH * 0.5f - r);
                float ax = (std::max)(qx, 0.0f), ay = (std::max)(qy, 0.0f);
                float sdf = std::sqrt(ax * ax + ay * ay) + (std::min)((std::max)(qx, qy), 0.0f) - r;
                inside = sdf <= 0.0f;
            }

            if (!inside) {
                uint8_t* p = img.getPixel(x, y);
                if (p) p[3] = 0;
            }
        }
    }
}

} // namespace NoiseArt
