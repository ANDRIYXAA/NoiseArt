// ============================================================================
// NoiseArt — Config.h (Центральний файл налаштувань)
// ============================================================================
// ВСІ налаштувані константи програми зібрані тут.
// Щоб змінити поведінку — модифікуй значення нижче і перекомпілюй.
// Документація: README_SETTINGS.md у кореневій папці проєкту.
// ============================================================================

#pragma once

#include <imgui.h>

namespace NoiseArt::Config {

// =============================================
// WINDOW — Розміри вікна та заголовок
// =============================================
constexpr int         WINDOW_WIDTH         = 1280;
constexpr int         WINDOW_HEIGHT        = 720;
constexpr const char* WINDOW_TITLE         = "NoiseArt — Редактор зображень";

// =============================================
// GRID — Сітка
// =============================================
constexpr float GRID_DEFAULT_SIZE          = 100.0f;   // Базовий розмір клітинки (пікселі)
constexpr bool  GRID_SHOW_DEFAULT          = true;     // Показувати сітку при старті
constexpr bool  GRID_AUTO_SCALE_DEFAULT    = true;     // Автомасштаб сітки
constexpr bool  GRID_SNAP_DEFAULT          = false;    // Snap to grid при старті
constexpr float GRID_MIN_PIXEL_SIZE        = 4.0f;     // Мінімальний розмір клітинки на екрані

// Кольори сітки
constexpr ImU32 GRID_LINE_COLOR            = IM_COL32(200, 200, 200, 40);
constexpr ImU32 AXIS_X_COLOR               = IM_COL32(255, 100, 100, 80);
constexpr ImU32 AXIS_Y_COLOR               = IM_COL32(255, 100, 100, 80);

// =============================================
// ZOOM — Масштабування
// =============================================
constexpr float ZOOM_MIN                   = 0.05f;
constexpr float ZOOM_MAX                   = 20.0f;
constexpr float ZOOM_STEP                  = 1.15f;    // Множник при скролі

// =============================================
// TRANSFORM HANDLES — Ручки трансформації
// =============================================
constexpr float HANDLE_RADIUS              = 6.0f;     // Візуальний радіус ручки
constexpr float HANDLE_HIT_RADIUS          = 12.0f;    // Радіус кліку (більший для зручності)
constexpr ImU32 HANDLE_COLOR               = IM_COL32(0, 150, 255, 255);
constexpr ImU32 HANDLE_COLOR_MULTI         = IM_COL32(255, 200, 0, 200);  // Колір при мульти-виборі
constexpr float HANDLE_BORDER_WIDTH        = 2.0f;     // Ширина рамки

// =============================================
// SELECTION — Вибір
// =============================================
constexpr ImU32 SELECTION_COLOR            = IM_COL32(0, 150, 255, 255);
constexpr ImU32 MULTI_SELECTION_COLOR      = IM_COL32(255, 200, 0, 180);
constexpr float SELECTION_BORDER_WIDTH     = 2.0f;

// =============================================
// PANELS — Панелі (чи відкриті за замовчуванням)
// =============================================
constexpr bool  PANEL_VIEWPORT_OPEN        = true;
constexpr bool  PANEL_LAYERS_OPEN          = true;
constexpr bool  PANEL_EFFECTS_OPEN         = true;
constexpr bool  PANEL_PROPERTIES_OPEN      = true;
constexpr bool  PANEL_SETTINGS_OPEN        = false;

// =============================================
// LAYERS — Шари
// =============================================
constexpr float LAYER_DEFAULT_OPACITY      = 1.0f;
constexpr int   LAYER_TREE_INDENT          = 20;       // Відступ дочірніх шарів (пікселі)
constexpr float LAYER_DRAG_THRESHOLD       = 5.0f;     // Поріг для початку drag-and-drop

// =============================================
// NEW LAYER DEFAULTS — Розміри нових елементів
// =============================================
constexpr float NEW_RECT_WIDTH             = 200.0f;
constexpr float NEW_RECT_HEIGHT            = 200.0f;
constexpr float NEW_CIRCLE_RADIUS          = 100.0f;
constexpr float NEW_TEXT_FONT_SIZE         = 48.0f;
constexpr float NEW_TEXT_X                 = 200.0f;
constexpr float NEW_TEXT_Y                 = 200.0f;

// =============================================
// FONTS — Шрифти
// =============================================
constexpr float IMGUI_FONT_SIZE            = 18.0f;    // Розмір шрифту UI
constexpr const char* DEFAULT_FONT_PATH    = "resources/fonts/Inter.ttf";

} // namespace NoiseArt::Config
