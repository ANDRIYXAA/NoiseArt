// ============================================================================
// NoiseArt — NodeGraph (реалізація)
// ============================================================================
#include "graph/NodeGraph.h"
#include "graph/nodes/InputNode.h"
#include "graph/nodes/OutputNode.h"
#include <set>
#include <functional>

namespace NoiseArt {

void NodeGraph::assignSocketIds(Node& n) {
    for (auto& s : n.inputs())  { s.id = allocId(); s.nodeId = n.id(); }
    for (auto& s : n.outputs()) { s.id = allocId(); s.nodeId = n.id(); }
}

void NodeGraph::buildDefault() {
    m_nodes.clear();
    m_links.clear();
    m_nextId = 1; m_outputNodeId = -1; m_inputNodeId = -1;

    auto* in  = createNode<InputNode>();
    auto* out = createNode<OutputNode>();
    m_inputNodeId  = in->id();
    m_outputNodeId = out->id();
    in->m_editorX  =  40.0f; in->m_editorY  = 80.0f;
    out->m_editorX = 360.0f; out->m_editorY = 80.0f;

    connect(in->outputs()[0].id, out->inputs()[0].id);  // Input.in -> Output.result
}

void NodeGraph::copyFrom(const NodeGraph& o) {
    m_nodes.clear();
    m_links        = o.m_links;
    m_nextId       = o.m_nextId;
    m_outputNodeId = o.m_outputNodeId;
    m_inputNodeId  = o.m_inputNodeId;
    m_nodes.reserve(o.m_nodes.size());
    for (const auto& n : o.m_nodes) m_nodes.push_back(n->clone());  // clone зберігає id+сокети
}

Node* NodeGraph::findNode(int nodeId) {
    for (auto& n : m_nodes) if (n->id() == nodeId) return n.get();
    return nullptr;
}
const Node* NodeGraph::findNode(int nodeId) const {
    for (const auto& n : m_nodes) if (n->id() == nodeId) return n.get();
    return nullptr;
}

const Socket* NodeGraph::findSocket(int socketId) const {
    for (const auto& n : m_nodes) {
        for (const auto& s : n->inputs())  if (s.id == socketId) return &s;
        for (const auto& s : n->outputs()) if (s.id == socketId) return &s;
    }
    return nullptr;
}

int NodeGraph::socketOwnerNode(int socketId) const {
    const Socket* s = findSocket(socketId);
    return s ? s->nodeId : -1;
}

int NodeGraph::producerNodeForInput(int inputSocketId) const {
    for (const auto& l : m_links)
        if (l.toSocketId == inputSocketId) return socketOwnerNode(l.fromSocketId);
    return -1;
}

// Чи досяжна fromNode вниз по потоку з toNode? Якщо так — нове ребро дасть цикл.
bool NodeGraph::wouldCycle(int fromNode, int toNode) const {
    std::set<int> visited;
    std::function<bool(int)> dfs = [&](int n) -> bool {
        if (n == fromNode) return true;
        if (!visited.insert(n).second) return false;
        for (const auto& l : m_links)
            if (socketOwnerNode(l.fromSocketId) == n && dfs(socketOwnerNode(l.toSocketId)))
                return true;
        return false;
    };
    return dfs(toNode);
}

bool NodeGraph::connect(int fromSocketId, int toSocketId) {
    const Socket* from = findSocket(fromSocketId);
    const Socket* to   = findSocket(toSocketId);
    if (!from || !to)                   return false;
    if (from->isInput || !to->isInput)  return false;   // from = output, to = input
    if (from->type != to->type)         return false;   // типи мають збігатись
    if (from->nodeId == to->nodeId)     return false;   // без самопетлі
    if (wouldCycle(from->nodeId, to->nodeId)) return false;

    // один вхід — лише одне вхідне ребро (новий лінк заміщує старий)
    for (int i = static_cast<int>(m_links.size()) - 1; i >= 0; --i)
        if (m_links[i].toSocketId == toSocketId)
            m_links.erase(m_links.begin() + i);

    m_links.push_back({ allocId(), fromSocketId, toSocketId });
    return true;
}

void NodeGraph::disconnect(int linkId) {
    for (int i = static_cast<int>(m_links.size()) - 1; i >= 0; --i)
        if (m_links[i].id == linkId)
            m_links.erase(m_links.begin() + i);
}

void NodeGraph::removeNode(int nodeId) {
    if (nodeId == m_outputNodeId) return;   // Output не видаляється
    if (!findNode(nodeId))        return;

    auto belongs = [&](int socketId) { return socketOwnerNode(socketId) == nodeId; };
    for (int i = static_cast<int>(m_links.size()) - 1; i >= 0; --i)
        if (belongs(m_links[i].fromSocketId) || belongs(m_links[i].toSocketId))
            m_links.erase(m_links.begin() + i);

    for (int i = static_cast<int>(m_nodes.size()) - 1; i >= 0; --i)
        if (m_nodes[i]->id() == nodeId) { m_nodes.erase(m_nodes.begin() + i); break; }

    if (nodeId == m_inputNodeId) m_inputNodeId = -1;
}

unsigned int NodeGraph::evaluate(NodeEvalContext& ctx) {
    // Phase 1: заглушка. Справжній топологічний GPU-евал — у Phase 2.
    // Поки що прокидаємо вхідну текстуру шару (для генератора = 0 → нічого не малюється).
    return ctx.sourceTex;
}

} // namespace NoiseArt
