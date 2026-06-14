#include "KeybindManager.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace NoiseArt {

void KeybindManager::registerAction(const std::string& actionName, ImGuiKey defaultKey) {
    if (m_keybinds.find(actionName) == m_keybinds.end()) {
        m_keybinds[actionName] = {actionName, defaultKey, defaultKey};
    }
}

bool KeybindManager::isActionPressed(const std::string& actionName) const {
    auto it = m_keybinds.find(actionName);
    if (it != m_keybinds.end() && it->second.key != ImGuiKey_None) {
        return ImGui::IsKeyPressed(it->second.key, false);
    }
    return false;
}

bool KeybindManager::isActionReleased(const std::string& actionName) const {
    auto it = m_keybinds.find(actionName);
    if (it != m_keybinds.end() && it->second.key != ImGuiKey_None) {
        return ImGui::IsKeyReleased(it->second.key);
    }
    return false;
}

bool KeybindManager::isActionDown(const std::string& actionName) const {
    auto it = m_keybinds.find(actionName);
    if (it != m_keybinds.end() && it->second.key != ImGuiKey_None) {
        return ImGui::IsKeyDown(it->second.key);
    }
    return false;
}

void KeybindManager::setKeybind(const std::string& actionName, ImGuiKey key) {
    auto it = m_keybinds.find(actionName);
    if (it != m_keybinds.end()) {
        it->second.key = key;
    }
}

std::string KeybindManager::getKeyName(ImGuiKey key) {
    if (key == ImGuiKey_None) return "None";
    const char* name = ImGui::GetKeyName(key);
    return name ? name : "Unknown";
}

void KeybindManager::load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    try {
        json j;
        file >> j;
        for (auto& [key, val] : j.items()) {
            if (m_keybinds.find(key) != m_keybinds.end() && val.is_number_integer()) {
                m_keybinds[key].key = static_cast<ImGuiKey>(val.get<int>());
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Failed to parse keybinds: " << e.what() << "\n";
    }
}

void KeybindManager::save(const std::string& filepath) const {
    json j;
    for (const auto& [name, bind] : m_keybinds) {
        j[name] = static_cast<int>(bind.key);
    }

    std::ofstream file(filepath);
    if (file.is_open()) {
        file << j.dump(4);
    }
}

} // namespace NoiseArt
