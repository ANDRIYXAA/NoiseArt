// ============================================================================
// NoiseArt — Реєстр ефектів (Реалізація)
// ============================================================================

#include "EffectRegistry.h"
#include <algorithm>  // std::find

namespace NoiseArt {

// ============================================================================
// getCategories() — Отримати список унікальних категорій
// ============================================================================
std::vector<std::string> EffectRegistry::getCategories() const
{
    std::vector<std::string> categories;

    for (const auto& effect : m_effects) {
        // Перевіряємо чи категорія вже є в списку
        // std::find шукає елемент у діапазоні [begin, end)
        auto it = std::find(categories.begin(), categories.end(), effect.category);
        if (it == categories.end()) {
            // Не знайдено — додаємо
            categories.push_back(effect.category);
        }
    }

    return categories;
}

// ============================================================================
// getByCategory() — Отримати ефекти певної категорії
// ============================================================================
std::vector<const EffectInfo*> EffectRegistry::getByCategory(const std::string& category) const
{
    std::vector<const EffectInfo*> result;

    for (const auto& effect : m_effects) {
        if (effect.category == category) {
            // &effect — адреса елемента у m_effects
            result.push_back(&effect);
        }
    }

    return result;
}

// ============================================================================
// createEffect() — Створити ефект за назвою
// ============================================================================
std::unique_ptr<Effect> EffectRegistry::createEffect(const std::string& name) const
{
    for (const auto& effect : m_effects) {
        if (effect.name == name) {
            // Викликаємо фабричну функцію, яка створить новий об'єкт
            return effect.create();
        }
    }

    // Ефект не знайдено
    return nullptr;
}

} // namespace NoiseArt
