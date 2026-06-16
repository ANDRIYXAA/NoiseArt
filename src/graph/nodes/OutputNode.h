// ============================================================================
// NoiseArt — OutputNode (вихід графа)
// ============================================================================
// Результат графа = текстура, під'єднана до входу "result".
// Рівно одна на граф; видаленню не підлягає.
// ============================================================================
#pragma once

#include "graph/Node.h"

namespace NoiseArt {

class OutputNode : public Node {
public:
    explicit OutputNode(int id) : Node(id) { addInput("result", SocketType::Texture); }

    std::string getTypeName() const override { return "Output"; }
    std::string getCategory() const override { return "Output"; }

    bool cacheKeyComplete() const override { return true; }   // лише прокидання входу

    unsigned int evaluate(NodeEvalContext&, const std::vector<unsigned int>& inputTex) override {
        return inputTex.empty() ? 0u : inputTex[0];
    }

    std::unique_ptr<Node> clone() const override {
        auto n = std::make_unique<OutputNode>(m_id);
        n->copyBaseFrom(*this);
        return n;
    }
};

} // namespace NoiseArt
