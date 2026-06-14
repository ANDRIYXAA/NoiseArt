// ============================================================================
// NoiseArt — Клас App (Реалізація / Implementation)
// ============================================================================
// Тут живе ВСЯ логіка класу App — ініціалізація, головний цикл, очищення.
// ============================================================================

#include "App.h"
#include <glad/glad.h>
#define NANOVG_GL3_IMPLEMENTATION
#include <nanovg_gl.h>

// ImGui
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_opengl3.h>

// Ефекти (для реєстрації)
#include "effects/NoiseEffect.h"
#include "effects/BloomEffect.h"
#include "effects/BlurEffect.h"
#include "effects/BrightnessContrast.h"
#include "effects/InvertEffect.h"
#include "effects/AdjustmentsEffect.h"
#include "effects/BlockifyEffect.h"
#include "effects/ThresholdEffect.h"
#include "effects/TextLayerEffect.h"
#include "effects/VectorLayerEffect.h"
#include "effects/ArtboardLayerEffect.h"
#include "effects/ShaderLayerEffect.h"
#include "effects/OverlayEffect.h"

// Стандартні бібліотеки
#include <iostream>
#include <filesystem>
#include <utility>
#include <vector>
#include <portable-file-dialogs.h>

// Для діалогу відкриття файлу на Windows
#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#endif

namespace NoiseArt {

// ============================================================================
// init() — Ініціалізація всіх підсистем
// ============================================================================
bool App::init()
{
    // ===== SDL3 =====
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "[ПОМИЛКА] SDL_Init: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "[OK] SDL3 ініціалізовано" << std::endl;

    // ===== OpenGL атрибути =====
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // ===== Вікно =====
    m_window = SDL_CreateWindow(
        "NoiseArt — Редактор зображень",
        m_windowWidth, m_windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED
    );
    if (!m_window) {
        std::cerr << "[ПОМИЛКА] SDL_CreateWindow: " << SDL_GetError() << std::endl;
        return false;
    }
    std::cout << "[OK] Вікно створено" << std::endl;

    // ===== OpenGL контекст =====
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        std::cerr << "[ПОМИЛКА] SDL_GL_CreateContext: " << SDL_GetError() << std::endl;
        return false;
    }
    SDL_GL_MakeCurrent(m_window, m_glContext);
    SDL_GL_SetSwapInterval(1);
    std::cout << "[OK] OpenGL контекст створено" << std::endl;

