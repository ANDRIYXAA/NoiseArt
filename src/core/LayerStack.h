// ============================================================================
// NoiseArt — LayerStack (Стек шарів)
// ============================================================================
// LayerStack керує УСІМА шарами як деревом:
//   Root
//     ├── Layer (Image)
//     ├── Layer (Group)
//     │     ├── Layer (Vector)
//     │     └── Layer (Text)
//     └── Layer (Shader)
//
// Підтримує multi-select та drag-and-drop reordering.
// ============================================================================

#pragma once

#include <vector>
#include <set>
#include <memory>
#include "Layer.h"
#include "Image.h"

namespace NoiseArt {

class LayerStack {
public:
    // ===== Керування шарами (root level) =====

    /// Додати шар зверху стека
    void addLayer(std::unique_ptr<Layer> layer);

    /// Вставити шар на позицію
    void insertLayer(int index, std::unique_ptr<Layer> layer);

    /// Видалити шар за індексом (root level)
    void removeLayer(int index);

    /// Перемістити шар з одної позиції на іншу
    void moveLayer(int fromIndex, int toIndex);

    /// Дублювати шар
    void duplicateLayer(int index);

    /// Очистити всі шари
    void clear();

    // ===== Обробка =====

    /// Головний метод: пропустити зображення через ВСІ шари
    Image processAll(const Image& sourceImage);

    // ===== Доступ (root level) =====

    Layer* getLayer(int index);
    const Layer* getLayer(int index) const;
    int getLayerCount() const { return static_cast<int>(m_layers.size()); }

    // ===== Вибір (Selection) =====

    /// Одиночний вибір (для сумісності)
    int getSelectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index) { m_selectedIndex = index; }

    // --- Multi-select ---
    const std::set<int>& getSelectedIndices() const { return m_selectedIndices; }
    
    /// Додати до виділення
    void addToSelection(int index);
    
    /// Прибрати з виділення
    void removeFromSelection(int index);
    
    /// Перемкнути виділення (Shift+Click)
    void toggleSelection(int index);
    
    /// Очистити multi-select
    void clearSelection();
    
    /// Чи є шар виділеним (у multi-select)?
    bool isSelected(int index) const;
    
    /// Кількість виділених
    int getSelectionCount() const { return static_cast<int>(m_selectedIndices.size()); }

    // ===== Tree operations =====

    /// Структура для flat-ітерації по дереву
    struct FlatEntry {
        Layer* layer;       // Вказівник на шар
        int depth;          // Глибина вкладеності (0 = root)
        int rootIndex;      // Індекс у root масиві
        int childIndex;     // Індекс серед дітей батька (-1 якщо root)
        Layer* parent;      // Батьківський шар (nullptr якщо root)
    };

    /// Лінеаризувати дерево для рендерингу/UI
    std::vector<FlatEntry> flattenTree() const;

    /// Вкласти шар як дочірній до іншого
    void nestLayer(int childIdx, int parentIdx);

    /// Витягнути шар з батьківського на root рівень
    void unnestLayer(int parentIdx, int childIdx);

    // ===== Dirty flag =====
    
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty = true) { m_dirty = dirty; }

private:
    /// Рекурсивна helper для flattenTree
    void flattenRecursive(Layer* layer, int depth, int rootIdx, int childIdx, Layer* parent, std::vector<FlatEntry>& out) const;

    std::vector<std::unique_ptr<Layer>> m_layers;
    int m_selectedIndex = -1;          // Поточний вибраний (одиночний)
    std::set<int> m_selectedIndices;   // Multi-select (root indices)
    bool m_dirty = true;
};

} // namespace NoiseArt
