#pragma once

#include "effects/Effect.h"
#include "renderer/Texture.h"
#include "effects/ClipShape.h"
#include <nanovg.h>
#include <memory>
#include <vector>
#include <utility>

namespace NoiseArt {

class VectorLayerEffect : public Effect, public ITransformable {
public:
    VectorLayerEffect();
    ~VectorLayerEffect() override = default;

    std::string getName() const override { return "Vector Shape"; }
    std::string getCategory() const override { return "Vector"; }

    // Для растру ми нічого не робимо, просто копіюємо вхідне зображення
    void apply(const Image& input, Image& output) override;

    // Векторні методи
    bool isVector() const override { return true; }
    void renderVector(NVGcontext* vg) override;

    bool renderUI() override;

    std::unique_ptr<Effect> clone() const override;

    ITransformable* getTransformable() override { return this; }

    // ITransformable
    float getX() const override { return m_x; }
    float getY() const override { return m_y; }
    float getWidth() const override { return m_width; }
    float getHeight() const override { return m_height; }
    void setPosition(float x, float y) override { m_x = x; m_y = y; }
    void setSize(float w, float h) override { m_width = w; m_height = h; }

    // Властивості форми
    enum class ShapeType {
        Rectangle,
        Circle,
        RoundedRectangle
    };

    // Геттери для візуалізації у viewport
    ShapeType getShapeType() const { return m_shapeType; }
    void setShapeType(ShapeType t) { m_shapeType = t; }
    const float* getFillColor() const { return m_color; }
    bool isFill() const { return m_fill; }
    bool hasStroke() const { return m_stroke; }
    const float* getStrokeColor() const { return m_strokeColor; }
    float getStrokeWidth() const { return m_strokeWidth; }
    float getRadius() const { return m_radius; }

    // --- Ефекти на вектор (модель "контент своєї групи") ---
    /// Растеризує фігуру у RGBA-зображення (для застосування фільтрів на CPU)
    void rasterize(Image& out) const;
    /// Застосовує дочірні растрові фільтри до растеризованої фігури → кеш-текстура
    void applyFilters(const std::vector<std::pair<Effect*, float>>& filters, const ClipShape& clip = {});
    /// Текстура з накладеними фільтрами (nullptr якщо фільтрів немає)
    const Texture* getProcessedTexture() const {
        return (m_processed && m_processed->isValid()) ? m_processed.get() : nullptr;
    }

private:
    ShapeType m_shapeType = ShapeType::Rectangle;
    
    // Координати та розміри
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 200.0f;
    float m_height = 200.0f;
    float m_radius = 20.0f; // Для RoundedRectangle
    
    // Стиль
    float m_color[4] = {1.0f, 0.0f, 0.0f, 1.0f};
    bool m_fill = true;
    
    bool m_stroke = false;
    float m_strokeColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    float m_strokeWidth = 2.0f;

    std::shared_ptr<Texture> m_processed;   // кеш: фігура з накладеними фільтрами
};

} // namespace NoiseArt
