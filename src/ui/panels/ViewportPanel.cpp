// ============================================================================
// NoiseArt — ViewportPanel (Реалізація)
// ============================================================================

#include "ViewportPanel.h"
#include <algorithm>

namespace NoiseArt {

void ViewportPanel::render(const Texture& texture, const std::string& imageName)
{
    if (!isOpen) return;

    ImGui::Begin("Viewport", &isOpen);

    // Інформаційний рядок зверху
    if (texture.isValid()) {
        ImGui::Text("%s  |  %dx%d  |  Zoom: %.0f%%",
            imageName.empty() ? "No file" : imageName.c_str(),
            texture.getWidth(), texture.getHeight(),
            m_zoom * 100.0f);
    } else {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
            "Drag & drop an image or use File > Open");
    }

    // Кнопки зуму
    ImGui::SameLine();
    if (ImGui::SmallButton("Fit")) {
        // Підігнати під розмір панелі
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (texture.isValid() && texture.getWidth() > 0 && texture.getHeight() > 0) {
            float scaleX = avail.x / texture.getWidth();
            float scaleY = avail.y / texture.getHeight();
            m_zoom = std::min(scaleX, scaleY);
            m_offset = {0, 0};
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("100%")) {
        m_zoom = 1.0f;
        m_offset = {0, 0};
    }

    ImGui::Separator();

    // Область для зображення
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

    // Невидима кнопка для перехоплення введення (миша, колесо)
    ImGui::InvisibleButton("viewport_area", avail);
    bool isHovered = ImGui::IsItemHovered();

    // Zoom колесом миші
    if (isHovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float oldZoom = m_zoom;
            m_zoom *= (wheel > 0) ? 1.15f : 0.87f; // ±15% за крок
            m_zoom = std::clamp(m_zoom, 0.05f, 20.0f);

            // Зум до курсора (щоб центр масштабування = позиція миші)
            ImVec2 mousePos = ImGui::GetMousePos();
            float relX = mousePos.x - cursorPos.x - avail.x * 0.5f - m_offset.x;
            float relY = mousePos.y - cursorPos.y - avail.y * 0.5f - m_offset.y;
            float zoomRatio = m_zoom / oldZoom;
            m_offset.x -= relX * (zoomRatio - 1.0f);
            m_offset.y -= relY * (zoomRatio - 1.0f);
        }
    }

    // Pan середньою кнопкою або Space+ЛКМ
    if (isHovered && (ImGui::IsMouseDown(ImGuiMouseButton_Middle) ||
        (ImGui::IsKeyDown(ImGuiKey_Space) && ImGui::IsMouseDown(ImGuiMouseButton_Left)))) {
        if (!m_isPanning) {
            m_isPanning = true;
            m_lastMousePos = ImGui::GetMousePos();
        }
        ImVec2 mousePos = ImGui::GetMousePos();
        m_offset.x += mousePos.x - m_lastMousePos.x;
        m_offset.y += mousePos.y - m_lastMousePos.y;
        m_lastMousePos = mousePos;
    } else {
        m_isPanning = false;
    }

    // Малюємо зображення
    if (texture.isValid()) {
        float imgW = texture.getWidth() * m_zoom;
        float imgH = texture.getHeight() * m_zoom;

        // Центруємо зображення в панелі
        float drawX = cursorPos.x + avail.x * 0.5f - imgW * 0.5f + m_offset.x;
        float drawY = cursorPos.y + avail.y * 0.5f - imgH * 0.5f + m_offset.y;

        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Шахова дошка (для прозорості) — як в Photoshop
        drawList->AddRectFilled(
            ImVec2(drawX, drawY),
            ImVec2(drawX + imgW, drawY + imgH),
            IM_COL32(40, 40, 40, 255)
        );

        // Зображення
        drawList->AddImage(
            (ImTextureID)(intptr_t)texture.getID(),
            ImVec2(drawX, drawY),
            ImVec2(drawX + imgW, drawY + imgH)
        );

        // Рамка
        drawList->AddRect(
            ImVec2(drawX, drawY),
            ImVec2(drawX + imgW, drawY + imgH),
            IM_COL32(80, 80, 80, 200)
        );
    }

    ImGui::End();
}

} // namespace NoiseArt
