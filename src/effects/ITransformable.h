#pragma once

namespace NoiseArt {

// Інтерфейс для об'єктів, які можна перетягувати та масштабувати у Viewport
class ITransformable {
public:
    virtual ~ITransformable() = default;

    // Геометрія на полотні
    virtual float getX() const = 0;
    virtual float getY() const = 0;
    virtual float getWidth() const = 0;
    virtual float getHeight() const = 0;

    // Сітери для трансформації
    virtual void setPosition(float x, float y) = 0;
    virtual void setSize(float w, float h) = 0;
};

} // namespace NoiseArt
