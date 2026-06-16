// ============================================================================
// NoiseArt — NodeEditorPanel (реалізація, imgui-node-editor)
// ============================================================================
// ВАЖЛИВО: ImGui-віджети з попапами (Combo, ColorEdit) НЕ працюють усередині
// ed::BeginNode/EndNode — канва редактора застосовує трансформацію пан/зум, і
// попапи (екранні координати) зміщуються/не клікаються. Тому тіло ноди = лише
// заголовок + піни, а параметри ВИБРАНОЇ ноди редагуються у смузі ВНИЗУ вікна,
// поза канвою.
// ============================================================================
#include "ui/panels/NodeEditorPanel.h"

#include "core/LayerStack.h"
#include "effects/EffectRegistry.h"
#include "effects/NodeGraphEffect.h"
#include "graph/NodeGraph.h"
#include "graph/nodes/InputNode.h"
#include "graph/nodes/ShaderNode.h"
#include "graph/nodes/CpuEffectNode.h"

#include <imgui.h>
#include <imgui-node-editor/imgui_node_editor.h>
#include <portable-file-dialogs.h>
#include <nlohmann/json.hpp>
#include <fstream>

namespace ed = ax::NodeEditor;

namespace NoiseArt {

NodeEditorPanel::~NodeEditorPanel() { shutdown(); }

void NodeEditorPanel::shutdown() {
    if (m_ctx) { ed::DestroyEditor(m_ctx); m_ctx = nullptr; }
}

bool NodeEditorPanel::render(LayerStack& stack, EffectRegistry& registry) {
    if (!isOpen) return false;

    // Кеш імен растрових ефектів (раз): растровий фільтр = без трансформації, не вектор/шейдер.
    if (m_rasterNames.empty()) {
        for (const auto& info : registry.getAll()) {
            auto e = registry.createEffect(info.name);
            if (e && !e->isVector() && !e->isShader() && e->getTransformable() == nullptr)
                m_rasterNames.push_back(info.name);
        }
    }

    bool dirty = false;

    if (ImGui::Begin("Node Editor", &isOpen)) {
        Layer*  sel = stack.getSelectedLayer();
        Effect* eff = sel ? sel->getEffect() : nullptr;
        NodeGraphEffect* nge = dynamic_cast<NodeGraphEffect*>(eff);

        if (!nge) {
            ImGui::TextDisabled("Виберіть шар \"Node Graph\" у дереві шарів, щоб редагувати його граф.");
        } else {
            // Тулбар: збереження / завантаження графа (поза канвою — діалоги працюють нормально)
            if (ImGui::Button("Save Graph...")) {
                auto path = pfd::save_file("Save Node Graph", "graph.json", { "JSON", "*.json" }).result();
                if (!path.empty()) {
                    std::ofstream f(path);
                    if (f) f << nge->graph().toJson().dump(2);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Load Graph...")) {
                auto res = pfd::open_file("Load Node Graph", "", { "JSON", "*.json" }).result();
                if (!res.empty()) {
                    std::ifstream f(res[0]);
                    if (f) {
                        try {
                            nlohmann::json jj; f >> jj;
                            if (nge->graph().fromJson(jj, registry)) {
                                m_positioned.clear();   // нові id нод
                                m_navigateFrames = 2;
                                dirty = true;
                            }
                        } catch (...) { /* ігноруємо биті файли */ }
                    }
                }
            }

            if (!m_ctx) {
                ed::Config cfg;
                cfg.SettingsFile = nullptr;   // позиції зберігаємо самі (у нодах)
                m_ctx = ed::CreateEditor(&cfg);
            }

            NodeGraph& g = nge->graph();
            if (m_lastGraph != &g) {          // зміна вибраного графа → перепозиціонувати + відцентрувати
                m_positioned.clear();
                m_lastGraph = &g;
                m_navigateFrames = 2;
            }

            ed::SetCurrentEditor(m_ctx);

            // Канва лишає смугу параметрів унизу.
            const float stripH = 190.0f;
            ImVec2 avail = ImGui::GetContentRegionAvail();
            float canvasH = avail.y - stripH;
            if (canvasH < 80.0f) canvasH = 80.0f;
            ed::Begin("graph_canvas", ImVec2(avail.x, canvasH));

            // ===== Ноди (лише заголовок + піни; параметри — у смузі внизу) =====
            for (const auto& up : g.nodes()) {
                Node* n = up.get();
                if (!m_positioned.count(n->id())) {
                    ed::SetNodePosition(ed::NodeId(n->id()), ImVec2(n->m_editorX, n->m_editorY));
                    m_positioned.insert(n->id());
                }
                ed::BeginNode(ed::NodeId(n->id()));
                ImGui::PushID(n->id());
                ImGui::TextUnformatted(n->getTypeName().c_str());
                for (const auto& s : n->inputs()) {
                    ed::BeginPin(ed::PinId(s.id), ed::PinKind::Input);
                    ImGui::Text("-> %s", s.name.c_str());
                    ed::EndPin();
                }
                for (const auto& s : n->outputs()) {
                    ed::BeginPin(ed::PinId(s.id), ed::PinKind::Output);
                    ImGui::Text("%s ->", s.name.c_str());
                    ed::EndPin();
                }
                ImGui::PopID();
                ed::EndNode();

                ImVec2 p = ed::GetNodePosition(ed::NodeId(n->id()));
                n->m_editorX = p.x;
                n->m_editorY = p.y;
            }

            // ===== Лінки =====
            for (const auto& l : g.links())
                ed::Link(ed::LinkId(l.id), ed::PinId(l.fromSocketId), ed::PinId(l.toSocketId));

            // ===== Створення лінків =====
            if (ed::BeginCreate()) {
                ed::PinId aPin, bPin;
                if (ed::QueryNewLink(&aPin, &bPin) && aPin && bPin) {
                    const int a = static_cast<int>(aPin.Get());
                    const int b = static_cast<int>(bPin.Get());
                    const Socket* sa = g.findSocket(a);
                    const Socket* sb = g.findSocket(b);
                    const int outId = (sa && sa->isInput) ? b : a;
                    const int inId  = (sa && sa->isInput) ? a : b;
                    const bool valid = sa && sb && (sa->isInput != sb->isInput) && g.canConnect(outId, inId);
                    if (valid) {
                        if (ed::AcceptNewItem()) { if (g.connect(outId, inId)) dirty = true; }
                    } else {
                        ed::RejectNewItem();
                    }
                }
            }
            ed::EndCreate();

            // ===== Видалення =====
            if (ed::BeginDelete()) {
                ed::LinkId lid;
                while (ed::QueryDeletedLink(&lid)) {
                    if (ed::AcceptDeletedItem()) { g.disconnect(static_cast<int>(lid.Get())); dirty = true; }
                }
                ed::NodeId nid;
                while (ed::QueryDeletedNode(&nid)) {
                    const int id = static_cast<int>(nid.Get());
                    if (id == g.outputNodeId()) { ed::RejectDeletedItem(); continue; }  // Output не видаляється
                    if (ed::AcceptDeletedItem()) { g.removeNode(id); m_positioned.erase(id); dirty = true; }
                }
            }
            ed::EndDelete();

            // ===== Палітра (права кнопка по фону) =====
            auto placeNew = [&](Node* nn) {
                const float k = static_cast<float>(g.nodes().size());
                nn->m_editorX = 80.0f + k * 6.0f;
                nn->m_editorY = 220.0f + k * 6.0f;
            };
            ed::Suspend();
            if (ed::ShowBackgroundContextMenu()) ImGui::OpenPopup("AddNodePopup");
            if (ImGui::BeginPopup("AddNodePopup")) {
                if (ImGui::MenuItem("Shader: Plasma")) { placeNew(g.createNode<ShaderNode>(ShaderNode::Kind::Plasma)); dirty = true; }
                if (ImGui::MenuItem("Shader: Perlin")) { placeNew(g.createNode<ShaderNode>(ShaderNode::Kind::Perlin)); dirty = true; }
                ImGui::Separator();
                if (ImGui::MenuItem("Input"))          { placeNew(g.createNode<InputNode>()); dirty = true; }
                ImGui::Separator();
                if (ImGui::BeginMenu("Effect")) {
                    for (const auto& name : m_rasterNames) {
                        if (ImGui::MenuItem(name.c_str())) {
                            auto e = registry.createEffect(name);
                            if (e) { placeNew(g.createNode<CpuEffectNode>(std::move(e))); dirty = true; }
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndPopup();
            }
            ed::Resume();

            ed::End();

            // Авто-центрування одразу після відкриття/зміни графа.
            if (m_navigateFrames > 0) { ed::NavigateToContent(0.0f); --m_navigateFrames; }

            // Вибрана нода (поки редактор ще активний) — для смуги параметрів.
            ed::NodeId selBuf[8];
            const int selCount = ed::GetSelectedNodes(selBuf, 8);
            ed::SetCurrentEditor(nullptr);

            // ===== Смуга параметрів вибраної ноди (ПОЗА канвою — попапи коректні) =====
            ImGui::Separator();
            Node* selNode = (selCount > 0) ? g.findNode(static_cast<int>(selBuf[0].Get())) : nullptr;
            ImGui::BeginChild("nodeparams", ImVec2(0, 0), true);
            if (selNode) {
                ImGui::Text("Параметри: %s", selNode->getTypeName().c_str());
                ImGui::Separator();
                ImGui::PushID(selNode->id());
                ImGui::PushItemWidth(200.0f);
                if (selNode->renderParamsUI()) dirty = true;
                ImGui::PopItemWidth();
                ImGui::PopID();
            } else {
                ImGui::TextDisabled("Виберіть ноду, щоб редагувати її параметри.");
                ImGui::TextDisabled("ПКМ по фону — додати ноду.");
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();
    return dirty;
}

} // namespace NoiseArt
