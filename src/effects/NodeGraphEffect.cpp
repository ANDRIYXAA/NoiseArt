// ============================================================================
// NoiseArt — NodeGraphEffect (реалізація)
// ============================================================================
#include "effects/NodeGraphEffect.h"
#include <imgui.h>

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

} // namespace NoiseArt
