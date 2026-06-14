#pragma once
#include <string>
#include "renderer/Texture.h"

namespace NoiseArt {

class AppSettings;

class SettingsPanel {
public:
    SettingsPanel();
    
    // Відображає панель налаштувань
    void render(bool* open, AppSettings& settings);

    bool isOpen = false;  // За замовчуванням закрита

    // Отримати колір фону
    float* getClearColor() { return m_clearColor; }
    
    // Відображає фонове зображення
    void renderBackground(float screenWidth, float screenHeight);

private:
    float m_clearColor[4] = {0.1f, 0.1f, 0.1f, 1.0f};
    bool m_hasBgImage = false;
    Texture m_bgTexture;

    std::string m_recordingAction = "";
};

} // namespace NoiseArt
