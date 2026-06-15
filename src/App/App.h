// ============================================================================
// NoiseArt — Клас App (Заголовок / Header)
// ============================================================================
// Цей файл ОГОЛОШУЄ клас App — говорить "що існує", але не "як працює".
// Реалізація (тіло методів) — у App.cpp.
// ============================================================================

#pragma once

// ===== Бібліотеки =====
#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <string>
#include <memory>
#include "nanovg.h"

// ===== Наші модулі =====
#include "core/Image.h"
#include "core/LayerStack.h"
#include "renderer/Framebuffer.h"
#include "core/Camera2D.h"
#include "core/History.h"
#include "effects/EffectRegistry.h"
#include "renderer/Texture.h"
#include "ui/panels/ViewportPanel.h"
#include "ui/panels/LayersPanel.h"
#include "ui/panels/EffectsPanel.h"
#include "ui/panels/PropertiesPanel.h"
#include "ui/panels/SettingsPanel.h"
#include "ui/panels/NodeEditorPanel.h"

namespace NoiseArt {

// ============================================================================
// Глобальні налаштування
// ============================================================================
struct AppSettings {
    bool showGrid = true;
    bool snapToGrid = false;
    bool autoGridScale = true;
    float gridSize = 100.0f;

    // ===== Оптимізація рендеру (усі вмикаються/вимикаються) =====
    bool optShaderCache = true;   // кешувати результат шейдера, перераховувати лише при зміні
    bool optThrottle    = true;   // важкі (анімовані) шари перераховувати не частіше ~30 FPS
    bool optLowResDrag  = true;   // під час перетягування рендерити шейдер у нижчій роздільності
};

// ============================================================================
// Клас App — серце програми
// ============================================================================
class App {
public:
    bool init();
    void run();
    void shutdown();

private:
    // ----- Методи головного циклу -----
    void processEvents();
    void beginFrame();
    void endFrame();

    // ----- Методи UI -----
    void renderMenuBar();       // Меню File/View/Help
    void renderUI();            // Всі панелі
    void updateProcessing();    // Перерахувати ефекти якщо потрібно

    // ----- Файлові операції -----
    void openImage(const std::string& path);
    void saveImage(const std::string& path);
    void processPendingFonts();   // обробити запит "Add Font..." (потрібен NanoVG-контекст)

    // ----- SDL -----
    SDL_Window* m_window = nullptr;
    SDL_GLContext m_glContext = nullptr;
    bool m_running = false;
    int m_windowWidth = 1280;
    int m_windowHeight = 720;

    // ----- Глобальні налаштування -----
    AppSettings m_settings;

    // ----- Дані зображення -----
    Image m_sourceImage;           // Оригінальне зображення
    Image m_resultImage;           // Результат після обробки
    std::string m_imagePath;       // Шлях до відкритого файлу

    // ----- Система ефектів -----
    LayerStack m_layerStack;       // Стек шарів
    EffectRegistry m_registry;     // Каталог ефектів

    // ----- Рендеринг -----
    Texture m_resultTexture;       // Текстура для відображення

    // NanoVG and FBO
    NVGcontext* m_vg = nullptr;
    Framebuffer m_fbo;
    Camera2D m_camera;
    History m_history;
    
    void renderCanvasToFBO();

    // ----- UI Панелі -----
    ViewportPanel m_viewportPanel;
    LayersPanel m_layersPanel;
    EffectsPanel m_effectsPanel;
    PropertiesPanel m_propertiesPanel;
    SettingsPanel m_settingsPanel;
    NodeEditorPanel m_nodeEditorPanel;
};

} // namespace NoiseArt
