# ============================================================================
# NoiseArt — Кросплатформний редактор зображень
# ============================================================================
# Цей файл — "рецепт" збірки. CMake читає його і генерує файли для компілятора.
# Працює на Windows (Visual Studio / MSVC) і Linux (GCC / Make).
# ============================================================================

# Мінімальна версія CMake, яку ми підтримуємо
cmake_minimum_required(VERSION 3.20)

# Назва проєкту та мова програмування
project(NoiseArt
    VERSION 0.1.0
    LANGUAGES CXX
    DESCRIPTION "Image editor with layered effects system"
)

# ===== Стандарт C++ =====
# C++17 — сучасний стандарт з корисними фічами (std::optional, std::filesystem тощо)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Не дозволяти старіший стандарт
set(CMAKE_CXX_EXTENSIONS OFF)         # Не використовувати розширення компілятора (для кросплатформності)

# Генерувати compile_commands.json — файл, який IDE (VS Code, CLion) використовує
# щоб "розуміти" проєкт: знати шляхи включення, прапорці компіляції тощо.
# Без нього IDE показуватиме хибні помилки типу "file not found".
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===== Знаходимо бібліотеки =====
# find_package шукає бібліотеку, встановлену через vcpkg (або системну).
# CONFIG означає "шукай файл конфігурації CMake" (а не стару FindXxx.cmake).
# REQUIRED означає "якщо не знайдено — помилка збірки".

# SDL3 — вікна та введення (наступник SDL2, сучасніший API)
find_package(SDL3 CONFIG REQUIRED)

# Dear ImGui — інтерфейс користувача (з docking, SDL2 та OpenGL3 бекендами)
find_package(imgui CONFIG REQUIRED)

# GLAD — завантажувач функцій OpenGL
find_package(glad CONFIG REQUIRED)

# OpenGL — графіка (системна бібліотека, є на будь-якій ОС)
find_package(OpenGL REQUIRED)

# stb — завантаження/збереження зображень (header-only)
find_package(Stb REQUIRED)
find_package(nanovg CONFIG REQUIRED)

# ===== Створюємо виконуваний файл =====
# Тут перелічуємо ВСІ .cpp файли проєкту.
# Коли додаєш новий .cpp файл — додай його сюди!
add_executable(${PROJECT_NAME}
    # Точка входу
    src/main.cpp

    # Застосунок (Game Loop)
    src/App/App.cpp

    # Ядро (Core)
    src/core/stb_impl.cpp
    src/core/Image.cpp
    src/core/Layer.cpp
    src/core/LayerStack.cpp
    src/core/Camera2D.cpp
    src/core/History.cpp
    src/core/KeybindManager.cpp

    # Ефекти
    src/effects/EffectRegistry.cpp
    src/effects/ArtboardLayerEffect.cpp
    src/effects/ShaderLayerEffect.cpp
    src/effects/TextLayerEffect.cpp
    src/effects/VectorLayerEffect.cpp

    # Рендеринг
    src/renderer/Texture.cpp
    src/renderer/Framebuffer.cpp
    src/renderer/Shader.cpp

    # UI Панелі
    src/ui/panels/ViewportPanel.cpp
    src/ui/panels/LayersPanel.cpp
    src/ui/panels/EffectsPanel.cpp
    src/ui/panels/PropertiesPanel.cpp
    src/ui/panels/SettingsPanel.cpp
)

# ===== Шляхи для #include =====
# Щоб писати #include "App/App.h" замість #include "../App/App.h"
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src     # Наші файли
    ${Stb_INCLUDE_DIR}          # stb_image.h
)

# ===== Підключаємо бібліотеки =====
# target_link_libraries "з'єднує" наш проєкт з бібліотеками.
# PRIVATE означає "тільки для нашого проєкту" (не для тих, хто нас використовує).
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3              # SDL3 основна бібліотека
    imgui::imgui            # Dear ImGui з бекендами
    glad::glad              # GLAD — завантажувач OpenGL
    OpenGL::GL              # Системна OpenGL бібліотека
    nanovg::nanovg
)

# Копіюємо папку resources у вихідну директорію після збірки
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/resources"
)

# ===== Кросплатформні налаштування =====
# На Windows ховаємо консольне вікно в Release режимі
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>  # В Release — без консолі
    )
endif()

# Виводимо інформацію про збірку
message(STATUS "========================================")
message(STATUS "  NoiseArt v${PROJECT_VERSION}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "========================================")
