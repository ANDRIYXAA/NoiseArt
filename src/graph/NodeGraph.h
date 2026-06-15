// ============================================================================
// NoiseArt — NodeGraph (граф нод)
// ============================================================================
// Володіє нодами й лінками, призначає унікальні id, перевіряє типи/цикли при
// з'єднанні, і обчислює результат у текстуру. Глибоко копіюється (для clone
// шару). Phase 1: evaluate() — заглушка; справжній топо-евал додає Phase 2.
// ============================================================================
#pragma once

#include <vector>
#include <memory>
#include <utility>
#include "graph/Node.h"

namespace NoiseArt {

class NodeGraph {
public:
    NodeGraph()  { buildDefault(); }
    ~NodeGraph() = default;

    // Глибоке копіювання (ноди клонуються, лінки копіюються)
    NodeGraph(const NodeGraph& o)            { copyFrom(o); }
    NodeGraph& operator=(const NodeGraph& o) { if (this != &o) copyFrom(o); return *this; }
    NodeGraph(NodeGraph&&)            = default;
    NodeGraph& operator=(NodeGraph&&) = default;

    // Створює ноду типу T, призначає id ноді+сокетам, додає у граф. Повертає вказівник.
    template<typename T, typename... Args>
    T* createNode(Args&&... args) {
        auto node = std::make_unique<T>(allocId(), std::forward<Args>(args)...);
        T* ptr = node.get();
        assignSocketIds(*node);
        m_nodes.push_back(std::move(node));
        return ptr;
    }

    void removeNode(int nodeId);                     // не видаляє Output
    bool connect(int fromSocketId, int toSocketId);  // перевірка типів+циклів; 1 ребро на вхід
    void disconnect(int linkId);

    // --- Доступ ---
    const std::vector<std::unique_ptr<Node>>& nodes() const { return m_nodes; }
    const std::vector<Link>&                  links() const { return m_links; }
    int  outputNodeId() const { return m_outputNodeId; }
    int  inputNodeId()  const { return m_inputNodeId; }

    Node*         findNode(int nodeId);
    const Node*   findNode(int nodeId) const;
    const Socket* findSocket(int socketId) const;
    int  socketOwnerNode(int socketId) const;
    int  producerNodeForInput(int inputSocketId) const;  // nodeId продюсера або -1

    // --- Обчислення (Phase 1: заглушка) ---
    unsigned int evaluate(NodeEvalContext& ctx);

private:
    int  allocId() { return m_nextId++; }
    void assignSocketIds(Node& n);
    void buildDefault();
    void copyFrom(const NodeGraph& o);
    bool wouldCycle(int fromNode, int toNode) const;

    std::vector<std::unique_ptr<Node>> m_nodes;
    std::vector<Link>                  m_links;
    int m_nextId       = 1;
    int m_outputNodeId = -1;
    int m_inputNodeId  = -1;

    // Граф-рівневий кеш результату (транзієнтний — НЕ копіюється у copyFrom)
    unsigned int m_lastResult   = 0;
    bool         m_haveResult   = false;
    uint64_t     m_lastGen      = 0;
    int          m_lastRes      = 0;
    float        m_lastEvalTime = -1.0f;
};

} // namespace NoiseArt
