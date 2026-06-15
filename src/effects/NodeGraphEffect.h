// ============================================================================
// NoiseArt — NodeGraphEffect (шар, що володіє нодовим графом)
// ============================================================================
// Phase 1 — прохідний (passthrough): з'являється у дереві шарів, клонується й
// відкочується, але ще не рендериться. GPU-евал + малювання у viewport додає
// Phase 2; візуальний редактор — Phase 3.
// ============================================================================
#pragma once

#include "effects/Effect.h"
#include "effects/ITransformable.h"
#include "effects/ShaderLayerEffect.h"   // ShaderRenderOpts + ClipShape (паритет сигнатури рендеру)
#include "graph/NodeGraph.h"
#include "graph/QuadRenderer.h"
#include "renderer/Texture.h"
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace NoiseArt {

class NodeGraphEffect : public Effect, public ITransformable {
public:
    std::string getName() const override     { return "Node Graph"; }
    std::string getCategory() const override { return "Backgrounds"; }

    void apply(const Image& input, Image& output) override { output = input; }  // passthrough

    bool isShader() const override { return true; }  // генератор (Phase 2 дасть GPU-слот у viewport)

    bool renderUI() override;
    std::unique_ptr<Effect> clone() const override;

    /// Обчислює граф у текстуру. Та сама сигнатура, що в ShaderLayerEffect — viewport
    /// викликає однаково. flipV=true → сира FBO-текстура; false → оброблена (top-down).
    unsigned int renderAndGetTexture(float time, float opacity, const ClipShape& clip,
                                     const std::vector<std::pair<Effect*, float>>& filters, bool& flipV,
                                     const ShaderLayerEffect::ShaderRenderOpts& opts);

    // ITransformable — геометрія на полотні (як у ShaderLayerEffect)
    ITransformable* getTransformable() override { return this; }
    float getX() const override { return m_x; }
    float getY() const override { return m_y; }
    float getWidth() const override { return m_w; }
    float getHeight() const override { return m_h; }
    void setPosition(float x, float y) override { m_x = x; m_y = y; }
    void setSize(float w, float h) override { m_w = w; m_h = h; }

    NodeGraph&       graph()       { return m_graph; }
    const NodeGraph& graph() const { return m_graph; }

private:
    float        m_x = 0.0f,   m_y = 0.0f;
    float        m_w = 400.0f, m_h = 300.0f;
    NodeGraph    m_graph;
    QuadRenderer m_quad;                    // спільний квад для нод-шейдерів
    std::shared_ptr<Texture> m_processed;   // результат readback-шляху (фільтри/обрізання)
};

} // namespace NoiseArt
