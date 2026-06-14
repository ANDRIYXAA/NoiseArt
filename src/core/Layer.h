// ============================================================================
// NoiseArt — Клас Layer (Шар)
// ============================================================================
// Один шар = ефект + метадані + дочірні шари.
//
// Шар може містити дочірні шари (tree structure):
//   Layer (Group)
//     ├── Layer (Image)
//     ├── Layer (Vector)
//     └── Layer (Text)
//
// Дочірні шари наслідують батьківські трансформації (позиція).
// ============================================================================

#pragma once

#include <string>
#include <memory>
#include <vector>
#include "effects/Effect.h"
#include "core/Image.h"

namespace NoiseArt {

// ============================================================================
// BlendMode — режими змішування
// ============================================================================
enum class BlendMode {
    Normal,     // Просто замінює (з урахуванням opacity)
    Add,        // Додавання — робить яскравіше
    Multiply,   // Множення — робить темніше
    Screen,     // Екран — м'яке освітлення
    Overlay     // Накладення — збільшує контраст
};

// Масив назв для відображення в UI
inline const char* BlendModeNames[] = {
    "Normal", "Add", "Multiply", "Screen", "Overlay"
};
inline constexpr int BlendModeCount = 5;

// ============================================================================
// Клас Layer
// ============================================================================
class Layer {
public:
    /// Конструктор: створює шар з ефектом та необов'язковою назвою
    Layer(std::unique_ptr<Effect> effect, const std::string& name = "");

    /// Застосувати ефект з урахуванням opacity та blend mode
    void process(const Image& input, Image& output);

    // ----- Властивості -----
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    Effect* getEffect() { return m_effect.get(); }
    const Effect* getEffect() const { return m_effect.get(); }

    float getOpacity() const { return m_opacity; }
    void setOpacity(float opacity) { m_opacity = (std::max)(0.0f, (std::min)(1.0f, opacity)); }

    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    BlendMode getBlendMode() const { return m_blendMode; }
    void setBlendMode(BlendMode mode) { m_blendMode = mode; }

    // ----- Дочірні шари (Child Layers) -----

    /// Додати дочірній шар
    void addChild(std::unique_ptr<Layer> child);

    /// Вставити дочірній шар на позицію
    void insertChild(int index, std::unique_ptr<Layer> child);

    /// Видалити дочірній шар (повертає його для переміщення)
    std::unique_ptr<Layer> removeChild(int index);

    /// Доступ до дочірнього шару
    Layer* getChild(int index);
    const Layer* getChild(int index) const;
    int getChildCount() const { return static_cast<int>(m_children.size()); }
    std::vector<std::unique_ptr<Layer>>& getChildren() { return m_children; }
    const std::vector<std::unique_ptr<Layer>>& getChildren() const { return m_children; }
    bool hasChildren() const { return !m_children.empty(); }

    /// Батьківський шар (weak reference, не володіє)
    Layer* getParent() const { return m_parent; }
    void setParent(Layer* parent) { m_parent = parent; }

    /// Згорнутий/розгорнутий стан у UI дереві
    bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool c) { m_collapsed = c; }

    /// Дублювання шару (з копією ефекту та дітей)
    std::unique_ptr<Layer> clone() const;

private:
    /// Змішати два пікселі відповідно до blend mode та opacity
    void blendPixel(const uint8_t* base, const uint8_t* effect,
                    uint8_t* result, float opacity, BlendMode mode);

    std::string m_name;
    std::unique_ptr<Effect> m_effect;
    float m_opacity = 1.0f;
    bool m_enabled = true;
    BlendMode m_blendMode = BlendMode::Normal;

    // --- Child layers ---
    std::vector<std::unique_ptr<Layer>> m_children;
    Layer* m_parent = nullptr;   // weak ref, не видаляти!
    bool m_collapsed = false;
};

} // namespace NoiseArt