    // ===== GLAD =====
    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "[ПОМИЛКА] gladLoadGLLoader!" << std::endl;
        return false;
    }
    std::cout << "[OK] OpenGL " << GLVersion.major << "." << GLVersion.minor
              << " завантажено через GLAD" << std::endl;
    std::cout << "     GPU: " << glGetString(GL_RENDERER) << std::endl;

    // ===== NanoVG та FBO =====
    m_vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    if (!m_vg) {
        std::cerr << "[ПОМИЛКА] Не вдалося ініціалізувати NanoVG!" << std::endl;
        return false;
    }
    m_fbo.create(800, 600);

    // Завантажуємо шрифти для NanoVG (для текстового шару)
    nvgCreateFont(m_vg, "Inter", "resources/fonts/Inter.ttf");
    // Системні шрифти Windows
    nvgCreateFont(m_vg, "Arial", "C:/Windows/Fonts/arial.ttf");
    nvgCreateFont(m_vg, "Times New Roman", "C:/Windows/Fonts/times.ttf");
    nvgCreateFont(m_vg, "Courier New", "C:/Windows/Fonts/cour.ttf");
    nvgCreateFont(m_vg, "Georgia", "C:/Windows/Fonts/georgia.ttf");
    nvgCreateFont(m_vg, "Verdana", "C:/Windows/Fonts/verdana.ttf");
    nvgCreateFont(m_vg, "Trebuchet MS", "C:/Windows/Fonts/trebuc.ttf");
    nvgCreateFont(m_vg, "Impact", "C:/Windows/Fonts/impact.ttf");
    nvgCreateFont(m_vg, "Comic Sans MS", "C:/Windows/Fonts/comic.ttf");
    nvgCreateFont(m_vg, "Segoe UI", "C:/Windows/Fonts/segoeui.ttf");
    nvgCreateFont(m_vg, "Consolas", "C:/Windows/Fonts/consola.ttf");
    std::cout << "[OK] Шрифти завантажено" << std::endl;

    // ===== ImGui =====
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename = "imgui_layout.ini";

    // Шрифт з підтримкою кирилиці
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 1;
    io.Fonts->AddFontFromFileTTF(
        "resources/fonts/Inter.ttf", 16.0f,
        &fontConfig, io.Fonts->GetGlyphRangesCyrillic()
    );

    // Темна тема з кастомними кольорами
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.WindowPadding = ImVec2(10, 10);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]        = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBgActive]   = ImVec4(0.15f, 0.12f, 0.22f, 1.00f);
    colors[ImGuiCol_Tab]             = ImVec4(0.15f, 0.12f, 0.22f, 1.00f);
    colors[ImGuiCol_TabSelected]     = ImVec4(0.30f, 0.20f, 0.50f, 1.00f);
    colors[ImGuiCol_FrameBg]         = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_Button]          = ImVec4(0.25f, 0.20f, 0.35f, 1.00f);
    colors[ImGuiCol_ButtonHovered]   = ImVec4(0.35f, 0.28f, 0.50f, 1.00f);
    colors[ImGuiCol_ButtonActive]    = ImVec4(0.45f, 0.35f, 0.60f, 1.00f);
    colors[ImGuiCol_Header]          = ImVec4(0.25f, 0.20f, 0.35f, 1.00f);
    colors[ImGuiCol_HeaderHovered]   = ImVec4(0.35f, 0.28f, 0.50f, 1.00f);
    colors[ImGuiCol_HeaderActive]    = ImVec4(0.45f, 0.35f, 0.60f, 1.00f);
    colors[ImGuiCol_Separator]       = ImVec4(0.25f, 0.22f, 0.30f, 1.00f);
    colors[ImGuiCol_DockingPreview]  = ImVec4(0.45f, 0.30f, 0.70f, 0.70f);
    colors[ImGuiCol_MenuBarBg]       = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);

    // Бекенди ImGui
    ImGui_ImplSDL3_InitForOpenGL(m_window, m_glContext);
    ImGui_ImplOpenGL3_Init("#version 150");

    // ===== Реєстрація ефектів =====
    m_registry.registerEffect<NoiseEffect>();
    m_registry.registerEffect<BrightnessContrast>();
    m_registry.registerEffect<InvertEffect>();
    m_registry.registerEffect<BlurEffect>();
    m_registry.registerEffect<BloomEffect>();
    m_registry.registerEffect<AdjustmentsEffect>();
    m_registry.registerEffect<BlockifyEffect>();
    m_registry.registerEffect<ThresholdEffect>();
    m_registry.registerEffect<TextLayerEffect>();
    m_registry.registerEffect<VectorLayerEffect>();
    m_registry.registerEffect<ArtboardLayerEffect>();
    m_registry.registerEffect<ShaderLayerEffect>();

    // ===== Готово! =====
    m_running = true;
    std::cout << "\n======================================"  << std::endl;
    std::cout << "  NoiseArt ініціалізовано успішно!"       << std::endl;
    std::cout << "  Відкрий зображення через File > Open"   << std::endl;
    std::cout << "======================================\n" << std::endl;
    return true;
}

// ============================================================================
// run() — Головний цикл
// ============================================================================
void App::run()
{
    while (m_running) {
        processEvents();
        beginFrame();

        // Dockspace — робоча область для прикріплення панелей.
        // PassthruCentralNode — дозволяє панелям "плавати" вільно,
        // як звичайні вікна в ОС. Щоб прикріпити панель — перетягни
        // її до краю вікна (з'явиться синя підсвітка).
        ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

        // Меню
        renderMenuBar();

        // Рендеримо вектори у FBO
        renderCanvasToFBO();

        // Панелі
        renderUI();

        // Запит на додавання шрифту (кнопка "Add Font...")
        processPendingFonts();

        // Обробка (перерахунок ефектів якщо dirty)
        updateProcessing();

        endFrame();
    }
}

// ============================================================================
// renderMenuBar() — Головне меню
// ============================================================================
void App::renderMenuBar()
{
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Open...", "Ctrl+O")) {
                // Діалог відкриття файлу
                #ifdef _WIN32
                OPENFILENAMEA ofn = {};
                char fileName[260] = "";
                ofn.lStructSize = sizeof(ofn);
                ofn.lpstrFilter = "Images\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = sizeof(fileName);
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    openImage(fileName);
                }
                #endif
            }
            if (ImGui::MenuItem("Save As...", "Ctrl+S")) {
                #ifdef _WIN32
                OPENFILENAMEA ofn = {};
                char fileName[260] = "";
                ofn.lStructSize = sizeof(ofn);
                ofn.lpstrFilter = "PNG\0*.png\0";
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = sizeof(fileName);
                ofn.lpstrDefExt = "png";
                ofn.Flags = OFN_OVERWRITEPROMPT;
                if (GetSaveFileNameA(&ofn)) {
                    saveImage(fileName);
                }
                #endif
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "ESC")) {
                m_running = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Viewport", nullptr, &m_viewportPanel.isOpen);
            ImGui::MenuItem("Layers", nullptr, &m_layersPanel.isOpen);
            ImGui::MenuItem("Effects", nullptr, &m_effectsPanel.isOpen);
            ImGui::MenuItem("Properties", nullptr, &m_propertiesPanel.isOpen);
            ImGui::MenuItem("Settings", nullptr, &m_settingsPanel.isOpen);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // Поки нічого
            }
            ImGui::EndMenu();
        }

        // FPS в правому куті меню
        float fps = ImGui::GetIO().Framerate;
        char fpsText[32];
        snprintf(fpsText, sizeof(fpsText), "FPS: %.0f", fps);
        float textWidth = ImGui::CalcTextSize(fpsText).x;
        ImGui::SameLine(ImGui::GetWindowWidth() - textWidth - 20);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", fpsText);

        ImGui::EndMainMenuBar();
    }
}

