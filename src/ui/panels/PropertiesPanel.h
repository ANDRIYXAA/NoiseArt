// ============================================================================
// NoiseArt — PropertiesPanel (Панель властивостей)
// ============================================================================
// Показує властивості ВИБРАНОГО шару:
// - Назва, opacity, blend mode
// - Параметри ефекту (слайдери, чекбокси — від effect->renderUI())
// ============================================================================

#pragma once

#include <imgui.h>
#include "core/LayerStack.h"

namespace NoiseArt {

class PropertiesPanel {
public:
    /// @param stack Стек шарів
    /// @return true, якщо параметри змінилися
    bool render(LayerStack& stack);

    bool isOpen = true;
};

} // namespace NoiseArt
