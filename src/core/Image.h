// ============================================================================
// NoiseArt — Клас Image (Заголовок)
// ============================================================================
// Цей клас — "контейнер" для пікселів зображення.
// Він завантажує файл (PNG/JPG) у пам'ять, дозволяє читати/змінювати
// кожен піксель і зберігати результат назад у файл.
// ============================================================================

#pragma once

#include <string>
#include <vector>
#include <cstdint> // для uint8_t (беззнаковий 8-бітний цілий тип, від 0 до 255)

namespace NoiseArt {

class Image {
public:
    // ----- Завантаження та збереження -----

    /// Завантажує зображення з файлу. Повертає true, якщо успішно.
    bool loadFromFile(const std::string& path);

    /// Зберігає зображення у PNG-файл. Повертає true, якщо успішно.
    bool saveToFile(const std::string& path) const;

    // ----- Створення та копіювання -----

    /// Створює порожнє чорне зображення заданого розміру
    void create(int width, int height, int channels = 4);

    /// Очищує все зображення заданим кольором
    void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
        if (isEmpty()) return;
        for (size_t i = 0; i < m_pixels.size(); i += m_channels) {
            m_pixels[i] = r;
            m_pixels[i+1] = g;
            m_pixels[i+2] = b;
            if (m_channels >= 4) m_pixels[i+3] = a;
        }
    }

    /// Створює точну копію зображення (з усіма пікселями)
    Image clone() const;

    // ----- Доступ до пікселів -----

    /// Повертає вказівник на початок пікселя (R, G, B, A) за координатами (X, Y)
    /// Координата (0, 0) — це лівий верхній кут.
    uint8_t* getPixel(int x, int y);
    const uint8_t* getPixel(int x, int y) const;

    /// Зручна функція для встановлення кольору одного пікселя
    void setPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);

    // ----- Властивості -----

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    int getChannels() const { return m_channels; }
    
    /// Повертає вказівник на ВЕСЬ масив пікселів (потрібно для OpenGL текстур)
    uint8_t* getData() { return m_pixels.data(); }
    const uint8_t* getData() const { return m_pixels.data(); }
    
    /// Перевіряє чи завантажене якесь зображення
    bool isEmpty() const { return m_pixels.empty(); }

private:
    int m_width = 0;
    int m_height = 0;
    
    // Ми ЗАВЖДИ використовуємо 4 канали (RGBA). Навіть якщо картинка RGB (без прозорості),
    // ми примусово додаємо 4-й канал при завантаженні, щоб спростити обробку.
    int m_channels = 4; 

    // Масив усіх пікселів.
    // Розмір масиву = width * height * channels.
    // Зберігається підряд: [R, G, B, A,  R, G, B, A,  R, G, B, A...]
    std::vector<uint8_t> m_pixels;
};

} // namespace NoiseArt
