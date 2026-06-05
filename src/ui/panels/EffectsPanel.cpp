// ============================================================================
// NoiseArt — EffectsPanel (Реалізація)
// ============================================================================

#include "EffectsPanel.h"
#include "core/Layer.h"
#include <algorithm>
#include <cctype>

namespace NoiseArt {

// Допоміжна функція: чи містить рядок підрядок (без урахування регістру)
static bool containsIgnoreCase(const std::string& str, const std::string& sub)
{
    if (sub.empty()) return true;
    std::string strLower = str;
    std::string subLower = sub;
    std::transform(strLower.begin(), strLower.end(), strLower.begin(), ::tolower);
    std::transform(subLower.begin(), subLower.end(), subLower.begin(), ::tolower);
    return strLower.find(subLower) != std::string::npos;
}

bool EffectsPanel::render(const EffectRegistry& registry, LayerStack& stack)
{
    if (!isOpen) return false;

    bool added = false;

    ImGui::Begin("Effects", &isOpen);

    // ===== Пошук =====
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##search", "Search effects...", m_searchBuffer, sizeof(m_searchBuffer));

    ImGui::Separator();
    ImGui::Spacing();

    std::string searchStr(m_searchBuffer);

    // ===== Список ефектів за категоріями =====
    auto categories = registry.getCategories();

    for (const auto& category : categories) {
        auto effects = registry.getByCategory(category);

        // Фільтруємо за пошуком
        bool hasVisible = false;
        for (const auto* info : effects) {
            if (containsIgnoreCase(info->name, searchStr)) {
                hasVisible = true;
                break;
            }
        }
        if (!hasVisible) continue;

        // Іконка категорії
        const char* icon = "📦";
        if (category == "Generate") icon = "🎲";
        else if (category == "Color") icon = "🎨";
        else if (category == "Blur") icon = "🌫";
        else if (category == "Stylize") icon = "✨";

        // Розкривний заголовок категорії
        if (ImGui::TreeNodeEx(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto* info : effects) {
                if (!containsIgnoreCase(info->name, searchStr)) continue;

                // Кнопка "+" для додавання
                ImGui::PushID(info->name.c_str());
                if (ImGui::Button("+", ImVec2(24, 0))) {
                    // Створюємо новий ефект і загортаємо в шар
                    auto effect = info->create();
                    auto layer = std::make_unique<Layer>(std::move(effect));
                    stack.addLayer(std::move(layer));
                    stack.setDirty();
                    added = true;
                }
                ImGui::SameLine();
                ImGui::Text("%s", info->name.c_str());
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
    }

    ImGui::End();

    return added;
}

} // namespace NoiseArt
