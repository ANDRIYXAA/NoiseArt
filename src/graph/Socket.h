// ============================================================================
// NoiseArt — Socket / Link (елементи нодового графа)
// ============================================================================
#pragma once

#include <string>

namespace NoiseArt {

// Тип даних, що тече по з'єднанню. У v1 ребрами тече лише Texture;
// скалярні типи зарезервовано на майбутнє (wiring у параметри ефектів).
enum class SocketType { Texture, Float, Vec2, Vec3, Color };

inline const char* socketTypeName(SocketType t) {
    switch (t) {
        case SocketType::Float: return "Float";
        case SocketType::Vec2:  return "Vec2";
        case SocketType::Vec3:  return "Vec3";
        case SocketType::Color: return "Color";
        case SocketType::Texture:
        default:                return "Texture";
    }
}

// Сокет (пін) ноди. id унікальний у межах графа (призначає NodeGraph).
struct Socket {
    int         id      = -1;
    std::string name;
    SocketType  type    = SocketType::Texture;
    bool        isInput = true;
    int         nodeId  = -1;   // власник
};

// З'єднання: вихідний сокет продюсера → вхідний сокет споживача.
struct Link {
    int id           = -1;
    int fromSocketId = -1;  // output
    int toSocketId   = -1;  // input
};

} // namespace NoiseArt
