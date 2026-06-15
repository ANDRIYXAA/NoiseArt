// ============================================================================
// NoiseArt — CpuEffectNode (будь-який растровий Effect як нода)
// ============================================================================
// Читає вхідну текстуру → Effect::apply() на CPU → вихідна текстура. Завдяки
// цьому КОЖЕН зареєстрований растровий ефект стає нодою без окремого коду.
// Орієнтація зберігається (читаємо bottom-up FBO-текстуру, віддаємо так само).
// ============================================================================
#pragma once

#include "graph/Node.h"
#include "effects/Effect.h"
#include "core/Image.h"
#include <glad/glad.h>
#include <memory>
#include <string>

namespace NoiseArt {

class CpuEffectNode : public Node {
public:
    CpuEffectNode(int id, std::unique_ptr<Effect> effect)
        : Node(id), m_effect(std::move(effect)) {
        addInput("in",   SocketType::Texture);
        addOutput("out", SocketType::Texture);
    }

    std::string getTypeName() const override { return m_effect ? m_effect->getName() : "Effect"; }
    std::string getCategory() const override { return m_effect ? m_effect->getCategory() : "Effect"; }

    unsigned int evaluate(NodeEvalContext& ctx, const std::vector<unsigned int>& inputTex) override {
        unsigned int inTex = inputTex.empty() ? 0u : inputTex[0];
        if (inTex == 0 || !m_effect) return 0;

        const int W = ctx.width, H = ctx.height;
        Image img; img.create(W, H, 4);

        GLint prevTex = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTex);
        glBindTexture(GL_TEXTURE_2D, inTex);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.getData());
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(prevTex));

        Image out;
        m_effect->apply(img, out);
        m_outTex.update(out);
        return m_outTex.getID();
    }

    bool renderParamsUI() override { return m_effect ? m_effect->renderUI() : false; }

    std::unique_ptr<Node> clone() const override {
        auto n = std::make_unique<CpuEffectNode>(m_id, m_effect ? m_effect->clone() : nullptr);
        n->copyBaseFrom(*this);
        return n;
    }

    Effect* effect() const { return m_effect.get(); }

private:
    std::unique_ptr<Effect> m_effect;
};

} // namespace NoiseArt
