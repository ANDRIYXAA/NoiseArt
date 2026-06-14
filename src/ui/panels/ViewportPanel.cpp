// ============================================================================
// NoiseArt — ViewportPanel (Реалізація)
// ============================================================================

#include "ViewportPanel.h"
#include "Config.h"
#include "effects/OverlayEffect.h"
#include "effects/VectorLayerEffect.h"
#include "effects/TextLayerEffect.h"
#include "App/App.h"
#include <algorithm>
#include <cmath>

namespace NoiseArt {

void ViewportPanel::render(const Texture& texture, const Framebuffer& fbo, const std::string& imageName, LayerStack& layerStack, Camera2D& camera, History& history, AppSettings& settings)
{
    if (!isOpen) return;

    ImGui::Begin("Viewport", &isOpen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    // Інформаційний рядок зверху
    if (texture.isValid()) {
        ImGui::Text("%s  |  %dx%d  |  Zoom: %.0f%%",
            imageName.empty() ? "No file" : imageName.c_str(),
            texture.getWidth(), texture.getHeight(),
            camera.getZoom() * 100.0f);
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
            camera.setZoom((std::min)(scaleX, scaleY));
            camera.setPosition({0, 0});
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("100%")) {
        camera.setZoom(1.0f);
        camera.setPosition({0, 0});
    }

    ImGui::Separator();

    // Область для зображення
    ImVec2 winPos = ImGui::GetCursorScreenPos();
    ImVec2 winSize = ImGui::GetContentRegionAvail();
    
    ImVec2 cursorPos = winPos;
    ImVec2 avail = winSize;

    // Оновлюємо розмір камери
    camera.setViewportSize(avail.x, avail.y);

    // Невидима кнопка для перехоплення введення (миша, колесо)
    ImGui::InvisibleButton("viewport_area", avail);
    bool isHovered = ImGui::IsItemHovered();

    // Zoom колесом миші
    if (isHovered) {
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f) {
            float oldZoom = camera.getZoom();
            float newZoom = oldZoom * ((wheel > 0) ? Config::ZOOM_STEP : (1.0f / Config::ZOOM_STEP));
            newZoom = std::clamp(newZoom, Config::ZOOM_MIN, Config::ZOOM_MAX);

            // Зум до курсора
            ImVec2 mousePos = ImGui::GetMousePos();
            float relX = mousePos.x - cursorPos.x - avail.x * 0.5f - camera.getPosition().x;
            float relY = mousePos.y - cursorPos.y - avail.y * 0.5f - camera.getPosition().y;
            float zoomRatio = newZoom / oldZoom;
            
            camera.setPosition({
                camera.getPosition().x - relX * (zoomRatio - 1.0f),
                camera.getPosition().y - relY * (zoomRatio - 1.0f)
            });
            camera.setZoom(newZoom);
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
        camera.setPosition({
            camera.getPosition().x + mousePos.x - m_lastMousePos.x,
            camera.getPosition().y + mousePos.y - m_lastMousePos.y
        });
        m_lastMousePos = mousePos;
    } else {
        m_isPanning = false;
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Global Coordinates: origin is at the center of the viewport
    float globalOriginX = cursorPos.x + avail.x * 0.5f + camera.getPosition().x;
    float globalOriginY = cursorPos.y + avail.y * 0.5f + camera.getPosition().y;

    // ===== ІНФІНІТНА СІТКА (ГЛОБАЛЬНА) =====
    if (settings.showGrid && settings.gridSize > 0.0f) {
        float zoom = camera.getZoom();
        
        if (settings.autoGridScale) {
            float targetSnap = 100.0f / zoom;
            float magnitude = std::pow(10.0f, std::floor(std::log10(targetSnap)));
            float normalized = targetSnap / magnitude;
            
            // Використовуємо 1, 2, 5, 10 щоб сітка маштабувалась кратно (subdivisions)
            if (normalized < 2.0f) settings.gridSize = 1.0f * magnitude;
            else if (normalized < 5.0f) settings.gridSize = 2.0f * magnitude;
            else if (normalized < 10.0f) settings.gridSize = 5.0f * magnitude;
            else settings.gridSize = 10.0f * magnitude;
        }
        
        float snap = settings.gridSize;
        float scaledSnap = snap * zoom;

        // Запобігаємо малюванню мільйонів ліній якщо сітка занадто дрібна
        if (scaledSnap < Config::GRID_MIN_PIXEL_SIZE) scaledSnap = Config::GRID_MIN_PIXEL_SIZE; 

        // Координати вікна
        float minX = cursorPos.x;
        float minY = cursorPos.y;
        float maxX = cursorPos.x + avail.x;
        float maxY = cursorPos.y + avail.y;

        ImU32 gridColor = Config::GRID_LINE_COLOR;
        
        // Вертикальні лінії (вирівняні по globalOrigin)
        int startX_idx = std::ceil((minX - globalOriginX) / scaledSnap);
        for (float x = globalOriginX + startX_idx * scaledSnap; x <= maxX; x += scaledSnap) {
            drawList->AddLine(ImVec2(x, minY), ImVec2(x, maxY), gridColor);
        }

        // Горизонтальні лінії (вирівняні по globalOrigin)
        int startY_idx = std::ceil((minY - globalOriginY) / scaledSnap);
        for (float y = globalOriginY + startY_idx * scaledSnap; y <= maxY; y += scaledSnap) {
            drawList->AddLine(ImVec2(minX, y), ImVec2(maxX, y), gridColor);
        }
        
        // Виділення осей (X=0, Y=0)
        ImU32 axisColor = Config::AXIS_X_COLOR;
        if (globalOriginX >= minX && globalOriginX <= maxX)
            drawList->AddLine(ImVec2(globalOriginX, minY), ImVec2(globalOriginX, maxY), axisColor, 2.0f);
        if (globalOriginY >= minY && globalOriginY <= maxY)
            drawList->AddLine(ImVec2(minX, globalOriginY), ImVec2(maxX, globalOriginY), axisColor, 2.0f);
    }
    // ===== МАЛЮВАННЯ ШАРІВ (кожен шар на своїй позиції, правильний Z-порядок) =====
    float zoom = camera.getZoom();
    auto flatTree = layerStack.flattenTree();
    for (const auto& entry : flatTree) {
        auto layer = entry.layer;
        if (!layer || !layer->isEnabled()) continue;
        auto effect = layer->getEffect();
        if (!effect) continue;

        // Обчислюємо абсолютну позицію та opacity, піднімаючись по дереву
        float absX = 0.0f;
        float absY = 0.0f;
        float absOpacity = 1.0f;
        Layer* curr = layer;
        while (curr) {
            absOpacity *= curr->getOpacity();
            if (curr->getEffect() && curr->getEffect()->getTransformable()) {
                absX += curr->getEffect()->getTransformable()->getX();
                absY += curr->getEffect()->getTransformable()->getY();
            }
            curr = curr->getParent();
        }

        // --- OverlayEffect (зображення) ---
        auto overlay = dynamic_cast<OverlayEffect*>(effect);
        if (overlay && overlay->hasImage()) {
            auto tr = overlay->getTransformable();
            if (tr) {
                float lx = globalOriginX + absX * zoom;
                float ly = globalOriginY + absY * zoom;
                float lw = tr->getWidth() * zoom;
                float lh = tr->getHeight() * zoom;
                const Texture* proxy = overlay->getProxyTexture();
                if (proxy && proxy->isValid()) {
                    ImU32 tint = IM_COL32(255, 255, 255, static_cast<int>(overlay->getOpacity() * absOpacity * 255.0f));
                    drawList->AddImage(
                        (ImTextureID)(intptr_t)proxy->getID(),
                        ImVec2(lx, ly), ImVec2(lx + lw, ly + lh),
                        ImVec2(0, 0), ImVec2(1, 1), tint
                    );
                }
            }
        }

        // --- VectorLayerEffect (векторні фігури) ---
        auto vecEffect = dynamic_cast<VectorLayerEffect*>(effect);
        if (vecEffect) {
            float vx = globalOriginX + absX * zoom;
            float vy = globalOriginY + absY * zoom;
            float vw = vecEffect->getWidth() * zoom;
            float vh = vecEffect->getHeight() * zoom;
            float opacity = absOpacity;

            const float* fc = vecEffect->getFillColor();
            ImU32 fillCol = IM_COL32(
                (int)(fc[0]*255), (int)(fc[1]*255), (int)(fc[2]*255), (int)(fc[3]*opacity*255));
            const float* sc = vecEffect->getStrokeColor();
            ImU32 strokeCol = IM_COL32(
                (int)(sc[0]*255), (int)(sc[1]*255), (int)(sc[2]*255), (int)(sc[3]*opacity*255));

            switch (vecEffect->getShapeType()) {
            case VectorLayerEffect::ShapeType::Rectangle:
                if (vecEffect->isFill())
                    drawList->AddRectFilled(ImVec2(vx, vy), ImVec2(vx+vw, vy+vh), fillCol);
                if (vecEffect->hasStroke())
                    drawList->AddRect(ImVec2(vx, vy), ImVec2(vx+vw, vy+vh), strokeCol, 0.0f, 0, vecEffect->getStrokeWidth() * zoom);
                break;
            case VectorLayerEffect::ShapeType::Circle: {
                ImVec2 center(vx + vw*0.5f, vy + vh*0.5f);
                float rx = vw * 0.5f, ry = vh * 0.5f;
                if (vecEffect->isFill())
                    drawList->AddEllipseFilled(center, ImVec2(rx, ry), fillCol);
                if (vecEffect->hasStroke())
                    drawList->AddEllipse(center, ImVec2(rx, ry), strokeCol, 0.0f, 0, vecEffect->getStrokeWidth() * zoom);
                break;
            }
            case VectorLayerEffect::ShapeType::RoundedRectangle: {
                float rounding = vecEffect->getRadius() * zoom;
                if (vecEffect->isFill())
                    drawList->AddRectFilled(ImVec2(vx, vy), ImVec2(vx+vw, vy+vh), fillCol, rounding);
                if (vecEffect->hasStroke())
                    drawList->AddRect(ImVec2(vx, vy), ImVec2(vx+vw, vy+vh), strokeCol, rounding, 0, vecEffect->getStrokeWidth() * zoom);
                break;
            }
            }
        }

        // --- TextLayerEffect (текст) ---
        auto textEffect = dynamic_cast<TextLayerEffect*>(effect);
        if (textEffect && !textEffect->getText().empty()) {
            float tx = globalOriginX + absX * zoom;
            float ty = globalOriginY + absY * zoom;
            float fontSize = textEffect->getFontSize() * zoom;
            const float* tc = textEffect->getTextColor();
            float opacity = absOpacity;
            ImU32 textCol = IM_COL32(
                (int)(tc[0]*255), (int)(tc[1]*255), (int)(tc[2]*255), (int)(tc[3]*opacity*255));
            ImFont* font = ImGui::GetFont();
            drawList->AddText(font, fontSize, ImVec2(tx, ty), textCol, textEffect->getText().c_str());
        }
    }

    // ===== РАМКИ ВИДІЛЕНИХ ШАРІВ (multi-select) =====
    for (int si : layerStack.getSelectedIndices()) {
        if (si == layerStack.getSelectedIndex()) continue; // основний малюємо окремо
        auto l = layerStack.getLayer(si);
        if (!l || !l->isEnabled()) continue;
        auto tr = l->getEffect() ? l->getEffect()->getTransformable() : nullptr;
        if (tr) {
            float absX = 0.0f;
            float absY = 0.0f;
            Layer* curr = l;
            while (curr) {
                if (curr->getEffect() && curr->getEffect()->getTransformable()) {
                    absX += curr->getEffect()->getTransformable()->getX();
                    absY += curr->getEffect()->getTransformable()->getY();
                }
                curr = curr->getParent();
            }

            float lx = globalOriginX + absX * zoom;
            float ly = globalOriginY + absY * zoom;
            float lw = tr->getWidth() * zoom;
            float lh = tr->getHeight() * zoom;
            drawList->AddRect(ImVec2(lx, ly), ImVec2(lx + lw, ly + lh),
                Config::MULTI_SELECTION_COLOR, 0.0f, 0, Config::SELECTION_BORDER_WIDTH);
        }
    }

    // ===== ТРАНСФОРМАЦІЙНІ РУЧКИ (FREE TRANSFORM, основний вибраний шар) =====
    int selIdx = layerStack.getSelectedIndex();
    if (selIdx >= 0 && selIdx < layerStack.getLayerCount()) {
        auto layer = layerStack.getLayer(selIdx);
        auto transformable = layer->getEffect() ? layer->getEffect()->getTransformable() : nullptr;
        if (transformable) {
            float layerW = transformable->getWidth() * zoom;
            float layerH = transformable->getHeight() * zoom;
            float layerX = globalOriginX + transformable->getX() * zoom;
            float layerY = globalOriginY + transformable->getY() * zoom;

            auto overlay = dynamic_cast<OverlayEffect*>(layer->getEffect());

            ImU32 handleCol = Config::HANDLE_COLOR;
            drawList->AddRect(ImVec2(layerX, layerY), ImVec2(layerX + layerW, layerY + layerH), handleCol, 0.0f, 0, Config::HANDLE_BORDER_WIDTH);

            ImVec2 tl(layerX, layerY);
            ImVec2 tr(layerX + layerW, layerY);
            ImVec2 bl(layerX, layerY + layerH);
            ImVec2 br(layerX + layerW, layerY + layerH);

            float r = Config::HANDLE_RADIUS;

            drawList->AddCircleFilled(tl, r, handleCol);
            drawList->AddCircleFilled(tr, r, handleCol);
            drawList->AddCircleFilled(bl, r, handleCol);
            drawList->AddCircleFilled(br, r, handleCol);
            
            // --- Обробка взаємодії мишею ---
            ImVec2 mousePos = ImGui::GetMousePos();

            auto checkHover = [&](ImVec2 pos) {
                float dx = mousePos.x - pos.x;
                float dy = mousePos.y - pos.y;
                return (dx * dx + dy * dy) <= r * r * 4.0f;
            };

            if (!m_isPanning && isHovered) {
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    if (checkHover(tl)) m_dragState = DragState::ScaleTopLeft;
                    else if (checkHover(tr)) m_dragState = DragState::ScaleTopRight;
                    else if (checkHover(bl)) m_dragState = DragState::ScaleBottomLeft;
                    else if (checkHover(br)) m_dragState = DragState::ScaleBottomRight;
                    else if (mousePos.x >= layerX && mousePos.x <= layerX + layerW &&
                             mousePos.y >= layerY && mousePos.y <= layerY + layerH) {
                        m_dragState = DragState::Move;
                    } else {
                        m_dragState = DragState::None;
                    }

                    if (m_dragState != DragState::None) {
                        m_dragStartMouse = mousePos;
                        m_dragData.clear();
                        
                        if (m_dragState == DragState::Move) {
                            for (int si : layerStack.getSelectedIndices()) {
                                auto l = layerStack.getLayer(si);
                                if (l && l->getEffect() && l->getEffect()->getTransformable()) {
                                    auto tr = l->getEffect()->getTransformable();
                                    DragStateData d;
                                    d.startX = tr->getX();
                                    d.startY = tr->getY();
                                    d.oldState = l->clone();
                                    m_dragData[si] = std::move(d);
                                    
                                    auto ov = dynamic_cast<OverlayEffect*>(l->getEffect());
                                    if (ov) ov->setHiddenFromStack(true);
                                }
                            }
                        } else {
                            DragStateData d;
                            d.startX = transformable->getX();
                            d.startY = transformable->getY();
                            d.startWidth = transformable->getWidth();
                            d.oldState = layer->clone();
                            m_dragData[selIdx] = std::move(d);
                            
                            if (overlay) overlay->setHiddenFromStack(true);
                        }
                        layerStack.setDirty(true);
                    }
                }
            }

            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (m_dragState != DragState::None) {
                    for (auto& pair : m_dragData) {
                        int si = pair.first;
                        auto l = layerStack.getLayer(si);
                        if (l) {
                            auto ov = dynamic_cast<OverlayEffect*>(l->getEffect());
                            if (ov) ov->setHiddenFromStack(false);
                            if (pair.second.oldState) {
                                history.push(std::make_unique<ChangeLayerCommand>(si, std::move(pair.second.oldState), l->clone()));
                            }
                        }
                    }
                    m_dragData.clear();
                    layerStack.setDirty(true);
                }
                m_dragState = DragState::None;
            }

            if (m_dragState != DragState::None && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                float deltaX = mousePos.x - m_dragStartMouse.x;
                float deltaY = mousePos.y - m_dragStartMouse.y;

                auto applySnapping = [&](float& val) {
                    if (settings.snapToGrid && settings.gridSize > 0.0f) {
                        val = std::round(val / settings.gridSize) * settings.gridSize;
                    }
                };

                if (m_dragState == DragState::Move) {
                    for (int si : layerStack.getSelectedIndices()) {
                        auto it = m_dragData.find(si);
                        if (it == m_dragData.end()) continue;
                        
                        auto l = layerStack.getLayer(si);
                        if (!l) continue;
                        auto tr = l->getEffect() ? l->getEffect()->getTransformable() : nullptr;
                        if (!tr) continue;

                        float newX = it->second.startX + deltaX / camera.getZoom();
                        float newY = it->second.startY + deltaY / camera.getZoom();
                        applySnapping(newX);
                        applySnapping(newY);
                        
                        tr->setPosition(newX, newY);
                        if (l->getEffect()->isVector()) layerStack.setDirty(true);
                    }
                } else {
                    auto it = m_dragData.find(selIdx);
                    if (it != m_dragData.end()) {
                        ImVec2 oppCorner;
                        if (m_dragState == DragState::ScaleTopLeft) oppCorner = br;
                        else if (m_dragState == DragState::ScaleTopRight) oppCorner = bl;
                        else if (m_dragState == DragState::ScaleBottomLeft) oppCorner = tr;
                        else if (m_dragState == DragState::ScaleBottomRight) oppCorner = tl;
                        
                        float newDistX = std::abs(mousePos.x - oppCorner.x);
                        float newWidth = newDistX / camera.getZoom();
                        if (newWidth < 1.0f) newWidth = 1.0f;
                        
                        applySnapping(newWidth);
                        
                        // Зберігаємо пропорції
                        float aspectRatio = transformable->getHeight() / transformable->getWidth();
                        float newHeight = newWidth * aspectRatio;

                        // Компенсація Offset
                        float newX = it->second.startX;
                        float newY = it->second.startY;
                        float startScale = it->second.startWidth;
                        
                        if (m_dragState == DragState::ScaleTopLeft) {
                            newX -= (newWidth - startScale);
                            newY -= (newHeight - (startScale * aspectRatio));
                        } else if (m_dragState == DragState::ScaleBottomLeft) {
                            newX -= (newWidth - startScale);
                        } else if (m_dragState == DragState::ScaleTopRight) {
                            newY -= (newHeight - (startScale * aspectRatio));
                        }
                        
                        applySnapping(newX);
                        applySnapping(newY);

                        transformable->setPosition(newX, newY);
                        transformable->setSize(newWidth, newHeight);

                        if (layer->getEffect()->isVector()) layerStack.setDirty(true);
                    }
                }
            } // кінець if (m_isDragging)
        } // кінець if (transformable)
    } // кінець if (selIdx)

    // ===== КЛІК-ДЛЯ-ВИБОРУ / ДЕСЕЛЕКТ / MULTI-SELECT =====
    if (!m_isPanning && isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_dragState == DragState::None) {
        ImVec2 mousePos = ImGui::GetMousePos();
        bool shiftHeld = ImGui::GetIO().KeyShift;
        bool hitLayer = false;
        
        auto flatTree = layerStack.flattenTree();
        for (int fi = static_cast<int>(flatTree.size()) - 1; fi >= 0; --fi) {
            auto l = flatTree[fi].layer;
            int li = flatTree[fi].rootIndex; // We should probably use rootIndex if it's top level, but for selection it's easier. Actually flattenTree doesn't easily give flat index for selection. We need a way to select correctly.
            // Wait, selection in LayerStack operates on the flat index? NO! Selection operates on the ROOT indices currently!
            // Wait, does selection operate on root indices or flat indices?
            // "std::set<int> m_selectedIndices; // Multi-select (root indices)"
            // So we can only select root layers currently?
            // "int rootIndex; // Індекс у root масиві"
            if (!l || !l->isEnabled()) continue;
            auto tr = l->getEffect() ? l->getEffect()->getTransformable() : nullptr;
            if (tr) {
                float absX = 0.0f;
                float absY = 0.0f;
                Layer* curr = l;
                while (curr) {
                    if (curr->getEffect() && curr->getEffect()->getTransformable()) {
                        absX += curr->getEffect()->getTransformable()->getX();
                        absY += curr->getEffect()->getTransformable()->getY();
                    }
                    curr = curr->getParent();
                }

                float lx = globalOriginX + absX * zoom;
                float ly = globalOriginY + absY * zoom;
                float lw = tr->getWidth() * zoom;
                float lh = tr->getHeight() * zoom;
                if (mousePos.x >= lx && mousePos.x <= lx + lw &&
                    mousePos.y >= ly && mousePos.y <= ly + lh) {
                    
                    // We only select root index for now
                    int selectIdx = flatTree[fi].rootIndex;

                    if (shiftHeld) {
                        layerStack.toggleSelection(selectIdx);
                    } else {
                        layerStack.clearSelection();
                        layerStack.setSelectedIndex(selectIdx);
                        layerStack.addToSelection(selectIdx);
                    }
                    hitLayer = true;
                    break;
                }
            }
        }
        if (!hitLayer) {
            layerStack.clearSelection();
            layerStack.setSelectedIndex(-1);
        }
    }

    ImGui::End();
}

} // namespace NoiseArt
