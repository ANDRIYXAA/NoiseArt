// ============================================================================
// NoiseArt — NodeEditorPanel (візуальний нодовий редактор, imgui-node-editor)
// ============================================================================
// Редагує граф вибраного шару "Node Graph": перетягування пінів для з'єднання,
// права кнопка → палітра нод (шейдери + кожен растровий ефект), параметри в тілі
// ноди, видалення лінків/нод (Output не видаляється).
// ============================================================================
#pragma once

#include <set>
#include <vector>
#include <string>

namespace ax { namespace NodeEditor { struct EditorContext; } }

namespace NoiseArt {

class LayerStack;
class EffectRegistry;
class NodeGraph;

class NodeEditorPanel {
public:
    ~NodeEditorPanel();
    void shutdown();   // звільнити контекст редактора (поки GL/ImGui ще живі)

    bool isOpen = true;

    /// Малює редактор для вибраного шару. Повертає true, якщо граф змінено.
    bool render(LayerStack& stack, EffectRegistry& registry);

private:
    ax::NodeEditor::EditorContext* m_ctx = nullptr;
    const NodeGraph*               m_lastGraph = nullptr;  // визначення зміни вибраного графа
    std::set<int>                  m_positioned;           // ноди, вже спозиціоновані в редакторі
    int                            m_navigateFrames = 0;   // авто-центрування viewport (кілька кадрів після відкриття)
    std::vector<std::string>       m_rasterNames;          // кеш імен растрових ефектів для палітри
};

} // namespace NoiseArt
