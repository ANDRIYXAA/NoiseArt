// ============================================================================
// NoiseArt — OpenGL Текстура (RAII обгортка)
// ============================================================================
// Текстура — це зображення, завантажене в пам'ять відеокарти (GPU).
// Щоб показати Image на екрані через ImGui, потрібно:
// 1. Створити OpenGL текстуру (glGenTextures)
// 2. Завантажити в неї пікселі (glTexImage2D)
// 3. Передати ImGui ID текстури (ImGui::Image)
//
// RAII (Resource Acquisition Is Initialization):
// Текстура автоматично видаляється в деструкторі — не потрібно
// пам'ятати про glDeleteTextures!
// ============================================================================

#pragma once

#include <glad/glad.h>
#include "core/Image.h"

namespace NoiseArt {

class Texture {
public:
    Texture() = default;
    ~Texture();

    // Заборона копіювання (текстура — унікальний ресурс GPU)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Переміщення дозволене
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    /// Створити текстуру з Image (завантажити пікселі на GPU)
    void createFromImage(const Image& image);

    /// Оновити пікселі (без перестворення текстури — швидше)
    void update(const Image& image);

    /// ID текстури для ImGui::Image()
    GLuint getID() const { return m_textureID; }

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    bool isValid() const { return m_textureID != 0; }

    void cleanup();

private:

    GLuint m_textureID = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace NoiseArt
