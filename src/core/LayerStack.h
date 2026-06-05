// ============================================================================
// NoiseArt — LayerStack (Стек шарів)
// ============================================================================
// LayerStack керує УСІМА шарами і обробляє зображення через них послідовно:
//   Оригінал → [Шар 1] → [Шар 2] → ... → Результат
// ============================================================================

#pragma once

#include <vector>
#include <memory>
#include "Layer.h"
#include "Image.h"

namespace NoiseArt {

class LayerStack {
public:
    // ===== Керування шарами =====

    /// Додати шар зверху стека
    void addLayer(std::unique_ptr<Layer> layer);

    /// Вставити шар на позицію
    void insertLayer(int index, std::unique_ptr<Layer> layer);

    /// Видалити шар за індексом
    void removeLayer(int index);

    /// Перемістити шар з одної позиції на іншу
    void moveLayer(int fromIndex, int toIndex);

    /// Дублювати шар
    void duplicateLayer(int index);

    // ===== Обробка =====

    /// Головний метод: пропустити зображення через ВСІ шари
    Image processAll(const Image& sourceImage);

    // ===== Доступ =====

    Layer* getLayer(int index);
    const Layer* getLayer(int index) const;
    int getLayerCount() const { return static_cast<int>(m_layers.size()); }

    int getSelectedIndex() const { return m_selectedIndex; }
    void setSelectedIndex(int index) { m_selectedIndex = index; }

    /// Прапорець "щось змінилось" — для оптимізації
    bool isDirty() const { return m_dirty; }
    void setDirty(bool dirty = true) { m_dirty = dirty; }

private:
    std::vector<std::unique_ptr<Layer>> m_layers;
    int m_selectedIndex = -1;  // -1 = нічого не вибрано
    bool m_dirty = true;
};

} // namespace NoiseArt
