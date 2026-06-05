// ============================================================================
// NoiseArt — Клас App (Реалізація / Implementation)
// ============================================================================
// Тут живе ВСЯ логіка класу App — ініціалізація, головний цикл, очищення.
// ============================================================================

#include "App.h"

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

// Стандартні бібліотеки
#include <iostream>
#include <filesystem>

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
        ImGui::DockSpaceOverViewport(0, ImGuiDockNodeFlags_PassthruCentralNode);

        // Меню
        renderMenuBar();

        // Панелі
        renderUI();

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
    std::string fileName = m_imagePath.empty() ? "" : 
        std::filesystem::path(m_imagePath).filename().string();
    m_viewportPanel.render(m_resultTexture, fileName);

    // Панель шарів
    if (m_layersPanel.render(m_layerStack)) {
        m_layerStack.setDirty();
    }

    // Каталог ефектів
    if (m_effectsPanel.render(m_registry, m_layerStack)) {
        m_layerStack.setDirty();
    }

    // Властивості
    if (m_propertiesPanel.render(m_layerStack)) {
        m_layerStack.setDirty();
    }
}

// ============================================================================
// updateProcessing() — Перерахувати ефекти якщо потрібно
// ============================================================================
void App::updateProcessing()
{
    if (!m_layerStack.isDirty()) return;
    if (m_sourceImage.isEmpty()) return;

    // Пропускаємо зображення через всі шари
    m_resultImage = m_layerStack.processAll(m_sourceImage);

    // Оновлюємо текстуру на GPU
    m_resultTexture.update(m_resultImage);
}

// ============================================================================
// openImage() — Відкрити зображення
// ============================================================================
void App::openImage(const std::string& path)
{
    if (m_sourceImage.loadFromFile(path)) {
        m_imagePath = path;
        m_resultImage = m_sourceImage.clone();
        m_resultTexture.createFromImage(m_resultImage);
        m_layerStack.setDirty();
        std::cout << "[OK] Зображення відкрито: " << path << std::endl;
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
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    if (m_glContext) SDL_GL_DestroyContext(m_glContext);
    if (m_window) SDL_DestroyWindow(m_window);
    SDL_Quit();
    std::cout << "NoiseArt завершено. До зустрічі!" << std::endl;
}

} // namespace NoiseArt
