#pragma once

#include "effects/Effect.h"
#include <nanovg.h>

namespace NoiseArt {

class ArtboardLayerEffect : public Effect, public ITransformable {
public:
    ArtboardLayerEffect();
    ~ArtboardLayerEffect() override = default;

    std::string getName() const override { return "Artboard"; }
    std::string getCategory() const override { return "Canvas"; }

    // Для растру ми нічого не робимо
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

private:
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 800.0f;
    float m_height = 600.0f;
    float m_opacity = 1.0f;
};

} // namespace NoiseArt
