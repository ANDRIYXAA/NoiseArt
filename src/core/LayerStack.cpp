// ============================================================================
// NoiseArt — LayerStack (Реалізація)
// ============================================================================

#include "LayerStack.h"
#include <algorithm>
#include <iostream>

namespace NoiseArt {

// ============================================================================
// addLayer() — Додати шар зверху стека
// ============================================================================
void LayerStack::addLayer(std::unique_ptr<Layer> layer)
{
    m_layers.push_back(std::move(layer));
    m_selectedIndex = static_cast<int>(m_layers.size()) - 1;
    m_selectedLayer = m_layers.back().get();   // одразу вибираємо новий шар (для ручок трансформації)
    m_selectedIndices.clear();
    m_selectedIndices.insert(m_selectedIndex);
    m_dirty = true;
}

// ============================================================================
// insertLayer() — Вставити шар на позицію
// ============================================================================
void LayerStack::insertLayer(int index, std::unique_ptr<Layer> layer)
{
    if (index < 0) index = 0;
    if (index > static_cast<int>(m_layers.size())) {
        index = static_cast<int>(m_layers.size());
    }

    m_layers.insert(m_layers.begin() + index, std::move(layer));
    m_selectedIndex = index;
    m_selectedLayer = m_layers[index].get();
    m_dirty = true;
}

// ============================================================================
// removeLayer() — Видалити шар
// ============================================================================
void LayerStack::removeLayer(int index)
{
    if (index < 0 || index >= static_cast<int>(m_layers.size())) return;

    m_layers.erase(m_layers.begin() + index);

    // Коригуємо вибраний індекс
    if (m_selectedIndex >= static_cast<int>(m_layers.size())) {
        m_selectedIndex = static_cast<int>(m_layers.size()) - 1;
    }
    m_selectedLayer = nullptr;
    m_dirty = true;
}

// ============================================================================
// moveLayer() — Перемістити шар
// ============================================================================
void LayerStack::moveLayer(int fromIndex, int toIndex)
{
    if (fromIndex < 0 || fromIndex >= static_cast<int>(m_layers.size())) return;
    if (toIndex < 0 || toIndex >= static_cast<int>(m_layers.size())) return;
    if (fromIndex == toIndex) return;

    // Зберігаємо шар, видаляємо зі старої позиції, вставляємо на нову
    auto layer = std::move(m_layers[fromIndex]);
    m_layers.erase(m_layers.begin() + fromIndex);
    m_layers.insert(m_layers.begin() + toIndex, std::move(layer));

    m_selectedIndex = toIndex;
    m_dirty = true;
}

// ============================================================================
// duplicateLayer() — Дублювати шар
// ============================================================================
void LayerStack::duplicateLayer(int index)
{
    if (index < 0 || index >= static_cast<int>(m_layers.size())) return;

    auto copy = m_layers[index]->clone();
    copy->setName(copy->getName() + " (copy)");

    // Вставляємо копію одразу після оригіналу
    m_layers.insert(m_layers.begin() + index + 1, std::move(copy));
    m_selectedIndex = index + 1;
    m_selectedLayer = m_layers[index + 1].get();
    m_dirty = true;
}

void LayerStack::clear() {
    m_layers.clear();
    m_selectedIndex = -1;
    m_selectedLayer = nullptr;
    m_dirty = true;
}

// ============================================================================
// processAll() — Обробити зображення через ВСІ шари
// ============================================================================
// Алгоритм:
//   current = sourceImage
//   для кожного шару:
//     layer.process(current, next)
//     current = next
//   повернути current
// ============================================================================
Image LayerStack::processAll(const Image& sourceImage)
{
    if (m_layers.empty()) {
        return sourceImage.clone();
    }

    Image current = sourceImage.clone();

    for (auto& layer : m_layers) {
        Image next;
        layer->process(current, next);
        current = std::move(next);
    }

    m_dirty = false;
    return current;
}

// ============================================================================
// getLayer() — Доступ до шару за індексом
// ============================================================================
Layer* LayerStack::getLayer(int index)
{
    if (index < 0 || index >= static_cast<int>(m_layers.size())) return nullptr;
    return m_layers[index].get();
}

const Layer* LayerStack::getLayer(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_layers.size())) return nullptr;
    return m_layers[index].get();
}

