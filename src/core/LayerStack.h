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
    void setSelectedIndex(int index) { m_selectedIndex = index; m_selectedLayer = getLayer(index); }

    // --- Вибір довільного вузла дерева (працює і для дочірніх шарів) ---
    /// Повертає вибраний вузол; самоочищується, якщо вказівник застарів (після undo/видалення)
    Layer* getSelectedLayer();
    void setSelectedLayer(Layer* layer) { m_selectedLayer = layer; }
    /// Чи існує цей шар десь у дереві (валідація вказівника)
    bool containsLayer(const Layer* target) const;
    /// Індекс кореневого предка шару у root-масиві (-1 якщо не знайдено)
    int rootIndexOf(const Layer* layer) const;

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

    // --- Загальні операції з деревом (по вказівниках, для будь-якого вузла) ---
    /// Зробити `moving` дитиною `newParent` (з будь-якого місця; із захистом від циклу)
    void nestUnder(Layer* moving, Layer* newParent);
    /// Перемістити `moving` на кореневий рівень
    void moveToRoot(Layer* moving);
    /// Переставити шар серед сусідів (delta -1 / +1 у масиві контейнера)
    void moveLayerInParent(Layer* layer, int delta);

    // ===== Dirty flag =====
    
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty = true) { m_dirty = dirty; }

private:
    /// Рекурсивна helper для flattenTree
    void flattenRecursive(Layer* layer, int depth, int rootIdx, int childIdx, Layer* parent, std::vector<FlatEntry>& out) const;

    /// Від'єднати шар від контейнера (root або батько), повертаючи володіння
    std::unique_ptr<Layer> detachLayer(Layer* target);

    std::vector<std::unique_ptr<Layer>> m_layers;
    int m_selectedIndex = -1;          // Вибраний root (для root-операцій та історії)
    Layer* m_selectedLayer = nullptr;  // Вибраний вузол будь-якого рівня (для редагування)
    std::set<int> m_selectedIndices;   // Multi-select (root indices)
    bool m_dirty = true;
};

} // namespace NoiseArt
