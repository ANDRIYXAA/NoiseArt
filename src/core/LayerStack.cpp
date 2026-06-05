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

} // namespace NoiseArt