// ============================================================================
// Multi-Select API
// ============================================================================

void LayerStack::addToSelection(int index) {
    if (index >= 0 && index < static_cast<int>(m_layers.size())) {
        m_selectedIndices.insert(index);
        m_selectedIndex = index;
    }
}

void LayerStack::removeFromSelection(int index) {
    m_selectedIndices.erase(index);
    if (m_selectedIndex == index) {
        m_selectedIndex = m_selectedIndices.empty() ? -1 : *m_selectedIndices.rbegin();
    }
}

void LayerStack::toggleSelection(int index) {
    if (isSelected(index)) {
        removeFromSelection(index);
    } else {
        addToSelection(index);
    }
}

void LayerStack::clearSelection() {
    m_selectedIndices.clear();
    m_selectedIndex = -1;
    m_selectedLayer = nullptr;
}

bool LayerStack::isSelected(int index) const {
    return m_selectedIndices.count(index) > 0;
}

// ============================================================================
// Вибір довільного вузла дерева
// ============================================================================

// Рекурсивний пошук вузла у піддереві
static bool subtreeContains(const Layer* node, const Layer* target) {
    if (!node) return false;
    if (node == target) return true;
    for (int i = 0; i < node->getChildCount(); ++i) {
        if (subtreeContains(node->getChild(i), target)) return true;
    }
    return false;
}

bool LayerStack::containsLayer(const Layer* target) const {
    if (!target) return false;
    for (const auto& l : m_layers) {
        if (subtreeContains(l.get(), target)) return true;
    }
    return false;
}

Layer* LayerStack::getSelectedLayer() {
    // Захист від dangling: якщо шар зник із дерева (undo, видалення) — скидаємо
    if (m_selectedLayer && !containsLayer(m_selectedLayer)) {
        m_selectedLayer = nullptr;
    }
    return m_selectedLayer;
}

int LayerStack::rootIndexOf(const Layer* layer) const {
    if (!layer) return -1;
    const Layer* root = layer;
    while (root->getParent()) root = root->getParent();
    for (int i = 0; i < static_cast<int>(m_layers.size()); ++i) {
        if (m_layers[i].get() == root) return i;
    }
    return -1;
}

// ============================================================================
// flattenTree() — Лінеаризація дерева шарів
// ============================================================================
std::vector<LayerStack::FlatEntry> LayerStack::flattenTree() const {
    std::vector<FlatEntry> result;
    for (int i = 0; i < static_cast<int>(m_layers.size()); ++i) {
        flattenRecursive(m_layers[i].get(), 0, i, -1, nullptr, result);
    }
    return result;
}

void LayerStack::flattenRecursive(Layer* layer, int depth, int rootIdx, int childIdx, Layer* parent, std::vector<FlatEntry>& out) const {
    if (!layer) return;
    out.push_back({ layer, depth, rootIdx, childIdx, parent });
    
    if (!layer->isCollapsed()) {
        for (int i = 0; i < layer->getChildCount(); ++i) {
            flattenRecursive(layer->getChild(i), depth + 1, rootIdx, i, layer, out);
        }
    }
}

