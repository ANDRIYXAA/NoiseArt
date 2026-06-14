#pragma once

#include <glad/glad.h>
#include <cstdint>

namespace NoiseArt {

class Framebuffer {
public:
    Framebuffer();
    ~Framebuffer();

    // Забороняємо копіювання (RAII)
    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void create(uint32_t width, uint32_t height);
    void destroy();
    void resize(uint32_t width, uint32_t height);

    void bind() const;
    void unbind() const;

    // Зв'язуємо і очищуємо
    void begin() const;
    void end() const;

    uint32_t getColorAttachmentID() const { return m_colorTexture; }
    uint32_t getWidth() const { return m_width; }
    uint32_t getHeight() const { return m_height; }

private:
    uint32_t m_fbo = 0;
    uint32_t m_colorTexture = 0;
    uint32_t m_rbo = 0; // Renderbuffer для stencil (потрібно для NanoVG)

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};

} // namespace NoiseArt
