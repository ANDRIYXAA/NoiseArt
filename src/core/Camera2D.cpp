#include "Camera2D.h"
#include <algorithm>

namespace NoiseArt {

Camera2D::Camera2D() {
    recalculateMatrices();
}

void Camera2D::setPosition(const glm::vec2& position) {
    m_position = position;
    m_dirty = true;
}

void Camera2D::setZoom(float zoom) {
    // Обмежуємо зум, щоб не було "вивернутого" простору або поділу на 0
    m_zoom = std::max(0.01f, std::min(zoom, 100.0f));
    m_dirty = true;
}

void Camera2D::setViewportSize(float width, float height) {
    m_viewportWidth = width;
    m_viewportHeight = height;
    m_dirty = true;
}

glm::mat4 Camera2D::getViewMatrix() const {
    if (m_dirty) { const_cast<Camera2D*>(this)->recalculateMatrices(); }
    return m_viewMatrix;
}

glm::mat4 Camera2D::getProjectionMatrix() const {
    if (m_dirty) { const_cast<Camera2D*>(this)->recalculateMatrices(); }
    return m_projMatrix;
}

glm::mat4 Camera2D::getViewProjectionMatrix() const {
    if (m_dirty) { const_cast<Camera2D*>(this)->recalculateMatrices(); }
    return m_viewProjMatrix;
}

void Camera2D::recalculateMatrices() {
    // Ортографічна матриця (0, 0 у верхньому лівому куті, як прийнято у 2D редакторах)
    m_projMatrix = glm::ortho(0.0f, m_viewportWidth, m_viewportHeight, 0.0f, -1.0f, 1.0f);

    // View Matrix: спочатку переміщуємо, потім масштабуємо
    // Щоб масштабування відбувалося відносно центру, це можна ускладнити, 
    // але поки що робимо базовий скейл і трансляцію.
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(m_position.x, m_position.y, 0.0f))
                        * glm::scale(glm::mat4(1.0f), glm::vec3(m_zoom, m_zoom, 1.0f));
    
    // View матриця - це зворотна до Transform матриці
    m_viewMatrix = glm::inverse(transform);
    
    m_viewProjMatrix = m_projMatrix * m_viewMatrix;
    m_dirty = false;
}

glm::vec2 Camera2D::screenToWorld(const glm::vec2& screenPos) const {
    if (m_dirty) { const_cast<Camera2D*>(this)->recalculateMatrices(); }
    
    // Для 2D (ортографічна) перетворення просте:
    // world = (screen / zoom) + position
    // Але через матрицю це надійніше.
    // Inverse ViewProjection перетворює з NDC [-1, 1] в World.
    
    // Спершу переводимо screenPos (де 0,0 - верхній лівий кут, ширина/висота екрану) в NDC
    float ndcX = (2.0f * screenPos.x) / m_viewportWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * screenPos.y) / m_viewportHeight; // Перевертаємо Y
    
    glm::vec4 ndcPos(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 worldPos = glm::inverse(m_viewProjMatrix) * ndcPos;
    
    return glm::vec2(worldPos.x, worldPos.y);
}

glm::vec2 Camera2D::worldToScreen(const glm::vec2& worldPos) const {
    if (m_dirty) { const_cast<Camera2D*>(this)->recalculateMatrices(); }
    
    glm::vec4 clipSpacePos = m_viewProjMatrix * glm::vec4(worldPos.x, worldPos.y, 0.0f, 1.0f);
    
    // Переводимо з NDC в екранні координати
    glm::vec2 ndcPos(clipSpacePos.x, clipSpacePos.y);
    
    float screenX = (ndcPos.x + 1.0f) * 0.5f * m_viewportWidth;
    float screenY = (1.0f - ndcPos.y) * 0.5f * m_viewportHeight;
    
    return glm::vec2(screenX, screenY);
}

} // namespace NoiseArt
