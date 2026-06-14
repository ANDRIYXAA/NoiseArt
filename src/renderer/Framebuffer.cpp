#include "Framebuffer.h"
#include <iostream>

namespace NoiseArt {

Framebuffer::Framebuffer() {
}

Framebuffer::~Framebuffer() {
    destroy();
}

void Framebuffer::create(uint32_t width, uint32_t height) {
    destroy();

    m_width = width;
    m_height = height;

    if (m_width == 0 || m_height == 0) return;

    glGenFramebuffers(1, &m_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // Color attachment
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

    // Stencil/Depth Renderbuffer (потрібно для векторного малювання, напр. NanoVG використовує stencil для winding)
    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_width, m_height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete!\n";
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroy() {
    if (m_fbo) {
        glDeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }
    if (m_colorTexture) {
        glDeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_rbo) {
        glDeleteRenderbuffers(1, &m_rbo);
        m_rbo = 0;
    }
}

void Framebuffer::resize(uint32_t width, uint32_t height) {
    if (m_width == width && m_height == height) return;
    create(width, height);
}

void Framebuffer::bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glViewport(0, 0, m_width, m_height);
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::begin() const {
    bind();
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Прозорий фон
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); // NanoVG потребує чистого stencil!
}

void Framebuffer::end() const {
    unbind();
}

} // namespace NoiseArt