// ============================================================================
// nestLayer() — Вкласти шар як дочірній
// ============================================================================
void LayerStack::nestLayer(int childIdx, int parentIdx) {
    if (childIdx < 0 || childIdx >= static_cast<int>(m_layers.size())) return;
    if (parentIdx < 0 || parentIdx >= static_cast<int>(m_layers.size())) return;
    if (childIdx == parentIdx) return;

    auto child = std::move(m_layers[childIdx]);
    m_layers.erase(m_layers.begin() + childIdx);
    
    // Коригуємо parentIdx якщо він був після childIdx
    int adjustedParent = (parentIdx > childIdx) ? parentIdx - 1 : parentIdx;
    
    if (adjustedParent >= 0 && adjustedParent < static_cast<int>(m_layers.size())) {
        m_layers[adjustedParent]->addChild(std::move(child));
    }
    
    m_selectedIndex = -1;
    m_selectedLayer = nullptr;
    m_dirty = true;
}

// ============================================================================
// unnestLayer() — Витягнути дочірній шар на root рівень
// ============================================================================
void LayerStack::unnestLayer(int parentIdx, int childIdx) {
    if (parentIdx < 0 || parentIdx >= static_cast<int>(m_layers.size())) return;
    
    auto child = m_layers[parentIdx]->removeChild(childIdx);
    if (child) {
        m_layers.insert(m_layers.begin() + parentIdx + 1, std::move(child));
        m_selectedIndex = parentIdx + 1;
        m_selectedLayer = nullptr;
        m_dirty = true;
    }
}

// ============================================================================
// Загальні операції з деревом (по вказівниках)
// ============================================================================
std::unique_ptr<Layer> LayerStack::detachLayer(Layer* target) {
    if (!target) return nullptr;
    Layer* parent = target->getParent();
    if (parent) {
        auto& kids = parent->getChildren();
        for (int i = 0; i < static_cast<int>(kids.size()); ++i) {
            if (kids[i].get() == target) return parent->removeChild(i);
        }
        return nullptr;
    }
    for (int i = 0; i < static_cast<int>(m_layers.size()); ++i) {
        if (m_layers[i].get() == target) {
            auto owned = std::move(m_layers[i]);
            m_layers.erase(m_layers.begin() + i);
            return owned;
        }
    }
    return nullptr;
}

void LayerStack::nestUnder(Layer* moving, Layer* newParent) {
    if (!moving || !newParent || moving == newParent) return;
    // Не можна вкласти у власного нащадка (інакше цикл у дереві)
    for (Layer* p = newParent; p; p = p->getParent()) {
        if (p == moving) return;
    }
    auto owned = detachLayer(moving);
    if (!owned) return;
    newParent->addChild(std::move(owned));  // встановлює parent
    m_selectedIndices.clear();
    m_selectedLayer = moving;
    m_selectedIndex = rootIndexOf(moving);
    if (m_selectedIndex >= 0) m_selectedIndices.insert(m_selectedIndex);
    m_dirty = true;
}

void LayerStack::moveToRoot(Layer* moving) {
    if (!moving || !moving->getParent()) return;
    auto owned = detachLayer(moving);
    if (!owned) return;
    m_layers.push_back(std::move(owned));
    m_selectedIndices.clear();
    m_selectedLayer = moving;
    m_selectedIndex = static_cast<int>(m_layers.size()) - 1;
    m_selectedIndices.insert(m_selectedIndex);
    m_dirty = true;
}

void LayerStack::moveLayerInParent(Layer* layer, int delta) {
    if (!layer || delta == 0) return;
    Layer* parent = layer->getParent();
    if (parent) {
        auto& kids = parent->getChildren();
        for (int i = 0; i < static_cast<int>(kids.size()); ++i) {
            if (kids[i].get() == layer) {
                int j = i + delta;
                if (j < 0 || j >= static_cast<int>(kids.size())) return;
                std::swap(kids[i], kids[j]);
                m_dirty = true;
                return;
            }
        }
    } else {
        for (int i = 0; i < static_cast<int>(m_layers.size()); ++i) {
            if (m_layers[i].get() == layer) {
                int j = i + delta;
                if (j < 0 || j >= static_cast<int>(m_layers.size())) return;
                std::swap(m_layers[i], m_layers[j]);
                m_selectedIndex = j;
                m_dirty = true;
                return;
            }
        }
    }
}

} // namespace NoiseArt
