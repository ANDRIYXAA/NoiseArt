// ============================================================================
// NoiseArt — LayersPanel (Реалізація)
// ============================================================================
// Панель шарів з підтримкою:
// - Дерево шарів (child layers з відступами)
// - Multi-select (Shift+Click)
// - Drag-and-Drop (перетягування для reorder + nesting)
// - Expand/Collapse для груп
// ============================================================================

#include "LayersPanel.h"
#include "core/Layer.h"
#include "Config.h"
#include "effects/TextLayerEffect.h"
#include "effects/VectorLayerEffect.h"
#include "effects/OverlayEffect.h"
#include "effects/ShaderLayerEffect.h"
#include <portable-file-dialogs.h>

namespace NoiseArt {

struct LayerDragPayload {
    int rootIdx;
    int childIdx; // -1 якщо це кореневий шар
};

bool LayersPanel::render(LayerStack& stack)
{
    if (!isOpen) return false;

    bool changed = false;

    ImGui::Begin("Layers", &isOpen);

    // ===== Кнопки керування (зверху) =====
    int selected = stack.getSelectedIndex();

    // Кнопка "+" з випадаючим меню для додавання шарів
    if (ImGui::Button("+", ImVec2(28, 0))) {
        ImGui::OpenPopup("AddLayerMenu");
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add new layer");

    if (ImGui::BeginPopup("AddLayerMenu")) {
        ImGui::SeparatorText("Vector");
        if (ImGui::MenuItem("Rectangle")) {
            auto effect = std::make_unique<VectorLayerEffect>();
            auto layer = std::make_unique<Layer>(std::move(effect), "Rectangle");
            stack.addLayer(std::move(layer));
            stack.setDirty(true);
            changed = true;
        }
        if (ImGui::MenuItem("Circle")) {
            auto effect = std::make_unique<VectorLayerEffect>();
            auto layer = std::make_unique<Layer>(std::move(effect), "Circle");
            stack.addLayer(std::move(layer));
            stack.setDirty(true);
            changed = true;
        }
        if (ImGui::MenuItem("Text")) {
            auto effect = std::make_unique<TextLayerEffect>();
            auto layer = std::make_unique<Layer>(std::move(effect), "Text");
            stack.addLayer(std::move(layer));
            stack.setDirty(true);
            changed = true;
        }

        ImGui::SeparatorText("Media");
        if (ImGui::MenuItem("Image...")) {
            auto result = pfd::open_file("Choose Image", "",
                { "Images", "*.png *.jpg *.jpeg *.bmp *.tga", "All Files", "*" }).result();
            if (!result.empty()) {
                auto effect = std::make_unique<OverlayEffect>();
                if (effect->loadOverlayImage(result[0])) {
                    std::string name = result[0];
                    auto pos = name.find_last_of("/\\");
                    if (pos != std::string::npos) name = name.substr(pos + 1);
                    auto layer = std::make_unique<Layer>(std::move(effect), name);
                    stack.addLayer(std::move(layer));
                    stack.setDirty(true);
                    changed = true;
                }
            }
        }

        ImGui::SeparatorText("Backgrounds");
        if (ImGui::MenuItem("Live Shader")) {
            auto effect = std::make_unique<ShaderLayerEffect>();
            auto layer = std::make_unique<Layer>(std::move(effect), "Shader");
            stack.addLayer(std::move(layer));
            stack.setDirty(true);
            changed = true;
        }

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    // Кнопка "Видалити"
    if (ImGui::Button("Del") && selected >= 0) {
        stack.removeLayer(selected);
        stack.clearSelection();
        changed = true;
    }
    ImGui::SameLine();

    // Кнопка "Дублювати"
    if (ImGui::Button("Dup") && selected >= 0) {
        stack.duplicateLayer(selected);
        changed = true;
    }
    ImGui::SameLine();

    // Стрілки вверх/вниз
    if (ImGui::ArrowButton("up", ImGuiDir_Up) && selected > 0) {
        stack.moveLayer(selected, selected - 1);
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("down", ImGuiDir_Down) &&
        selected >= 0 && selected < stack.getLayerCount() - 1) {
        stack.moveLayer(selected, selected + 1);
        changed = true;
    }

    ImGui::Separator();

    // ===== Список шарів (дерево, від верхнього до нижнього) =====
    auto flatTree = stack.flattenTree();
    
    // Малюємо у зворотньому порядку (верхній шар зверху UI)
    for (int fi = static_cast<int>(flatTree.size()) - 1; fi >= 0; --fi) {
        auto& entry = flatTree[fi];
        Layer* layer = entry.layer;
        if (!layer) continue;

        ImGui::PushID(fi);

        // Відступ для дочірніх шарів
        float indent = static_cast<float>(entry.depth * Config::LAYER_TREE_INDENT);
        if (indent > 0.0f) ImGui::Indent(indent);

        // --- Expand/Collapse стрілка для груп ---
        if (layer->hasChildren()) {
            bool collapsed = layer->isCollapsed();
            ImGuiDir dir = collapsed ? ImGuiDir_Right : ImGuiDir_Down;
            if (ImGui::ArrowButton("##collapse", dir)) {
                layer->setCollapsed(!collapsed);
            }
            ImGui::SameLine();
        } else {
            // Порожнє місце для вирівнювання
            ImGui::Dummy(ImVec2(ImGui::GetFrameHeight(), 0));
            ImGui::SameLine();
        }

        // --- Чекбокс видимості (око) ---
        bool enabled = layer->isEnabled();
        if (ImGui::Checkbox("##vis", &enabled)) {
            layer->setEnabled(enabled);
            changed = true;
            stack.setDirty();
        }
        ImGui::SameLine();

        // --- Визначаємо чи шар виділений ---
        bool isRootLevel = (entry.depth == 0);
        bool isSelected = false;
        if (isRootLevel) {
            isSelected = stack.isSelected(entry.rootIndex) ||
                         (entry.rootIndex == stack.getSelectedIndex());
        }

        // --- Вибір шару (Selectable) ---
        char label[128];
        const char* typeIcon = "";
        if (layer->hasChildren()) typeIcon = "[G] ";
        snprintf(label, sizeof(label), "%s%s  [%s]",
            typeIcon,
            layer->getName().c_str(),
            BlendModeNames[static_cast<int>(layer->getBlendMode())]);

        // Колір виділення: жовтий для multi-select, синій для single
        if (isSelected && stack.getSelectionCount() > 1) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.5f, 0.0f, 0.5f));
        }

        if (ImGui::Selectable(label, isSelected)) {
            if (isRootLevel) {
                bool shiftHeld = ImGui::GetIO().KeyShift;
                if (shiftHeld) {
                    // Shift+Click → multi-select toggle
                    stack.toggleSelection(entry.rootIndex);
                } else {
                    // Normal click
                    if (isSelected && stack.getSelectionCount() <= 1) {
                        // Re-click → deselect
                        stack.clearSelection();
                        stack.setSelectedIndex(-1);
                    } else {
                        // Select this one only
                        stack.clearSelection();
                        stack.setSelectedIndex(entry.rootIndex);
                        stack.addToSelection(entry.rootIndex);
                    }
                }
            }
        }

        if (isSelected && stack.getSelectionCount() > 1) {
            ImGui::PopStyleColor();
        }

        // --- Drag Source (для перетягування) ---
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            LayerDragPayload payload = { entry.rootIndex, entry.childIndex };
            ImGui::SetDragDropPayload("LAYER_DND", &payload, sizeof(LayerDragPayload));
            ImGui::Text("Move: %s", layer->getName().c_str());
            ImGui::EndDragDropSource();
        }

