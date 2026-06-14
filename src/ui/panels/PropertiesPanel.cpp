// ============================================================================
// NoiseArt — PropertiesPanel (Реалізація)
// ============================================================================

#include "PropertiesPanel.h"
#include "core/Layer.h"
#include "App/App.h"

namespace NoiseArt {

bool PropertiesPanel::render(LayerStack& stack, AppSettings& settings)
{
    if (!isOpen) return false;

    bool changed = false;

    ImGui::Begin("Properties", &isOpen);

    int selected = stack.getSelectedIndex();
    Layer* layer = stack.getLayer(selected);

    if (!layer) {
        // Коли нічого не вибрано — показуємо повідомлення
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No layer selected");
        ImGui::End();
        return changed;
    }

    // ===== Назва шару =====
    char nameBuf[128];
    strncpy(nameBuf, layer->getName().c_str(), sizeof(nameBuf) - 1);
    nameBuf[sizeof(nameBuf) - 1] = '\0';
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf))) {
        layer->setName(nameBuf);
    }

    ImGui::Spacing();

    // ===== Інфо про ефект =====
    Effect* effect = layer->getEffect();
    if (effect) {
        ImGui::TextColored(ImVec4(0.6f, 0.4f, 1.0f, 1.0f),
            "Effect: %s", effect->getName().c_str());
        ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.6f, 1.0f),
            "Category: %s", effect->getCategory().c_str());
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ===== Opacity =====
    float opacity = layer->getOpacity() * 100.0f;
    ImGui::SliderFloat("Opacity", &opacity, 0.0f, 100.0f, "%.0f%%");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        layer->setOpacity(opacity / 100.0f);
        changed = true;
        stack.setDirty();
    }

    // ===== Blend Mode =====
    int blendIdx = static_cast<int>(layer->getBlendMode());
    if (ImGui::Combo("Blend Mode", &blendIdx, BlendModeNames, BlendModeCount)) {
        layer->setBlendMode(static_cast<BlendMode>(blendIdx));
        changed = true;
        stack.setDirty();
    }

    // ===== Enabled =====
    bool enabled = layer->isEnabled();
    if (ImGui::Checkbox("Enabled", &enabled)) {
        layer->setEnabled(enabled);
        changed = true;
        stack.setDirty();
    }

    ImGui::Separator();
    ImGui::Spacing();

    // ===== Параметри ефекту =====
    if (effect) {
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Effect Settings:");
        ImGui::Spacing();

        // Кожен ефект малює свої власні слайдери/чекбокси
        if (effect->renderUI()) {
            changed = true;
            stack.setDirty();
        }
    }

    ImGui::End();

    return changed;
}

} // namespace NoiseArt
