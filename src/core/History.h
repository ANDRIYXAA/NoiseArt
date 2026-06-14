// ============================================================================
// NoiseArt — History System (Undo / Redo)
// ============================================================================

#pragma once

#include <vector>
#include <memory>
#include <string>
#include "LayerStack.h"

namespace NoiseArt {

// ============================================================================
// Base Command Interface
// ============================================================================
class Command {
public:
    virtual ~Command() = default;
    
    /// Виконується при Redo (або одразу після додавання, якщо команда ще не застосована)
    virtual void execute(LayerStack& stack) = 0;
    
    /// Виконується при Undo
    virtual void undo(LayerStack& stack) = 0;
};

// ============================================================================
// History Manager
// ============================================================================
class History {
public:
    /// Додає нову команду, автоматично очищуючи стек redo.
    /// Команда ВЖЕ МАЄ БУТИ ВИКОНАНА до того, як її передадуть сюди.
    void push(std::unique_ptr<Command> cmd);

    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }

    void undo(LayerStack& stack);
    void redo(LayerStack& stack);
    
    void clear();

private:
    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
    
    const size_t MAX_HISTORY = 50;
};

// ============================================================================
// Concrete Commands
// ============================================================================

class AddLayerCommand : public Command {
public:
    AddLayerCommand(std::unique_ptr<Layer> layer, int index = -1);
    
    void execute(LayerStack& stack) override;
    void undo(LayerStack& stack) override;

private:
    std::unique_ptr<Layer> m_layer;
    int m_index;
};

class RemoveLayerCommand : public Command {
public:
    RemoveLayerCommand(int index, std::unique_ptr<Layer> layer);
    
    void execute(LayerStack& stack) override;
    void undo(LayerStack& stack) override;

private:
    std::unique_ptr<Layer> m_layer;
    int m_index;
};

class MoveLayerCommand : public Command {
public:
    MoveLayerCommand(int fromIdx, int toIdx);
    
    void execute(LayerStack& stack) override;
    void undo(LayerStack& stack) override;

private:
    int m_from;
    int m_to;
};

class ChangeLayerCommand : public Command {
public:
    // Зберігає "знімок" шару до і після
    ChangeLayerCommand(int index, std::unique_ptr<Layer> stateBefore, std::unique_ptr<Layer> stateAfter);
    
    void execute(LayerStack& stack) override;
    void undo(LayerStack& stack) override;

private:
    int m_index;
    std::unique_ptr<Layer> m_stateBefore;
    std::unique_ptr<Layer> m_stateAfter;
};

} // namespace NoiseArt
