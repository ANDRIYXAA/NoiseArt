// ============================================================================
// NoiseArt — OverlayEffect (Накладання іншого зображення)
// ============================================================================
// Дозволяє додати інше зображення (текстуру, водяний знак) поверх поточного.
// ============================================================================

#pragma once

#include "Effect.h"
#include "renderer/Texture.h"
#include "effects/ClipShape.h"
#include <imgui.h>
#include <portable-file-dialogs.h>
#include <stb_image.h>
#include <algorithm>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <cstring>

namespace NoiseArt {

class OverlayEffect : public Effect, public ITransformable {
public:
    std::string getName() const override { return "Image Overlay"; }
    std::string getCategory() const override { return "Compositing"; }

    OverlayEffect() = default;
    ~OverlayEffect() = default;

    bool loadOverlayImage(const std::string& path) {
        int w, h, ch;
        uint8_t* raw = stbi_load(path.c_str(), &w, &h, &ch, 4);
        if (!raw) return false;
        
        auto img = std::make_shared<SharedImage>();
        img->w = w;
        img->h = h;
        img->ch = ch;
        img->pixels = std::shared_ptr<uint8_t>(raw, [](uint8_t* p) { stbi_image_free(p); });
        
        img->texture = std::make_shared<Texture>();
        Image tmp;
        tmp.create(w, h, 4);
        std::memcpy(tmp.getData(), raw, w * h * 4);
        img->texture->update(tmp);
        
        m_image = img;
        m_overlayPath = path;
        return true;
    }

    void apply(const Image& input, Image& output) override
    {
        output = input.clone();
        if (!m_image || !m_image->pixels || m_image->w == 0 || m_image->h == 0 || m_isHiddenFromStack) return;

        uint8_t* outData = output.getData();
        int w = input.getWidth();
        int h = input.getHeight();

        int overlayW = m_image->w;
        int overlayH = m_image->h;
        uint8_t* overlayData = m_image->pixels.get();

        int startY = 0, endY = h;
        int startX = 0, endX = w;

        if (!m_scaleToFit) {
            startX = (std::max)(0, m_offsetX);
            endX = (std::min)(w, m_offsetX + static_cast<int>(overlayW * m_scale));
            startY = (std::max)(0, m_offsetY);
            endY = (std::min)(h, m_offsetY + static_cast<int>(overlayH * m_scale));
        }

        for (int y = startY; y < endY; ++y) {
            for (int x = startX; x < endX; ++x) {
                int ox, oy;
                if (m_scaleToFit) {
                    ox = (x * overlayW) / w;
                    oy = (y * overlayH) / h;
                } else {
                    ox = static_cast<int>((x - m_offsetX) / m_scale);
                    oy = static_cast<int>((y - m_offsetY) / m_scale);
                }

                if (ox >= 0 && ox < overlayW && oy >= 0 && oy < overlayH) {
                    int outIdx = (y * w + x) * 4;
                    int overIdx = (oy * overlayW + ox) * 4;
                    
                    float r1 = outData[outIdx + 0] / 255.0f;
                    float g1 = outData[outIdx + 1] / 255.0f;
                    float b1 = outData[outIdx + 2] / 255.0f;
                    float a1 = outData[outIdx + 3] / 255.0f;

                    float r2 = overlayData[overIdx + 0] / 255.0f;
                    float g2 = overlayData[overIdx + 1] / 255.0f;
                    float b2 = overlayData[overIdx + 2] / 255.0f;
                    float a2 = (overlayData[overIdx + 3] / 255.0f) * m_opacity;

                    float aout = a2 + a1 * (1.0f - a2);
                    
                    if (aout > 0.0f) {
                        float rout = (r2 * a2 + r1 * a1 * (1.0f - a2)) / aout;
                        float gout = (g2 * a2 + g1 * a1 * (1.0f - a2)) / aout;
                        float bout = (b2 * a2 + b1 * a1 * (1.0f - a2)) / aout;

                        outData[outIdx + 0] = static_cast<uint8_t>(std::clamp(rout * 255.0f, 0.0f, 255.0f));
                        outData[outIdx + 1] = static_cast<uint8_t>(std::clamp(gout * 255.0f, 0.0f, 255.0f));
                        outData[outIdx + 2] = static_cast<uint8_t>(std::clamp(bout * 255.0f, 0.0f, 255.0f));
                    }
                    outData[outIdx + 3] = static_cast<uint8_t>(std::clamp(aout * 255.0f, 0.0f, 255.0f));
                }
            }
        }
    }

