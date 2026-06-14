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
    
    struct DragStateData {
        float startX = 0.0f;
        float startY = 0.0f;
        float startWidth = 1.0f;
        std::unique_ptr<Layer> oldState;
    };
    std::unordered_map<int, DragStateData> m_dragData;
};

} // namespace NoiseArt
