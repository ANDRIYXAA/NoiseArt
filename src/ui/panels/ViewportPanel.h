#pragma once

#include <imgui.h>
#include <memory>
#include <string>
#include <unordered_map>
#include "renderer/Texture.h"
#include "renderer/Framebuffer.h"
#include "core/LayerStack.h"
#include "core/Camera2D.h"
#include "core/History.h"

class History;

namespace NoiseArt {

struct AppSettings;

class ViewportPanel {
public:
    void render(const Texture& texture, const Framebuffer& fbo, const std::string& imageName, LayerStack& layerStack, Camera2D& camera, History& history, AppSettings& settings);

    bool isOpen = true;

private:
    bool m_isPanning = false;
    ImVec2 m_lastMousePos;

    enum class DragState {
        None, Move, ScaleTopLeft, ScaleTopRight, ScaleBottomLeft, ScaleBottomRight
    };
    DragState m_dragState = DragState::None;
    ImVec2 m_dragStartMouse;
    
    // Перетягування поточного вибраного шару (будь-який вузол дерева)
    Layer* m_dragLayer = nullptr;
    int m_dragRootIndex = -1;
    float m_dragStartX = 0.0f;
    float m_dragStartY = 0.0f;
    float m_dragStartWidth = 1.0f;
    std::unique_ptr<Layer> m_dragOldRoot;
};

} // namespace NoiseArt
