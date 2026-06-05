// ============================================================================
// NoiseArt — ViewportPanel (Панель перегляду зображення)
// ============================================================================
// Головна панель — показує оброблене зображення з zoom та pan.
// ============================================================================

#pragma once

#include <imgui.h>
#include "renderer/Texture.h"

namespace NoiseArt {

class ViewportPanel {
public:
    /// Малює панель viewport
    /// @param texture Текстура для відображення (результат обробки)
    /// @param imageName Назва файлу для відображення
    void render(const Texture& texture, const std::string& imageName = "");

    // Стан
    bool isOpen = true;

private:
    float m_zoom = 1.0f;        // Масштаб (0.1 — 10.0)
    ImVec2 m_offset = {0, 0};   // Зсув (pan)
    bool m_isPanning = false;
    ImVec2 m_lastMousePos = {0, 0};
};

} // namespace NoiseArt
