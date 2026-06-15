// ============================================================================
// NoiseArt — ViewportPanel (Реалізація)
// ============================================================================

#include "ViewportPanel.h"
#include "Config.h"
#include "effects/OverlayEffect.h"
#include "effects/VectorLayerEffect.h"
#include "effects/TextLayerEffect.h"
#include "effects/ShaderLayerEffect.h"
#include "effects/NodeGraphEffect.h"
#include "App/App.h"
#include <algorithm>
#include <cmath>
#include <chrono>

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
    // Перемикачі сітки та прив'язки (снепінгу) прямо в тулбарі viewport
    ImGui::SameLine();
    ImGui::Checkbox("Grid", &settings.showGrid);
    ImGui::SameLine();
    ImGui::Checkbox("Snap", &settings.snapToGrid);

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

        auto __perfT0 = std::chrono::high_resolution_clock::now();

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

        // Clipping: дочірній шар обрізається по межах батьківського (якщо той ITransformable)
        bool clipPushed = false;
        if (entry.parent && entry.parent->getEffect() && entry.parent->getEffect()->getTransformable()) {
            auto pt = entry.parent->getEffect()->getTransformable();
            float pAbsX = 0.0f, pAbsY = 0.0f;
            for (Layer* pc = entry.parent; pc; pc = pc->getParent()) {
                auto t = pc->getEffect() ? pc->getEffect()->getTransformable() : nullptr;
                if (t) { pAbsX += t->getX(); pAbsY += t->getY(); }
            }
            ImVec2 cMin(globalOriginX + pAbsX * zoom, globalOriginY + pAbsY * zoom);
            ImVec2 cMax(cMin.x + pt->getWidth() * zoom, cMin.y + pt->getHeight() * zoom);
            drawList->PushClipRect(cMin, cMax, true);
            clipPushed = true;
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
                const Texture* proxy = overlay->getDisplayTexture();
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

            const Texture* vtex = vecEffect->getProcessedTexture();
            if (vtex) {
                // Фігура з накладеними фільтрами — малюємо готову текстуру
                ImU32 tint = IM_COL32(255, 255, 255, static_cast<int>(opacity * 255.0f));
                drawList->AddImage((ImTextureID)(intptr_t)vtex->getID(),
                    ImVec2(vx, vy), ImVec2(vx + vw, vy + vh), ImVec2(0, 0), ImVec2(1, 1), tint);
            } else {
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
        }

        // --- TextLayerEffect (текст) ---
        auto textEffect = dynamic_cast<TextLayerEffect*>(effect);
        if (textEffect && !textEffect->getText().empty()) {
            float tx = globalOriginX + absX * zoom;
            float ty = globalOriginY + absY * zoom;
            float opacity = absOpacity;
            const Texture* ttex = textEffect->getProcessedTexture();
            if (ttex) {
                // Текст, відрендерений NanoVG вибраним шрифтом (+ фільтри/обрізання)
                float tw = textEffect->getWidth() * zoom;
                float th = textEffect->getHeight() * zoom;
                ImU32 tint = IM_COL32(255, 255, 255, static_cast<int>(opacity * 255.0f));
                drawList->AddImage((ImTextureID)(intptr_t)ttex->getID(),
                    ImVec2(tx, ty), ImVec2(tx + tw, ty + th), ImVec2(0, 0), ImVec2(1, 1), tint);
            } else {
                float fontSize = textEffect->getFontSize() * zoom;
                const float* tc = textEffect->getTextColor();
                ImU32 textCol = IM_COL32(
                    (int)(tc[0]*255), (int)(tc[1]*255), (int)(tc[2]*255), (int)(tc[3]*opacity*255));
                drawList->AddText(ImGui::GetFont(), fontSize, ImVec2(tx, ty), textCol, textEffect->getText().c_str());
            }
        }

        // --- ShaderLayerEffect (живий шейдер у власному FBO) ---
        auto shaderEffect = dynamic_cast<ShaderLayerEffect*>(effect);
        if (shaderEffect) {
            auto str = shaderEffect->getTransformable();
            float sx = globalOriginX + absX * zoom;
            float sy = globalOriginY + absY * zoom;
            float sw = str->getWidth() * zoom;
            float sh = str->getHeight() * zoom;

            // Растрові фільтри: власні дочірні + успадковані від груп-предків
            std::vector<std::pair<Effect*, float>> sfilters;
            for (Layer* container = layer; container; container = container->getParent()) {
                for (auto& ch : container->getChildren()) {
                    if (!ch || !ch->isEnabled()) continue;
                    Effect* ce = ch->getEffect();
                    if (ce && !ce->isVector() && !ce->isShader() && !dynamic_cast<OverlayEffect*>(ce))
                        sfilters.push_back({ ce, ch->getOpacity() });
                }
            }
            // Обрізання по формі батька (коло / заокруглений вектор)
            ClipShape sclip;
            if (entry.parent && entry.parent->getEffect()) {
                if (auto pv = dynamic_cast<VectorLayerEffect*>(entry.parent->getEffect())) {
                    if (pv->getShapeType() == VectorLayerEffect::ShapeType::Circle) {
                        sclip.type = ClipShape::Ellipse; sclip.parentW = pv->getWidth(); sclip.parentH = pv->getHeight();
                    } else if (pv->getShapeType() == VectorLayerEffect::ShapeType::RoundedRectangle) {
                        sclip.type = ClipShape::Rounded; sclip.parentW = pv->getWidth(); sclip.parentH = pv->getHeight(); sclip.radius = pv->getRadius();
                    }
                }
            }

            bool flipV = true;
            ShaderLayerEffect::ShaderRenderOpts sopts;
            sopts.cache          = settings.optShaderCache;
            sopts.throttle       = settings.optThrottle;
            sopts.lowResInteract = settings.optLowResDrag;
            sopts.interacting    = (m_dragState != DragState::None);
            sopts.editGen        = layerStack.editGen();
            unsigned int texId = shaderEffect->renderAndGetTexture(
                static_cast<float>(ImGui::GetTime()), absOpacity, sclip, sfilters, flipV, sopts);
            if (texId != 0) {
                ImU32 tint = IM_COL32(255, 255, 255, 255);  // прозорість запечена в альфу/оброблено на CPU
                ImVec2 uv0 = flipV ? ImVec2(0, 1) : ImVec2(0, 0);
                ImVec2 uv1 = flipV ? ImVec2(1, 0) : ImVec2(1, 1);
                drawList->AddImage((ImTextureID)(intptr_t)texId,
                    ImVec2(sx, sy), ImVec2(sx + sw, sy + sh), uv0, uv1, tint);
            }
        }

        // --- NodeGraphEffect (нодовий граф) — окрема гілка (line 304 кастить до КОНКРЕТНОГО ShaderLayerEffect*) ---
        auto nodeGraphEffect = dynamic_cast<NodeGraphEffect*>(effect);
        if (nodeGraphEffect) {
            auto ngt = nodeGraphEffect->getTransformable();
            float sx = globalOriginX + absX * zoom;
            float sy = globalOriginY + absY * zoom;
            float sw = ngt->getWidth() * zoom;
            float sh = ngt->getHeight() * zoom;

            // Растрові фільтри: власні дочірні + успадковані від груп-предків (як для шейдера)
            std::vector<std::pair<Effect*, float>> sfilters;
            for (Layer* container = layer; container; container = container->getParent()) {
                for (auto& ch : container->getChildren()) {
                    if (!ch || !ch->isEnabled()) continue;
                    Effect* ce = ch->getEffect();
                    if (ce && !ce->isVector() && !ce->isShader() && !dynamic_cast<OverlayEffect*>(ce))
                        sfilters.push_back({ ce, ch->getOpacity() });
                }
            }
            ClipShape sclip;
            if (entry.parent && entry.parent->getEffect()) {
                if (auto pv = dynamic_cast<VectorLayerEffect*>(entry.parent->getEffect())) {
                    if (pv->getShapeType() == VectorLayerEffect::ShapeType::Circle) {
                        sclip.type = ClipShape::Ellipse; sclip.parentW = pv->getWidth(); sclip.parentH = pv->getHeight();
                    } else if (pv->getShapeType() == VectorLayerEffect::ShapeType::RoundedRectangle) {
                        sclip.type = ClipShape::Rounded; sclip.parentW = pv->getWidth(); sclip.parentH = pv->getHeight(); sclip.radius = pv->getRadius();
                    }
                }
            }

            bool flipV = true;
            ShaderLayerEffect::ShaderRenderOpts sopts;
            sopts.cache          = settings.optShaderCache;
            sopts.throttle       = settings.optThrottle;
            sopts.lowResInteract = settings.optLowResDrag;
            sopts.interacting    = (m_dragState != DragState::None);
            sopts.editGen        = layerStack.editGen();
            unsigned int texId = nodeGraphEffect->renderAndGetTexture(
                static_cast<float>(ImGui::GetTime()), absOpacity, sclip, sfilters, flipV, sopts);
            if (texId != 0) {
                ImU32 tint = IM_COL32(255, 255, 255, static_cast<int>(absOpacity * 255.0f));
                ImVec2 uv0 = flipV ? ImVec2(0, 1) : ImVec2(0, 0);
                ImVec2 uv1 = flipV ? ImVec2(1, 0) : ImVec2(1, 1);
                drawList->AddImage((ImTextureID)(intptr_t)texId,
                    ImVec2(sx, sy), ImVec2(sx + sw, sy + sh), uv0, uv1, tint);
            }
        }

        if (clipPushed) drawList->PopClipRect();

        auto __perfT1 = std::chrono::high_resolution_clock::now();
        layerStack.recordPerf(layer, std::chrono::duration<float, std::milli>(__perfT1 - __perfT0).count());
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

    // ===== ТРАНСФОРМАЦІЙНІ РУЧКИ (FREE TRANSFORM, вибраний вузол будь-якого рівня) =====
    Layer* selLayer = layerStack.getSelectedLayer();
    if (selLayer && selLayer->isEnabled() && selLayer->getEffect()) {
        auto transformable = selLayer->getEffect()->getTransformable();
        if (transformable) {
            // Абсолютна позиція (підйом по дереву батьків)
            float absX = 0.0f, absY = 0.0f;
            for (Layer* c = selLayer; c; c = c->getParent()) {
                auto t = c->getEffect() ? c->getEffect()->getTransformable() : nullptr;
                if (t) { absX += t->getX(); absY += t->getY(); }
            }

            float layerW = transformable->getWidth() * zoom;
            float layerH = transformable->getHeight() * zoom;
            float layerX = globalOriginX + absX * zoom;
            float layerY = globalOriginY + absY * zoom;

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

            ImVec2 mousePos = ImGui::GetMousePos();
            auto checkHover = [&](ImVec2 pos) {
                float dx = mousePos.x - pos.x;
                float dy = mousePos.y - pos.y;
                return (dx * dx + dy * dy) <= r * r * 4.0f;
            };

            // Початок перетягування
            if (!m_isPanning && isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (checkHover(tl)) m_dragState = DragState::ScaleTopLeft;
                else if (checkHover(tr)) m_dragState = DragState::ScaleTopRight;
                else if (checkHover(bl)) m_dragState = DragState::ScaleBottomLeft;
                else if (checkHover(br)) m_dragState = DragState::ScaleBottomRight;
                else if (mousePos.x >= layerX && mousePos.x <= layerX + layerW &&
                         mousePos.y >= layerY && mousePos.y <= layerY + layerH)
                    m_dragState = DragState::Move;
                else m_dragState = DragState::None;

                if (m_dragState != DragState::None) {
                    m_dragStartMouse = mousePos;
                    m_dragLayer = selLayer;
                    m_dragStartX = transformable->getX();
                    m_dragStartY = transformable->getY();
                    m_dragStartWidth = transformable->getWidth();
                    m_dragRootIndex = layerStack.rootIndexOf(selLayer);
                    Layer* rootAnc = layerStack.getLayer(m_dragRootIndex);
                    m_dragOldRoot = rootAnc ? rootAnc->clone() : nullptr;
                    auto ov = dynamic_cast<OverlayEffect*>(selLayer->getEffect());
                    if (ov) ov->setHiddenFromStack(true);
                }
            }

            // Завершення перетягування → запис в історію
            if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                if (m_dragState != DragState::None && m_dragLayer) {
                    auto ov = dynamic_cast<OverlayEffect*>(m_dragLayer->getEffect());
                    if (ov) ov->setHiddenFromStack(false);
                    Layer* rootAnc = layerStack.getLayer(m_dragRootIndex);
                    if (m_dragOldRoot && rootAnc) {
                        history.push(std::make_unique<ChangeLayerCommand>(
                            m_dragRootIndex, std::move(m_dragOldRoot), rootAnc->clone()));
                    }
                    m_dragOldRoot = nullptr;
                    m_dragLayer = nullptr;
                    layerStack.setDirty(true);
                }
                m_dragState = DragState::None;
            }

            // Власне рух / масштаб
            if (m_dragState != DragState::None && m_dragLayer &&
                ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                auto tr2 = m_dragLayer->getEffect()->getTransformable();
                if (tr2) {
                    float deltaX = mousePos.x - m_dragStartMouse.x;
                    float deltaY = mousePos.y - m_dragStartMouse.y;
                    auto applySnapping = [&](float& val) {
                        if (settings.snapToGrid && settings.gridSize > 0.0f)
                            val = std::round(val / settings.gridSize) * settings.gridSize;
                    };

                    if (m_dragState == DragState::Move) {
                        float newX = m_dragStartX + deltaX / zoom;
                        float newY = m_dragStartY + deltaY / zoom;
                        applySnapping(newX);
                        applySnapping(newY);
                        tr2->setPosition(newX, newY);
                    } else {
                        // Вільне (нерівномірне) масштабування: тягнемо від протилежного кута до миші,
                        // ширина і висота незалежні → можна і масштабувати, і розтягувати.
                        ImVec2 oppCorner;
                        if (m_dragState == DragState::ScaleTopLeft) oppCorner = br;
                        else if (m_dragState == DragState::ScaleTopRight) oppCorner = bl;
                        else if (m_dragState == DragState::ScaleBottomLeft) oppCorner = tr;
                        else oppCorner = tl;

                        float newWidth  = std::abs(mousePos.x - oppCorner.x) / zoom;
                        float newHeight = std::abs(mousePos.y - oppCorner.y) / zoom;
                        if (newWidth  < 1.0f) newWidth  = 1.0f;
                        if (newHeight < 1.0f) newHeight = 1.0f;
                        applySnapping(newWidth);
                        applySnapping(newHeight);

                        // Спершу задаємо розмір, тоді читаємо ФАКТИЧНІ розміри: деякі ефекти
                        // не приймають довільні W/H (фото тримає пропорцію через єдиний scale).
                        tr2->setSize(newWidth, newHeight);
                        float actualW = tr2->getWidth();
                        float actualH = tr2->getHeight();

                        // Протилежний кут лишається на місці (позиція рахується з фактичних розмірів)
                        float oppAbsX = (oppCorner.x - globalOriginX) / zoom;
                        float oppAbsY = (oppCorner.y - globalOriginY) / zoom;
                        float parentX = absX - transformable->getX();  // абс. зсув батька (стабільний під час drag)
                        float parentY = absY - transformable->getY();

                        bool anchorRight = (m_dragState == DragState::ScaleTopLeft || m_dragState == DragState::ScaleBottomLeft);
                        bool anchorBottom = (m_dragState == DragState::ScaleTopLeft || m_dragState == DragState::ScaleTopRight);
                        float newAbsX = anchorRight  ? oppAbsX - actualW : oppAbsX;
                        float newAbsY = anchorBottom ? oppAbsY - actualH : oppAbsY;

                        tr2->setPosition(newAbsX - parentX, newAbsY - parentY);
                    }
                }
            }
        }
    }

    // ===== КЛІК-ДЛЯ-ВИБОРУ / ДЕСЕЛЕКТ / MULTI-SELECT =====
    if (!m_isPanning && isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && m_dragState == DragState::None) {
        ImVec2 mousePos = ImGui::GetMousePos();
        bool shiftHeld = ImGui::GetIO().KeyShift;
        bool hitLayer = false;

        auto clickTree = layerStack.flattenTree();
        // Зверху вниз (верхні шари мають пріоритет на клік)
        for (int fi = static_cast<int>(clickTree.size()) - 1; fi >= 0; --fi) {
            auto l = clickTree[fi].layer;
            if (!l || !l->isEnabled()) continue;
            auto tr = l->getEffect() ? l->getEffect()->getTransformable() : nullptr;
            if (!tr) continue;

            float absX = 0.0f, absY = 0.0f;
            for (Layer* c = l; c; c = c->getParent()) {
                auto t = c->getEffect() ? c->getEffect()->getTransformable() : nullptr;
                if (t) { absX += t->getX(); absY += t->getY(); }
            }
            float lx = globalOriginX + absX * zoom;
            float ly = globalOriginY + absY * zoom;
            float lw = tr->getWidth() * zoom;
            float lh = tr->getHeight() * zoom;
            if (mousePos.x >= lx && mousePos.x <= lx + lw &&
                mousePos.y >= ly && mousePos.y <= ly + lh) {
                int rootIdx = clickTree[fi].rootIndex;
                if (shiftHeld && clickTree[fi].depth == 0) {
                    layerStack.toggleSelection(rootIdx);
                    layerStack.setSelectedLayer(l);
                } else {
                    layerStack.clearSelection();
                    layerStack.setSelectedIndex(rootIdx);
                    layerStack.addToSelection(rootIdx);
                    layerStack.setSelectedLayer(l);  // вибираємо саме цей вузол (можливо дочірній)
                }
                hitLayer = true;
                break;
            }
        }
        if (!hitLayer) {
            layerStack.clearSelection();
        }
    }

    ImGui::End();
}

} // namespace NoiseArt
