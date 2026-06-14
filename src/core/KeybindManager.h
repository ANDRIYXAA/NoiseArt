#pragma once

#include <string>
#include <map>
#include <vector>
#include <imgui.h>

namespace NoiseArt {

struct Keybind {
    std::string name;
    ImGuiKey key;      // Призначена клавіша (ImGuiKey_None якщо не призначено)
    ImGuiKey defaultKey; // Клавіша за замовчуванням
};

class KeybindManager {
public:
    static KeybindManager& get() {
        static KeybindManager instance;
        return instance;
    }

    void registerAction(const std::string& actionName, ImGuiKey defaultKey = ImGuiKey_None);
    
    // Перевіряє, чи натиснута кнопка для даної дії
    bool isActionPressed(const std::string& actionName) const;
    bool isActionReleased(const std::string& actionName) const;
    bool isActionDown(const std::string& actionName) const;

    void setKeybind(const std::string& actionName, ImGuiKey key);
    
    const std::map<std::string, Keybind>& getAllKeybinds() const { return m_keybinds; }
    
    // Збереження та завантаження з файлу
    void load(const std::string& filepath);
    void save(const std::string& filepath) const;

    static std::string getKeyName(ImGuiKey key);

private:
    KeybindManager() = default;
    ~KeybindManager() = default;

    std::map<std::string, Keybind> m_keybinds;
};

} // namespace NoiseArt
