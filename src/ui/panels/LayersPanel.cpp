// ============================================================================
// NoiseArt — LayersPanel (Реалізація)
// ============================================================================

#include "LayersPanel.h"
#include "core/Layer.h"

namespace NoiseArt {

bool LayersPanel::render(LayerStack& stack)
{
    if (!isOpen) return false;

    bool changed = false;

    ImGui::Begin("Layers", &isOpen);

    // ===== Кнопки керування (зверху) =====
    int selected = stack.getSelectedIndex();

    // Кнопка "Видалити"
    if (ImGui::Button("Delete") && selected >= 0) {
        stack.removeLayer(selected);
        changed = true;
    }
    ImGui::SameLine();

    // Кнопка "Дублювати"
    if (ImGui::Button("Duplicate") && selected >= 0) {
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

    // ===== Список шарів =====
    // Малюємо від ВЕРХНЬОГО шару до нижнього (останній = зверху)
    for (int i = stack.getLayerCount() - 1; i >= 0; --i) {
        Layer* layer = stack.getLayer(i);
        if (!layer) continue;

        ImGui::PushID(i); // Унікальний ID для ImGui (щоб кнопки не конфліктували)

        // --- Чекбокс видимості (око) ---
        bool enabled = layer->isEnabled();
        if (ImGui::Checkbox("##vis", &enabled)) {
            layer->setEnabled(enabled);
            changed = true;
            stack.setDirty();
        }
        ImGui::SameLine();

        // --- Вибір шару (Selectable) ---
        bool isSelected = (i == stack.getSelectedIndex());
        
        // Формуємо рядок: "Назва [Blend Mode]"
        char label[128];
        snprintf(label, sizeof(label), "%s  [%s]",
            layer->getName().c_str(),
            BlendModeNames[static_cast<int>(layer->getBlendMode())]);

        if (ImGui::Selectable(label, isSelected)) {
            stack.setSelectedIndex(i);
        }

        // --- Opacity (міні-слайдер при наведенні) ---
        if (isSelected) {
            float opacity = layer->getOpacity() * 100.0f;
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::SliderFloat("##opacity", &opacity, 0.0f, 100.0f, "Opacity: %.0f%%")) {
                layer->setOpacity(opacity / 100.0f);
                changed = true;
                stack.setDirty();
            }

            // Blend mode
            int blendIdx = static_cast<int>(layer->getBlendMode());
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            if (ImGui::Combo("##blend", &blendIdx, BlendModeNames, BlendModeCount)) {
                layer->setBlendMode(static_cast<BlendMode>(blendIdx));
                changed = true;
                stack.setDirty();
            }
        }

        ImGui::PopID();
    }

    // Якщо стек порожній
    if (stack.getLayerCount() == 0) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
            "No layers.\nAdd effects from the Effects panel.");
    }

    ImGui::End();

    return changed;
}

} // namespace NoiseArt
