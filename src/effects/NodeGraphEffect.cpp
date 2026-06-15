// ============================================================================
// NoiseArt — NodeGraphEffect (реалізація)
// ============================================================================
#include "effects/NodeGraphEffect.h"
#include "effects/ClipShape.h"
#include "core/Image.h"
#include <imgui.h>
#include <glad/glad.h>
#include <vector>
#include <cstring>
#include <cstddef>
#include <cstdint>

namespace NoiseArt {

bool NodeGraphEffect::renderUI() {
    ImGui::TextWrapped("Нодовий граф: %d нод.", static_cast<int>(m_graph.nodes().size()));
    ImGui::TextDisabled("Редактор з'явиться у View > Node Editor (Phase 3).");
    return false;
}

std::unique_ptr<Effect> NodeGraphEffect::clone() const {
    auto c = std::make_unique<NodeGraphEffect>();
    c->m_x = m_x; c->m_y = m_y; c->m_w = m_w; c->m_h = m_h;
    c->m_graph = m_graph;   // глибока копія (NodeGraph copy-assign)
    return c;
}

unsigned int NodeGraphEffect::renderAndGetTexture(float time, float opacity, const ClipShape& clip,
        const std::vector<std::pair<Effect*, float>>& filters, bool& flipV,
        const ShaderLayerEffect::ShaderRenderOpts& opts) {
    (void)opacity;   // прозорість застосовує viewport через tint
    const bool     hasPost = !filters.empty() || clip.type != ClipShape::None;
    const uint32_t RES     = (opts.lowResInteract && opts.interacting) ? 256u : 512u;

    NodeEvalContext ctx;
    ctx.width = ctx.height = static_cast<int>(RES);
    ctx.layerW = m_w; ctx.layerH = m_h;
    ctx.time = time;
    ctx.editGen = opts.editGen;
    ctx.interacting = opts.interacting;
    ctx.lowResInteract = opts.lowResInteract;
    ctx.cache = opts.cache;
    ctx.throttle = opts.throttle;
    ctx.throttleInterval = opts.throttleInterval;
    ctx.sourceTex = 0;
    ctx.quad = &m_quad;

    unsigned int graphTex = m_graph.evaluate(ctx);

    // Чистий GPU-граф без постобробки — віддаємо сиру FBO-текстуру (bottom-up).
    if (!hasPost) {
        flipV = true;
        return graphTex;
    }
    if (graphTex == 0) { flipV = true; return 0; }

    // Постобробка (фільтри/обрізання) — паритет із ShaderLayerEffect: зчитуємо
    // результат графа й проганяємо через ті самі CPU-фільтри + ClipShape.
    Image img;
    img.create(static_cast<int>(RES), static_cast<int>(RES), 4);
    GLint prevTex = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
    glBindTexture(GL_TEXTURE_2D, graphTex);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getData());
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));

    // FBO-текстура bottom-up → перевертаємо у top-down
    int W = img.getWidth(), H = img.getHeight();
    int rowBytes = W * 4;
    uint8_t* d = img.getData();
    std::vector<uint8_t> tmp(rowBytes);
    for (int y = 0; y < H / 2; ++y) {
        uint8_t* r0 = d + static_cast<size_t>(y) * rowBytes;
        uint8_t* r1 = d + static_cast<size_t>(H - 1 - y) * rowBytes;
        std::memcpy(tmp.data(), r0, rowBytes);
        std::memcpy(r0, r1, rowBytes);
        std::memcpy(r1, tmp.data(), rowBytes);
    }

    for (const auto& pr : filters) {
        Effect* f = pr.first; float op = pr.second;
        if (!f || op <= 0.0f) continue;
        Image res; f->apply(img, res);
        if (op >= 1.0f || res.getWidth() != img.getWidth() || res.getHeight() != img.getHeight()) {
            img = std::move(res);
        } else {
            uint8_t* a = img.getData(); const uint8_t* b = res.getData();
            int n = img.getWidth() * img.getHeight() * 4;
            for (int i = 0; i < n; ++i)
                a[i] = static_cast<uint8_t>(a[i] + op * (static_cast<float>(b[i]) - a[i]));
        }
    }
    applyClipMask(img, clip, getX(), getY(), getWidth(), getHeight());

    if (!m_processed) m_processed = std::make_shared<Texture>();
    m_processed->update(img);
    flipV = false;
    return m_processed->getID();
}

} // namespace NoiseArt
