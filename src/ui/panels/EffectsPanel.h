// ============================================================================
// NoiseArt — EffectsPanel (Каталог ефектів)
// ============================================================================
// Показує всі доступні ефекти, згруповані за категоріями.
// Натискання "+" додає новий шар з вибраним ефектом.
// ============================================================================

#pragma once

#include <imgui.h>
#include "effects/EffectRegistry.h"
#include "core/LayerStack.h"

namespace NoiseArt {

class EffectsPanel {
public:
    /// @param registry Реєстр ефектів (каталог)
    /// @param stack Стек шарів (куди додавати нові)
    /// @return true, якщо додано новий шар
    bool render(const EffectRegistry& registry, LayerStack& stack);

    bool isOpen = true;

private:
    char m_searchBuffer[128] = ""; // Буфер для пошуку
};

} // namespace NoiseArt
