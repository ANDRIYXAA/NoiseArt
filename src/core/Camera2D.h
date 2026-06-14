#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace NoiseArt {

class Camera2D {
public:
    Camera2D();

    void setPosition(const glm::vec2& position);
    const glm::vec2& getPosition() const { return m_position; }

    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }

    void setViewportSize(float width, float height);
    float getViewportWidth() const { return m_viewportWidth; }
    float getViewportHeight() const { return m_viewportHeight; }

    // Get the View Matrix (translates & scales the world based on camera pos and zoom)
    glm::mat4 getViewMatrix() const;

    // Get the Projection Matrix (Orthographic based on viewport size)
    glm::mat4 getProjectionMatrix() const;

    // Get View * Projection Matrix
    glm::mat4 getViewProjectionMatrix() const;

    // Convert Screen coordinates (mouse pos) to World coordinates
    glm::vec2 screenToWorld(const glm::vec2& screenPos) const;

    // Convert World coordinates to Screen coordinates
    glm::vec2 worldToScreen(const glm::vec2& worldPos) const;

private:
    void recalculateMatrices();

private:
    glm::vec2 m_position = { 0.0f, 0.0f };
    float m_zoom = 1.0f;
    float m_viewportWidth = 1280.0f;
    float m_viewportHeight = 720.0f;

    glm::mat4 m_viewMatrix = glm::mat4(1.0f);
    glm::mat4 m_projMatrix = glm::mat4(1.0f);
    glm::mat4 m_viewProjMatrix = glm::mat4(1.0f);

    bool m_dirty = true;
};

} // namespace NoiseArt
