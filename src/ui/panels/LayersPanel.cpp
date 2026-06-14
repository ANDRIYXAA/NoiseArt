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
            effect->setShapeType(VectorLayerEffect::ShapeType::Circle);
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

    // Кнопка "Видалити" — видаляє саме вибраний вузол (root або дитину)
    if (ImGui::Button("Del")) {
        if (Layer* sel = stack.getSelectedLayer()) {
            stack.removeLayerPtr(sel);
            changed = true;
        }
    }
    ImGui::SameLine();

    // Кнопка "Дублювати"
    if (ImGui::Button("Dup") && selected >= 0) {
        stack.duplicateLayer(selected);
        changed = true;
    }
    ImGui::SameLine();

    // Стрілки вверх/вниз — переставляють вибраний вузол серед сусідів (root або child)
    Layer* selLayer = stack.getSelectedLayer();
    if (ImGui::ArrowButton("up", ImGuiDir_Up) && selLayer) {
        stack.moveLayerInParent(selLayer, -1);
        changed = true;
    }
    ImGui::SameLine();
    if (ImGui::ArrowButton("down", ImGuiDir_Down) && selLayer) {
        stack.moveLayerInParent(selLayer, +1);
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
        bool isPrimary = (layer == stack.getSelectedLayer());
        bool inMultiSet = isRootLevel && stack.isSelected(entry.rootIndex);
        bool isSelected = isPrimary || inMultiSet;

        // --- Вибір шару (Selectable) ---
        char label[128];
        const char* typeIcon = "";
        if (layer->hasChildren()) typeIcon = "[G] ";
        snprintf(label, sizeof(label), "%s%s  [%s]",
            typeIcon,
            layer->getName().c_str(),
            BlendModeNames[static_cast<int>(layer->getBlendMode())]);

        // Колір виділення: жовтий для multi-select
        bool multiHighlight = inMultiSet && stack.getSelectionCount() > 1;
        if (multiHighlight) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.6f, 0.5f, 0.0f, 0.5f));
        }

        if (ImGui::Selectable(label, isSelected)) {
            bool shiftHeld = ImGui::GetIO().KeyShift;
            if (shiftHeld && isRootLevel) {
                // Shift+Click на кореневому → multi-select toggle
                stack.toggleSelection(entry.rootIndex);
                stack.setSelectedLayer(layer);
            } else if (isPrimary && stack.getSelectionCount() <= 1) {
                // Re-click → deselect
                stack.clearSelection();
            } else {
                // Вибір цього вузла (root або child)
                stack.clearSelection();
                stack.setSelectedIndex(entry.rootIndex);
                stack.addToSelection(entry.rootIndex);
                stack.setSelectedLayer(layer);
            }
        }

        if (multiHighlight) {
            ImGui::PopStyleColor();
        }

        // --- Drag Source (для перетягування) ---
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            Layer* dragLayer = layer;
            ImGui::SetDragDropPayload("LAYER_DND", &dragLayer, sizeof(Layer*));
            ImGui::Text("Move: %s", layer->getName().c_str());
            ImGui::EndDragDropSource();
        }

        // --- Drop Target: вкласти перетягнутий шар у цей (будь-який вузол) ---
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_DND")) {
                Layer* moving = *(Layer* const*)payload->Data;
                if (moving && moving != layer) {
                    stack.nestUnder(moving, layer);
                    changed = true;
                }
            }
            ImGui::EndDragDropTarget();
        }

        // --- Opacity та Blend Mode (для виділеного шару) ---
        if (isPrimary && stack.getSelectionCount() <= 1) {
            float opacity = layer->getOpacity() * 100.0f;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##opacity", &opacity, 0.0f, 100.0f, "Opacity: %.0f%%")) {
                layer->setOpacity(opacity / 100.0f);  // одразу, без відскоку
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                stack.setDirty();  // перерахувати фільтри-діти
                changed = true;
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
            Layer* moving = *(Layer* const*)payload->Data;
            stack.moveToRoot(moving);  // винести на кореневий рівень
            changed = true;
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
