#include "SettingsPanel.h"
#include "core/Image.h"
#include "core/KeybindManager.h"
#include <imgui.h>
#include <portable-file-dialogs.h>
#include <iostream>

#include "App/App.h"

namespace NoiseArt {

SettingsPanel::SettingsPanel()
{
}

void SettingsPanel::render(bool* open, AppSettings& settings)
{
    if (!*open) return;

    if (ImGui::Begin("Settings", open)) {
        if (ImGui::BeginTabBar("SettingsTabs")) {
            
            if (ImGui::BeginTabItem("Canvas")) {
                ImGui::Text("Grid Settings:");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Show Grid", &settings.showGrid);
                ImGui::Checkbox("Snap to Grid", &settings.snapToGrid);
                ImGui::Checkbox("Auto-scale Grid", &settings.autoGridScale);
                
                if (!settings.autoGridScale) {
                    ImGui::DragFloat("Grid Size", &settings.gridSize, 1.0f, 10.0f, 1000.0f);
                } else {
                    ImGui::BeginDisabled();
                    ImGui::DragFloat("Grid Size (Auto)", &settings.gridSize, 0.0f, 10.0f, 1000.0f);
                    ImGui::EndDisabled();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Performance")) {
                ImGui::Text("Оптимізація рендеру (важкі шари / шейдери):");
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Smart render cache", &settings.optShaderCache);
                ImGui::TextDisabled("  Рахувати шейдер лише при зміні. Статичні (Speed=0) майже безкоштовні.");
                ImGui::Spacing();
                ImGui::Checkbox("Throttle to ~30 FPS", &settings.optThrottle);
                ImGui::TextDisabled("  Анімовані шейдери оновлювати не частіше ~30 разів/сек.");
                ImGui::Spacing();
                ImGui::Checkbox("Low-res while dragging", &settings.optLowResDrag);
                ImGui::TextDisabled("  Під час перетягування рендерити в 256px (вчетверо менший readback).");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("General")) {
                ImGui::Text("App Background Color:");
                ImGui::ColorEdit4("Clear Color", m_clearColor);
                
                ImGui::Spacing();
                ImGui::Text("Background Image:");
                if (ImGui::Button("Load Background...")) {
                    auto result = pfd::open_file("Choose Background Image", "",
                        { "Images", "*.png *.jpg *.jpeg *.bmp", "All Files", "*" }).result();
                    if (!result.empty()) {
                        Image img;
                        if (img.loadFromFile(result[0])) {
                            m_bgTexture.update(img);
                            m_hasBgImage = true;
                        }
                    }
                }
                if (m_hasBgImage) {
                    ImGui::SameLine();
                    if (ImGui::Button("Clear Image")) {
                        m_hasBgImage = false;
                    }
                }
                
                ImGui::Separator();
                ImGui::Text("UI Style");
                
                ImGuiStyle& style = ImGui::GetStyle();
                
                ImGui::SliderFloat("Global Alpha", &style.Alpha, 0.2f, 1.0f, "%.2f");
                
                ImGui::Spacing();
                ImGui::Text("Colors");
                ImGui::ColorEdit4("Window Bg", (float*)&style.Colors[ImGuiCol_WindowBg], ImGuiColorEditFlags_AlphaPreview);
                ImGui::ColorEdit4("Title Bg", (float*)&style.Colors[ImGuiCol_TitleBg], ImGuiColorEditFlags_AlphaPreview);
                ImGui::ColorEdit4("Title Bg Active", (float*)&style.Colors[ImGuiCol_TitleBgActive], ImGuiColorEditFlags_AlphaPreview);
                ImGui::ColorEdit4("Frame Bg", (float*)&style.Colors[ImGuiCol_FrameBg], ImGuiColorEditFlags_AlphaPreview);
                ImGui::ColorEdit4("Button", (float*)&style.Colors[ImGuiCol_Button], ImGuiColorEditFlags_AlphaPreview);
                ImGui::ColorEdit4("Button Hovered", (float*)&style.Colors[ImGuiCol_ButtonHovered], ImGuiColorEditFlags_AlphaPreview);
                ImGui::ColorEdit4("Button Active", (float*)&style.Colors[ImGuiCol_ButtonActive], ImGuiColorEditFlags_AlphaPreview);
                ImGui::ColorEdit4("Text", (float*)&style.Colors[ImGuiCol_Text], ImGuiColorEditFlags_AlphaPreview);
                
                ImGui::Spacing();
                ImGui::Text("Rounding");
                ImGui::SliderFloat("Window Rounding", &style.WindowRounding, 0.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("Frame Rounding", &style.FrameRounding, 0.0f, 20.0f, "%.1f");
                ImGui::SliderFloat("Tab Rounding", &style.TabRounding, 0.0f, 20.0f, "%.1f");
                
                ImGui::Spacing();
                if (ImGui::Button("Reset to Defaults")) {
                    style.WindowRounding = 6.0f;
                    style.FrameRounding = 4.0f;
                    style.TabRounding = 4.0f;
                    style.Alpha = 1.0f;
                    style.Colors[ImGuiCol_WindowBg]        = ImVec4(0.12f, 0.12f, 0.15f, 1.00f);
                    style.Colors[ImGuiCol_TitleBg]         = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
                    style.Colors[ImGuiCol_TitleBgActive]   = ImVec4(0.15f, 0.12f, 0.22f, 1.00f);
                    style.Colors[ImGuiCol_FrameBg]         = ImVec4(0.18f, 0.18f, 0.22f, 1.00f);
                    style.Colors[ImGuiCol_Button]          = ImVec4(0.25f, 0.20f, 0.35f, 1.00f);
                    style.Colors[ImGuiCol_ButtonHovered]   = ImVec4(0.35f, 0.28f, 0.50f, 1.00f);
                    style.Colors[ImGuiCol_ButtonActive]    = ImVec4(0.45f, 0.35f, 0.60f, 1.00f);
                    style.Colors[ImGuiCol_Text]            = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
                }
                
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Keybinds")) {
                auto& km = KeybindManager::get();
                const auto& keybinds = km.getAllKeybinds();

                if (ImGui::BeginTable("KeybindsTable", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);
                    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 200.0f);
                    ImGui::TableHeadersRow();

                    for (const auto& [name, bind] : keybinds) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("%s", name.c_str());

                        ImGui::TableNextColumn();
                        
                        std::string buttonLabel;
                        if (m_recordingAction == name) {
                            buttonLabel = "Press key (ESC clear)";
                        } else {
                            buttonLabel = KeybindManager::getKeyName(bind.key);
                        }

                        if (ImGui::Button((buttonLabel + "##" + name).c_str(), ImVec2(-FLT_MIN, 0))) {
                            m_recordingAction = name;
                        }
                    }
                    ImGui::EndTable();
                }

                if (!m_recordingAction.empty()) {
                    for (int i = ImGuiKey_NamedKey_BEGIN; i < ImGuiKey_NamedKey_END; ++i) {
                        ImGuiKey key = static_cast<ImGuiKey>(i);
                        if (ImGui::IsKeyPressed(key, false)) {
                            if (key == ImGuiKey_Escape) {
                                km.setKeybind(m_recordingAction, ImGuiKey_None);
                            } else {
                                km.setKeybind(m_recordingAction, key);
                            }
                            km.save("keybinds.json");
                            m_recordingAction = "";
                            break;
                        }
                    }
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void SettingsPanel::renderBackground(float screenWidth, float screenHeight)
{
    if (m_hasBgImage && m_bgTexture.getID() != 0) {
        ImGui::GetBackgroundDrawList()->AddImage(
            (ImTextureID)(intptr_t)m_bgTexture.getID(),
            ImVec2(0, 0),
            ImVec2(screenWidth, screenHeight)
        );
    }
}

} // namespace NoiseArt