        // --- Drop Target (для reorder та nesting) ---
        if (isRootLevel && ImGui::BeginDragDropTarget()) {
            // Drop для nesting (всередину шару)
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_DND")) {
                const auto* p = (const LayerDragPayload*)payload->Data;
                if (p->childIdx == -1) { // Тільки кореневі шари можна вкладати
                    int srcIdx = p->rootIdx;
                    int dstIdx = entry.rootIndex;
                    if (srcIdx != dstIdx) {
                        stack.nestLayer(srcIdx, dstIdx);
                        changed = true;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }

        // --- Opacity та Blend Mode (для виділеного шару) ---
        if (isSelected && isRootLevel && stack.getSelectionCount() <= 1) {
            float opacity = layer->getOpacity() * 100.0f;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##opacity", &opacity, 0.0f, 100.0f, "Opacity: %.0f%%");
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                layer->setOpacity(opacity / 100.0f);
                changed = true;
                stack.setDirty();
            }

            int blendIdx = static_cast<int>(layer->getBlendMode());
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::Combo("##blend", &blendIdx, BlendModeNames, BlendModeCount)) {
                layer->setBlendMode(static_cast<BlendMode>(blendIdx));
                changed = true;
                stack.setDirty();
            }
        }

        if (indent > 0.0f) ImGui::Unindent(indent);
        ImGui::PopID();
    }

    // --- Drop zone at bottom (для reorder та unnesting) ---
    ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 30));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_DND")) {
            const auto* p = (const LayerDragPayload*)payload->Data;
            if (p->childIdx == -1) {
                // Move root layer to bottom
                if (p->rootIdx > 0) {
                    stack.moveLayer(p->rootIdx, 0);
                    changed = true;
                }
            } else {
                // Unnest child layer to root
                stack.unnestLayer(p->rootIdx, p->childIdx);
                changed = true;
            }
        }
        ImGui::EndDragDropTarget();
    }

    // Якщо стек порожній
    if (stack.getLayerCount() == 0) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "No layers.\nClick '+' to add a layer.");
    }

    ImGui::End();

    return changed;
}

} // namespace NoiseArt