// ============================================================================
// renderUI() — Малювання всіх панелей
// ============================================================================
void App::renderUI()
{
    // Viewport (головний перегляд)
    m_viewportPanel.render(m_resultTexture, m_fbo, m_imagePath, m_layerStack, m_camera, m_history, m_settings);

    // Панель шарів
    if (m_layersPanel.render(m_layerStack)) {
        m_layerStack.setDirty();
    }

    // Каталог ефектів
    if (m_effectsPanel.render(m_registry, m_layerStack)) {
        m_layerStack.setDirty();
    }

    // Панель властивостей вибраного шару
    if (m_propertiesPanel.render(m_layerStack, m_settings)) {
        // Будь-яка зміна параметрів → перерахувати (фільтри фото тощо)
        m_layerStack.setDirty(true);
    }

    // Панель налаштувань
    m_settingsPanel.render(&m_settingsPanel.isOpen, m_settings);
}

// ============================================================================
// updateProcessing() — Перерахувати ефекти якщо потрібно
// ============================================================================
void App::updateProcessing()
{
    float zoom = m_camera.getZoom();
    bool dirty = m_layerStack.isDirty();

    // Модель "контент своєї групи": для кожного фото-шару застосовуємо його
    // дочірні растрові фільтри (Noise, Blur, Invert тощо) до пікселів фото.
    auto flat = m_layerStack.flattenTree();
    for (auto& e : flat) {
        if (!e.layer || !e.layer->getEffect()) continue;
        Effect* eff = e.layer->getEffect();
        auto ov = dynamic_cast<OverlayEffect*>(eff);
        auto ve = dynamic_cast<VectorLayerEffect*>(eff);
        auto te = dynamic_cast<TextLayerEffect*>(eff);
        if (!ov && !ve && !te) continue;   // обробка для фото, векторів і тексту

        // На не-dirty кадрах переростеризовуємо ЛИШЕ текст під зум (для чіткості)
        if (!dirty && !(te && te->needsRerasterAtZoom(zoom))) continue;

        // Фільтри: власні дочірні + успадковані від груп-предків (модель "контент своєї групи").
        // Тобто фільтр, покладений у групу/артборд, діє на весь контент усередині.
        std::vector<std::pair<Effect*, float>> filters;
        for (Layer* container = e.layer; container; container = container->getParent()) {
            for (auto& child : container->getChildren()) {
                if (!child || !child->isEnabled()) continue;
                Effect* ce = child->getEffect();
                // Растровий фільтр = не вектор, не шейдер, не інше фото
                if (ce && !ce->isVector() && !ce->isShader() && !dynamic_cast<OverlayEffect*>(ce)) {
                    filters.push_back({ ce, child->getOpacity() });
                }
            }
        }
        // Обрізання по формі батька, якщо батько — коло / заокруглений вектор
        ClipShape clip;
        if (e.parent && e.parent->getEffect()) {
            if (auto pv = dynamic_cast<VectorLayerEffect*>(e.parent->getEffect())) {
                if (pv->getShapeType() == VectorLayerEffect::ShapeType::Circle) {
                    clip.type = ClipShape::Ellipse;
                    clip.parentW = pv->getWidth(); clip.parentH = pv->getHeight();
                } else if (pv->getShapeType() == VectorLayerEffect::ShapeType::RoundedRectangle) {
                    clip.type = ClipShape::Rounded;
                    clip.parentW = pv->getWidth(); clip.parentH = pv->getHeight();
                    clip.radius = pv->getRadius();
                }
            }
        }

        if (ov) ov->applyFilters(filters, clip);
        else if (ve) ve->applyFilters(filters, clip);
        else te->rasterizeAndProcess(m_vg, filters, clip, zoom);
    }

    if (dirty) m_layerStack.setDirty(false);
}

// ============================================================================
// openImage() — Відкрити зображення як новий шар
// ============================================================================
void App::openImage(const std::string& path)
{
    // Створюємо OverlayEffect
    auto effect = std::make_unique<OverlayEffect>();
    if (effect->loadOverlayImage(path)) {
        auto layer = std::make_unique<Layer>(std::move(effect));
        layer->setName(std::filesystem::path(path).filename().string());
        m_layerStack.addLayer(std::move(layer));
        m_layerStack.setDirty(true);
        std::cout << "[OK] Зображення відкрито як шар: " << path << std::endl;
    } else {
        std::cerr << "[ПОМИЛКА] Не вдалося відкрити: " << path << std::endl;
    }
}

