// ============================================================================
// NoiseArt — NodeGraph (реалізація)
// ============================================================================
#include "graph/NodeGraph.h"
#include "graph/nodes/InputNode.h"
#include "graph/nodes/OutputNode.h"
#include "graph/nodes/ShaderNode.h"
#include <set>
#include <functional>
#include <cstddef>
#include <cstdint>

namespace NoiseArt {

static inline void hashMix(std::size_t& h, std::size_t v) {
    h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
}

void NodeGraph::assignSocketIds(Node& n) {
    for (auto& s : n.inputs())  { s.id = allocId(); s.nodeId = n.id(); }
    for (auto& s : n.outputs()) { s.id = allocId(); s.nodeId = n.id(); }
}

void NodeGraph::buildDefault() {
    m_nodes.clear();
    m_links.clear();
    m_nextId = 1; m_outputNodeId = -1; m_inputNodeId = -1;

    // Типовий граф: процедурний шейдер (Plasma) → Output, щоб шар одразу щось малював.
    auto* sh  = createNode<ShaderNode>(ShaderNode::Kind::Plasma);
    auto* out = createNode<OutputNode>();
    m_outputNodeId = out->id();
    sh->m_editorX  =  40.0f; sh->m_editorY  = 80.0f;
    out->m_editorX = 360.0f; out->m_editorY = 80.0f;

    connect(sh->outputs()[0].id, out->inputs()[0].id);  // Shader.out -> Output.result
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

bool NodeGraph::canConnect(int fromSocketId, int toSocketId) const {
    const Socket* from = findSocket(fromSocketId);
    const Socket* to   = findSocket(toSocketId);
    if (!from || !to)                  return false;
    if (from->isInput || !to->isInput) return false;
    if (from->type != to->type)        return false;
    if (from->nodeId == to->nodeId)    return false;
    if (wouldCycle(from->nodeId, to->nodeId)) return false;
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
    if (m_outputNodeId < 0 || !findNode(m_outputNodeId)) return 0;

    bool animated = false;
    for (const auto& n : m_nodes) if (n->isAnimated()) { animated = true; break; }

    // Граф-рівневий кеш/тротлінг (аналог ShaderLayerEffect, але для всього графа).
    if (m_haveResult && ctx.cache && ctx.editGen == m_lastGen && ctx.width == m_lastRes) {
        if (!animated) return m_lastResult;                       // нічого не змінилось
        if (ctx.throttle && m_lastEvalTime >= 0.0f &&
            (ctx.time - m_lastEvalTime) < ctx.throttleInterval)
            return m_lastResult;                                  // анімація, але оновлювати зарано
    }

    // Топологічний прохід з ПОНОДНИМ кешем: перераховуємо лише ноди, чий ключ змінився
    // (їх параметри або входи) + анімовані. Решта віддають кешовану текстуру.
    std::set<int> inProgress;
    std::set<int> done;
    std::function<unsigned int(int)> eval = [&](int nid) -> unsigned int {
        Node* n = findNode(nid);
        if (!n) return 0;
        if (done.count(nid)) return n->m_cacheTex;
        if (!inProgress.insert(nid).second) return 0;   // захист від циклу

        std::vector<unsigned int> inTex;
        inTex.reserve(n->inputs().size());

        // Ключ ноди = параметри + RES + ключі вхідних нод (зміни течуть униз потоком).
        std::size_t key = n->paramHash();
        hashMix(key, static_cast<std::size_t>(ctx.width));
        for (const auto& s : n->inputs()) {
            int prod = producerNodeForInput(s.id);
            if (prod >= 0) {
                inTex.push_back(eval(prod));
                Node* pn = findNode(prod);
                hashMix(key, pn ? pn->m_cacheKey : 0u);
            } else {
                inTex.push_back(0u);          // вхід не під'єднано
                hashMix(key, 0x9E37u);
            }
        }
        inProgress.erase(nid);

        const bool nodeAnimated = n->isAnimated();
        if (!n->cacheKeyComplete())
            hashMix(key, std::hash<uint64_t>{}(ctx.editGen));  // консервативно: інвалідувати на будь-яку правку
        if (nodeAnimated)
            hashMix(key, std::hash<long long>{}(static_cast<long long>(ctx.time * 1000.0f)));

        if (ctx.cache && !nodeAnimated && n->m_cacheValid && n->m_cacheKey == key) {
            done.insert(nid);
            return n->m_cacheTex;             // понодний кеш-хіт — пропускаємо обчислення
        }

        unsigned int tex = n->evaluate(ctx, inTex);
        n->m_cacheKey   = key;
        n->m_cacheValid = true;
        n->m_cacheTex   = tex;
        done.insert(nid);
        return tex;
    };
    unsigned int result = eval(m_outputNodeId);

    m_lastResult   = result;
    m_haveResult   = true;
    m_lastGen      = ctx.editGen;
    m_lastRes      = ctx.width;
    m_lastEvalTime = ctx.time;
    return result;
}

} // namespace NoiseArt
