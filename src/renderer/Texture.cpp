// ============================================================================
// NoiseArt — OpenGL Текстура (Реалізація)
// ============================================================================

#include "Texture.h"
#include <iostream>

namespace NoiseArt {

Texture::~Texture() { cleanup(); }

Texture::Texture(Texture&& other) noexcept
    : m_textureID(other.m_textureID), m_width(other.m_width), m_height(other.m_height)
{
    other.m_textureID = 0; // Попередній об'єкт більше не "володіє" текстурою
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other) {
        cleanup();
        m_textureID = other.m_textureID;
        m_width = other.m_width;
        m_height = other.m_height;
        other.m_textureID = 0;
    }
    return *this;
}

// ============================================================================
// createFromImage() — Створити нову текстуру з Image
// ============================================================================
void Texture::createFromImage(const Image& image)
{
    // Видаляємо стару текстуру, якщо є
    cleanup();

    if (image.isEmpty()) return;

    m_width = image.getWidth();
    m_height = image.getHeight();

    // Крок 1: Генеруємо ID текстури
    // OpenGL працює через числові ID — кожна текстура має унікальний номер
    glGenTextures(1, &m_textureID);

    // Крок 2: "Прив'язуємо" текстуру — робимо її активною
    // GL_TEXTURE_2D — тип текстури (2D картинка)
    glBindTexture(GL_TEXTURE_2D, m_textureID);

    // Крок 3: Налаштування фільтрації
    // Що робити, коли текстура відображається НЕ в рідному розмірі:
    // GL_LINEAR — лінійна інтерполяція (плавне масштабування)
    // GL_NEAREST — найближчий піксель (піксельний вигляд, як у ретро-іграх)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Що робити на краях текстури:
    // GL_CLAMP_TO_EDGE — повторювати крайній піксель (без артефактів)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Крок 4: Завантажуємо пікселі з RAM у GPU
    // Аргументи:
    //   GL_TEXTURE_2D — тип текстури
    //   0 — рівень мипмапа (0 = оригінал)
    //   GL_RGBA — внутрішній формат (як GPU зберігатиме)
    //   width, height — розміри
    //   0 — border (завжди 0 у сучасному OpenGL)
    //   GL_RGBA — формат вхідних даних
    //   GL_UNSIGNED_BYTE — тип даних (uint8_t = 0-255)
    //   getData() — вказівник на масив пікселів
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 m_width, m_height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE,
                 image.getData());

    // Відв'язуємо текстуру (хороша практика)
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// update() — Оновити пікселі без перестворення текстури
// ============================================================================
void Texture::update(const Image& image)
{
    if (m_textureID == 0 || image.isEmpty()) {
        createFromImage(image);
        return;
    }

    // Якщо розмір змінився — перестворюємо
    if (image.getWidth() != m_width || image.getHeight() != m_height) {
        createFromImage(image);
        return;
    }

    // glTexSubImage2D — оновлює ЧАСТИНУ текстури (або всю)
    // Швидше за glTexImage2D, бо не перевиділяє пам'ять на GPU
    glBindTexture(GL_TEXTURE_2D, m_textureID);
    glTexSubImage2D(GL_TEXTURE_2D, 0,
                    0, 0, m_width, m_height,
                    GL_RGBA, GL_UNSIGNED_BYTE,
                    image.getData());
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================================
// cleanup() — Звільнити ресурс GPU
// ============================================================================
void Texture::cleanup()
{
    if (m_textureID != 0) {
        glDeleteTextures(1, &m_textureID);
        m_textureID = 0;
    }
    m_width = 0;
    m_height = 0;
}

} // namespace NoiseArt