// ============================================================================
// saveImage() — Зберегти результат
// ============================================================================
void App::saveImage(const std::string& path)
{
    if (m_resultImage.isEmpty()) {
        std::cerr << "[ПОМИЛКА] Немає зображення для збереження!" << std::endl;
        return;
    }
    m_resultImage.saveToFile(path);
}

// ============================================================================
// processPendingFonts() — додавання шрифту через діалог + NanoVG
// ============================================================================
void App::processPendingFonts()
{
    if (!TextLayerEffect::fontAddRequested()) return;
    TextLayerEffect::fontAddRequested() = false;

    auto res = pfd::open_file("Choose a font", "",
        { "Fonts (.ttf .otf)", "*.ttf *.otf", "All Files", "*" }).result();
    if (res.empty()) return;

    std::string path = res[0];
    std::string name = std::filesystem::path(path).stem().string();
    if (name.empty()) return;

    if (m_vg && nvgCreateFont(m_vg, name.c_str(), path.c_str()) != -1) {
        TextLayerEffect::fonts().push_back(name);
        m_layerStack.setDirty(true);
        std::cout << "[OK] Шрифт додано: " << name << std::endl;
    } else {
        std::cerr << "[ПОМИЛКА] Не вдалося завантажити шрифт: " << path << std::endl;
    }
}

// ============================================================================
// processEvents()
// ============================================================================
void App::processEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT) {
            m_running = false;
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) {
                m_running = false;
            }
            // Ctrl+O — відкрити файл
            if (event.key.key == SDLK_O && (event.key.mod & SDL_KMOD_CTRL)) {
                #ifdef _WIN32
                OPENFILENAMEA ofn = {};
                char fileName[260] = "";
                ofn.lStructSize = sizeof(ofn);
                ofn.lpstrFilter = "Images\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All Files\0*.*\0";
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = sizeof(fileName);
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameA(&ofn)) {
                    openImage(fileName);
                }
                #endif
            }
        }
        if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            m_windowWidth = event.window.data1;
            m_windowHeight = event.window.data2;
        }

        // Drag & Drop підтримка
        if (event.type == SDL_EVENT_DROP_FILE) {
            const char* droppedFile = event.drop.data;
            if (droppedFile) {
                openImage(droppedFile);
            }
        }
    }
}

// ============================================================================
// beginFrame() / endFrame()
// ============================================================================
void App::beginFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void App::endFrame()
{
    ImGui::Render();
    SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    glViewport(0, 0, m_windowWidth, m_windowHeight);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(m_window);
}

// ============================================================================
// shutdown()
// ============================================================================
void App::shutdown()
{
    // Звільняємо GPU-ресурси шарів та історії ПОКИ OpenGL контекст ще живий
    m_history.clear();
    m_layerStack.clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (m_glContext) SDL_GL_DestroyContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
    std::cout << "NoiseArt завершено. До зустрічі!" << std::endl;
}

// ============================================================================
// renderCanvasToFBO() — Рендеринг векторів
// ============================================================================
void App::renderCanvasToFBO()
{
    uint32_t vpWidth = static_cast<uint32_t>(m_camera.getViewportWidth());
    uint32_t vpHeight = static_cast<uint32_t>(m_camera.getViewportHeight());
    if (vpWidth == 0 || vpHeight == 0) return;

    if (m_fbo.getWidth() != vpWidth || m_fbo.getHeight() != vpHeight) {
        m_fbo.resize(vpWidth, vpHeight);
    }

    // Очищуємо FBO
    m_fbo.bind();
    glViewport(0, 0, vpWidth, vpHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    nvgBeginFrame(m_vg, vpWidth, vpHeight, 1.0f);

    // Застосовуємо трансформацію камери (Global Coordinates)
    float zoom = m_camera.getZoom();
    float imgDrawX = vpWidth * 0.5f + m_camera.getPosition().x;
    float imgDrawY = vpHeight * 0.5f + m_camera.getPosition().y;

    nvgTranslate(m_vg, imgDrawX, imgDrawY);
    nvgScale(m_vg, zoom, zoom);

    // Рендеримо всі векторні ефекти
    for (int i = 0; i < m_layerStack.getLayerCount(); ++i) {
        auto layer = m_layerStack.getLayer(i);
        if (layer->isEnabled() && layer->getEffect() && layer->getEffect()->isVector()) {
            layer->getEffect()->renderVector(m_vg);
        }
    }

    nvgEndFrame(m_vg);
    m_fbo.unbind();
}

} // namespace NoiseArt
