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

class AppSettings; // Forward declaration

class PropertiesPanel {
public:
    /// Малює панель властивостей
    /// @param stack Стек шарів для отримання вибраного шару
    /// @param settings Глобальні налаштування додатку
    /// @return true, якщо щось змінилося (потрібно перемалювати)
    bool render(LayerStack& stack, AppSettings& settings);

    bool isOpen = true;
};

} // namespace NoiseArt
