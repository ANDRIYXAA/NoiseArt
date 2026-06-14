#include "History.h"

namespace NoiseArt {

void History::push(std::unique_ptr<Command> cmd) {
    m_undoStack.push_back(std::move(cmd));
    m_redoStack.clear(); // Кожна нова дія знищує гілку Redo
    
    if (m_undoStack.size() > MAX_HISTORY) {
        m_undoStack.erase(m_undoStack.begin());
    }
}

void History::undo(LayerStack& stack) {
    if (m_undoStack.empty()) return;
    
    auto cmd = std::move(m_undoStack.back());
    m_undoStack.pop_back();
    
    cmd->undo(stack);
    stack.setDirty(true);
    
    m_redoStack.push_back(std::move(cmd));
}

void History::redo(LayerStack& stack) {
    if (m_redoStack.empty()) return;
    
    auto cmd = std::move(m_redoStack.back());
    m_redoStack.pop_back();
    
    cmd->execute(stack);
    stack.setDirty(true);
    
    m_undoStack.push_back(std::move(cmd));
}

void History::clear() {
    m_undoStack.clear();
    m_redoStack.clear();
}

// ============================================================================

AddLayerCommand::AddLayerCommand(std::unique_ptr<Layer> layer, int index)
    : m_layer(std::move(layer)), m_index(index) {}

void AddLayerCommand::execute(LayerStack& stack) {
    // Якщо індекс -1, це означає "додати наверх"
    // Але оскільки шар міг бути доданий в середину, ми маємо зберігати індекс при першому виклику
    if (m_index == -1) {
        m_index = stack.getLayerCount();
    }
    
    if (m_layer) {
        stack.insertLayer(m_index, std::move(m_layer));
    }
}

void AddLayerCommand::undo(LayerStack& stack) {
    if (m_index >= 0 && m_index < stack.getLayerCount()) {
        // Забираємо шар назад у команду
        m_layer = stack.getLayer(m_index)->clone(); // Простіше клонувати і видалити оригінал
        stack.removeLayer(m_index);
        
        // Щоб зберегти оригінальні вказівники та ресурси без клонування, 
        // LayerStack::removeLayer має повертати std::unique_ptr.
        // Оскільки він зараз повертає void, ми поки зробимо clone(),
        // але оскільки OverlayData тепер shared_ptr, clone() працює миттєво!
    }
}

// ============================================================================

RemoveLayerCommand::RemoveLayerCommand(int index, std::unique_ptr<Layer> layer)
    : m_index(index), m_layer(std::move(layer)) {}

void RemoveLayerCommand::execute(LayerStack& stack) {
    if (m_index >= 0 && m_index < stack.getLayerCount()) {
        m_layer = stack.getLayer(m_index)->clone();
        stack.removeLayer(m_index);
    }
}

void RemoveLayerCommand::undo(LayerStack& stack) {
    if (m_layer) {
        stack.insertLayer(m_index, std::move(m_layer));
    }
}

// ============================================================================

MoveLayerCommand::MoveLayerCommand(int fromIdx, int toIdx)
    : m_from(fromIdx), m_to(toIdx) {}

void MoveLayerCommand::execute(LayerStack& stack) {
    stack.moveLayer(m_from, m_to);
}

void MoveLayerCommand::undo(LayerStack& stack) {
    stack.moveLayer(m_to, m_from);
}

// ============================================================================

ChangeLayerCommand::ChangeLayerCommand(int index, std::unique_ptr<Layer> stateBefore, std::unique_ptr<Layer> stateAfter)
    : m_index(index), m_stateBefore(std::move(stateBefore)), m_stateAfter(std::move(stateAfter)) {}

void ChangeLayerCommand::execute(LayerStack& stack) {
    if (m_index >= 0 && m_index < stack.getLayerCount() && m_stateAfter) {
        stack.removeLayer(m_index);
        stack.insertLayer(m_index, m_stateAfter->clone());
    }
}

void ChangeLayerCommand::undo(LayerStack& stack) {
    if (m_index >= 0 && m_index < stack.getLayerCount() && m_stateBefore) {
        stack.removeLayer(m_index);
        stack.insertLayer(m_index, m_stateBefore->clone());
    }
}

} // namespace NoiseArt
