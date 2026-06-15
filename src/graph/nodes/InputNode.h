// ============================================================================
// NoiseArt — InputNode (вхід графа)
// ============================================================================
// Видає вхідну текстуру шару (ctx.sourceTex). Для шару-генератора це 0.
// ============================================================================
#pragma once

#include "graph/Node.h"

namespace NoiseArt {

class InputNode : public Node {
public:
    explicit InputNode(int id) : Node(id) { addOutput("in", SocketType::Texture); }

    std::string getTypeName() const override { return "Input"; }
    std::string getCategory() const override { return "Input"; }

    unsigned int evaluate(NodeEvalContext& ctx, const std::vector<unsigned int>&) override {
        return ctx.sourceTex;
    }

    std::unique_ptr<Node> clone() const override {
        auto n = std::make_unique<InputNode>(m_id);
        n->copyBaseFrom(*this);
        return n;
    }
};

} // namespace NoiseArt
