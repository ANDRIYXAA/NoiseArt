// ============================================================================
// NoiseArt — Клас Image (Реалізація)
// ============================================================================

#include "Image.h"
#include <iostream>

// Підключаємо stb_image тільки як заголовки (без реалізації)
#include <stb_image.h>
#include <stb_image_write.h>

namespace NoiseArt {

// ============================================================================
// Завантаження зображення
// ============================================================================
bool Image::loadFromFile(const std::string& path)
{
    // stb_image вміє перевертати зображення при завантаженні.
    // OpenGL використовує координати, де (0,0) — лівий НИЖНІЙ кут.
    // Але для редагування зручніше (0,0) — лівий ВЕРХНІЙ кут (як у Windows).
    // Тому ми кажемо stb НЕ перевертати (0).
    stbi_set_flip_vertically_on_load(0);

    // Завантажуємо зображення
    // 4-й аргумент (STBI_rgb_alpha) каже: "навіть якщо це JPG без прозорості,
    // додай канал A зі значенням 255".
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!data) {
        std::cerr << "[ПОМИЛКА] Не вдалося завантажити зображення: " << path << std::endl;
        std::cerr << "          Причина: " << stbi_failure_reason() << std::endl;
        return false;
    }

    // Зберігаємо властивості
    m_width = width;
    m_height = height;
    m_channels = 4; // Бо ми примусово використали STBI_rgb_alpha

    // Копіюємо дані з "сирого" вказівника stb_image у наш безпечний std::vector
    int totalBytes = m_width * m_height * m_channels;
    
    // resize виділяє потрібну кількість пам'яті
    m_pixels.resize(totalBytes);
    
    // std::copy копіює байти (це дуже швидко)
    std::copy(data, data + totalBytes, m_pixels.begin());

    // data нам більше не потрібен, звільняємо пам'ять stb
    stbi_image_free(data);

    std::cout << "[OK] Зображення завантажено: " << path 
              << " (" << m_width << "x" << m_height << ")" << std::endl;
    return true;
}

// ============================================================================
// Збереження зображення
// ============================================================================
bool Image::saveToFile(const std::string& path) const
{
    if (isEmpty()) {
        std::cerr << "[ПОМИЛКА] Спроба зберегти порожнє зображення!" << std::endl;
        return false;
    }

    // Зберігаємо як PNG (найкраще для редакторів — без втрати якості).
    // Аргументи: шлях, ширина, висота, канали, вказівник на дані, stride.
    // Stride — це кількість байтів в одному рядку пікселів (ширина * канали).
    int stride = m_width * m_channels;
    int success = stbi_write_png(path.c_str(), m_width, m_height, m_channels, m_pixels.data(), stride);

    if (!success) {
        std::cerr << "[ПОМИЛКА] Не вдалося зберегти зображення: " << path << std::endl;
        return false;
    }

    std::cout << "[OK] Зображення збережено: " << path << std::endl;
    return true;
}

// ============================================================================
// Створення порожнього зображення
// ============================================================================
void Image::create(int width, int height, int channels)
{
    m_width = width;
    m_height = height;
    m_channels = channels;
    
    // resize заповнить новий масив нулями (чорний колір, повністю прозорий)
    m_pixels.assign(width * height * channels, 0);
}

// ============================================================================
// Клонування
// ============================================================================
Image Image::clone() const
{
    Image copy;
    copy.m_width = m_width;
    copy.m_height = m_height;
    copy.m_channels = m_channels;
    
    // std::vector автоматично і швидко скопіює всі дані
    copy.m_pixels = m_pixels;
    return copy;
}

// ============================================================================
// Доступ до пікселів
// ============================================================================
uint8_t* Image::getPixel(int x, int y)
{
    // Формула 2D -> 1D індексу: (Y * Ширина + X) * Кількість каналів
    // Наприклад: щоб дістатись до 2-го пікселя (X=1, Y=0), ми пропускаємо
    // перші 4 байти (RGBA нульового пікселя).
    int index = (y * m_width + x) * m_channels;
    return &m_pixels[index];
}

const uint8_t* Image::getPixel(int x, int y) const
{
    int index = (y * m_width + x) * m_channels;
    return &m_pixels[index];
}

void Image::setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    uint8_t* pixel = getPixel(x, y);
    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
    pixel[3] = a;
}

} // namespace NoiseArt