    bool renderUI() override
    {
        bool changed = false;

        if (ImGui::Button("Load Image...")) {
            auto result = pfd::open_file("Choose Image Overlay", "",
                { "Images (.png .jpg .jpeg .bmp)", "*.png *.jpg *.jpeg *.bmp", "All Files", "*" }).result();
            
            if (!result.empty()) {
                if (loadOverlayImage(result[0])) {
                    changed = true;
                }
            }
        }

        if (m_image) {
            size_t slashPos = m_overlayPath.find_last_of("/\\");
            std::string filename = (slashPos != std::string::npos) ? m_overlayPath.substr(slashPos + 1) : m_overlayPath;
            ImGui::TextWrapped("File: %s", filename.c_str());
            ImGui::Text("Size: %dx%d", m_image->w, m_image->h);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No image loaded.");
        }

        ImGui::SliderFloat("Image Opacity", &m_opacity, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::IsItemDeactivatedAfterEdit();

        if (ImGui::Checkbox("Scale to Fit", &m_scaleToFit)) changed = true;

        if (!m_scaleToFit) {
            ImGui::SliderFloat("Scale", &m_scale, 0.01f, 10.0f, "%.2fx");
            changed |= ImGui::IsItemDeactivatedAfterEdit();
            ImGui::SliderInt("Offset X", &m_offsetX, -4000, 4000);
            changed |= ImGui::IsItemDeactivatedAfterEdit();
            ImGui::SliderInt("Offset Y", &m_offsetY, -4000, 4000);
            changed |= ImGui::IsItemDeactivatedAfterEdit();
        }

        return changed;
    }

    std::unique_ptr<Effect> clone() const override
    {
        auto copy = std::make_unique<OverlayEffect>();
        copy->m_opacity = m_opacity;
        copy->m_scaleToFit = m_scaleToFit;
        copy->m_scale = m_scale;
        copy->m_offsetX = m_offsetX;
        copy->m_offsetY = m_offsetY;
        copy->m_overlayPath = m_overlayPath;
        copy->m_image = m_image;
        return copy;
    }

    bool isScaleToFit() const { return m_scaleToFit; }
    void setScaleToFit(bool fit) { m_scaleToFit = fit; }

    float getScale() const { return m_scale; }
    void setScale(float s) { m_scale = s; }

    int getOffsetX() const { return m_offsetX; }
    void setOffsetX(int x) { m_offsetX = x; }

    int getOffsetY() const { return m_offsetY; }
    void setOffsetY(int y) { m_offsetY = y; }

    ITransformable* getTransformable() override { return this; }

    float getX() const override { return static_cast<float>(m_offsetX); }
    float getY() const override { return static_cast<float>(m_offsetY); }
    float getWidth() const override { return m_image ? m_image->w * m_scale : 0.0f; }
    float getHeight() const override { return m_image ? m_image->h * m_scale : 0.0f; }
    void setPosition(float x, float y) override { m_offsetX = static_cast<int>(x); m_offsetY = static_cast<int>(y); }
    void setSize(float w, float h) override { if (m_image && m_image->w > 0) m_scale = w / static_cast<float>(m_image->w); }

    int getOverlayWidth() const { return m_image ? m_image->w : 0; }
    int getOverlayHeight() const { return m_image ? m_image->h : 0; }
    bool hasImage() const { return m_image != nullptr; }

    bool isHiddenFromStack() const { return m_isHiddenFromStack; }
    void setHiddenFromStack(bool h) { m_isHiddenFromStack = h; }

    const Texture* getProxyTexture() const { return m_image ? m_image->texture.get() : nullptr; }

    // Текстура для показу: оброблена фільтрами (якщо є) або оригінал
    const Texture* getDisplayTexture() const {
        if (m_processed && m_processed->isValid()) return m_processed.get();
        return m_image ? m_image->texture.get() : nullptr;
    }

    // Застосовує ланцюжок дочірніх растрових фільтрів до пікселів фото (модель "контент своєї групи").
    // Кожен фільтр блендиться з попереднім результатом за СВОЄЮ opacity (pair.second).
    void applyFilters(const std::vector<std::pair<Effect*, float>>& filters, const ClipShape& clip = {}) {
        if (!m_image || !m_image->pixels || (filters.empty() && clip.type == ClipShape::None)) { m_processed.reset(); return; }
        Image img;
        img.create(m_image->w, m_image->h, 4);
        std::memcpy(img.getData(), m_image->pixels.get(),
                    static_cast<size_t>(m_image->w) * m_image->h * 4);
        for (const auto& pr : filters) {
            Effect* f = pr.first;
            float op = pr.second;
            if (!f || op <= 0.0f) continue;            // вимкнений / 0% — пропускаємо
            Image out;
            f->apply(img, out);
            if (op >= 1.0f || out.getWidth() != img.getWidth() || out.getHeight() != img.getHeight()) {
                img = std::move(out);                  // повна сила (або фільтр змінив розмір)
            } else {
                // блендимо результат фільтра з оригіналом за його opacity
                uint8_t* a = img.getData();
                const uint8_t* b = out.getData();
                int n = img.getWidth() * img.getHeight() * 4;
                for (int i = 0; i < n; ++i)
                    a[i] = static_cast<uint8_t>(a[i] + op * (static_cast<float>(b[i]) - a[i]));
            }
        }
        applyClipMask(img, clip, getX(), getY(), getWidth(), getHeight());
        if (!m_processed) m_processed = std::make_shared<Texture>();
        m_processed->update(img);
    }

    float getOpacity() const { return m_opacity; }

private:
    struct SharedImage {
        std::shared_ptr<uint8_t> pixels;
        std::shared_ptr<Texture> texture;
        int w = 0;
        int h = 0;
        int ch = 0;
    };

    float m_opacity = 1.0f;
    bool m_scaleToFit = false;
    float m_scale = 1.0f;
    int m_offsetX = 0;
    int m_offsetY = 0;
    
    std::shared_ptr<SharedImage> m_image;
    std::shared_ptr<Texture> m_processed;   // кеш: фото, оброблене дочірніми фільтрами
    std::string m_overlayPath = "";

    bool m_isHiddenFromStack = false;
};

} // namespace NoiseArt
