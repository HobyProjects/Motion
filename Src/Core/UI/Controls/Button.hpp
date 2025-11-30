#pragma once

#include <string>
#include <functional>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Responsive.hpp"
#include "Scope.hpp"

namespace Motion
{
    enum class ButtonStyle
    {
        Primary,
        Secondary,
        Success,
        Warning,
        Danger,
        Info,
        Default
    };

    struct ButtonConfig
    {
        ButtonStyle Style{ButtonStyle::Default};
        ImVec2 Size{0, 0};  // 0 = auto-size
        bool Disabled{false};
        bool SmallButton{false};
        const char* Tooltip{nullptr};
        ResponsiveLayout::Options Layout{};
    };

    using ButtonCallback = std::function<void()>;

    namespace ButtonInternal
    {
        inline ImVec4 GetStyleColor(ButtonStyle style, bool hovered, bool active, bool disabled)
        {
            if (disabled)
                return ImVec4(0.3f, 0.3f, 0.3f, 0.5f);

            // Base colors for each style
            ImVec4 baseColor;
            switch (style)
            {
                case ButtonStyle::Primary:
                    baseColor = ImVec4(0.26f, 0.59f, 0.98f, 1.0f);  // Blue
                    break;
                case ButtonStyle::Success:
                    baseColor = ImVec4(0.32f, 0.80f, 0.37f, 1.0f);  // Green
                    break;
                case ButtonStyle::Warning:
                    baseColor = ImVec4(1.0f, 0.76f, 0.03f, 1.0f);   // Orange/Yellow
                    break;
                case ButtonStyle::Danger:
                    baseColor = ImVec4(0.90f, 0.16f, 0.22f, 1.0f);  // Red
                    break;
                case ButtonStyle::Info:
                    baseColor = ImVec4(0.23f, 0.76f, 0.82f, 1.0f);  // Cyan
                    break;
                case ButtonStyle::Secondary:
                    baseColor = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);  // Gray
                    break;
                case ButtonStyle::Default:
                default:
                    baseColor = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);  // Dark Gray
                    break;
            }

            if (active)
            {
                // Darker when active (pressed)
                return ImVec4(
                    baseColor.x * 0.7f,
                    baseColor.y * 0.7f,
                    baseColor.z * 0.7f,
                    baseColor.w
                );
            }
            else if (hovered)
            {
                // Lighter when hovered
                return ImVec4(
                    baseColor.x * 1.2f,
                    baseColor.y * 1.2f,
                    baseColor.z * 1.2f,
                    baseColor.w
                );
            }

            return baseColor;
        }

        inline void ApplyButtonStyle(const ButtonConfig& config, bool& wasStylePushed)
        {
            wasStylePushed = false;
            
            if (config.Style != ButtonStyle::Default || config.Disabled)
            {
                ImGuiStyle& style = ImGui::GetStyle();
                
                ImVec4 normalColor = GetStyleColor(config.Style, false, false, config.Disabled);
                ImVec4 hoveredColor = GetStyleColor(config.Style, true, false, config.Disabled);
                ImVec4 activeColor = GetStyleColor(config.Style, false, true, config.Disabled);

                ImGui::PushStyleColor(ImGuiCol_Button, normalColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
                
                wasStylePushed = true;
            }
        }

        inline void PopButtonStyle(bool wasStylePushed)
        {
            if (wasStylePushed)
                ImGui::PopStyleColor(3);
        }
    }

    inline bool Button(const std::string& label, const ButtonConfig& config = {}, const ButtonCallback& callback = nullptr)
    {
        ScopeID id(label);
        
        bool wasStylePushed = false;
        ButtonInternal::ApplyButtonStyle(config, wasStylePushed);
        
        bool clicked = false;
        if (config.SmallButton)
        {
            clicked = ImGui::SmallButton(label.c_str());
        }
        else if (config.Size.x != 0 || config.Size.y != 0)
        {
            clicked = ImGui::Button(label.c_str(), config.Size);
        }
        else
        {
            clicked = ImGui::Button(label.c_str());
        }
        
        ButtonInternal::PopButtonStyle(wasStylePushed);
        
        if (config.Disabled)
            clicked = false;
        
        if (config.Tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        
        if (clicked && callback)
            callback();
        
        return clicked;
    }

    inline bool ButtonLabeled(const std::string& label, const std::string& buttonText, const ButtonConfig& config = {}, const ButtonCallback& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool wasStylePushed = false;
        ButtonInternal::ApplyButtonStyle(config, wasStylePushed);
        
        bool clicked = false;
        if (config.SmallButton)
        {
            clicked = ImGui::SmallButton(buttonText.c_str());
        }
        else if (config.Size.x != 0 || config.Size.y != 0)
        {
            clicked = ImGui::Button(buttonText.c_str(), config.Size);
        }
        else
        {
            clicked = ImGui::Button(buttonText.c_str());
        }
        
        ButtonInternal::PopButtonStyle(wasStylePushed);
        
        if (config.Disabled)
            clicked = false;
        
        if (config.Tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        
        if (clicked && callback)
            callback();
        
        return clicked;
    }

    inline bool IconButton(const char* icon, const ButtonConfig& config = {}, const ButtonCallback& callback = nullptr)
    {
        ScopeID id(icon);
        
        bool wasStylePushed = false;
        ButtonInternal::ApplyButtonStyle(config, wasStylePushed);
        
        bool clicked = false;
        if (config.Size.x != 0 || config.Size.y != 0)
        {
            clicked = ImGui::Button(icon, config.Size);
        }
        else
        {
            clicked = ImGui::Button(icon);
        }
        
        ButtonInternal::PopButtonStyle(wasStylePushed);
        
        if (config.Disabled)
            clicked = false;
        
        if (config.Tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        
        if (clicked && callback)
            callback();
        
        return clicked;
    }

    inline bool ArrowButton(const std::string& id, ImGuiDir dir, const ButtonConfig& config = {}, const ButtonCallback& callback = nullptr)
    {
        ScopeID scopeId(id);
        
        bool wasStylePushed = false;
        ButtonInternal::ApplyButtonStyle(config, wasStylePushed);
        
        bool clicked = ImGui::ArrowButton("##arrow", dir);
        
        ButtonInternal::PopButtonStyle(wasStylePushed);
        
        if (config.Disabled)
            clicked = false;
        
        if (config.Tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        
        if (clicked && callback)
            callback();
        
        return clicked;
    }

    struct ButtonGroupConfig
    {
        float Spacing{4.0f};
        bool SameLine{true};
    };

    class ButtonGroup
    {
    public:
        explicit ButtonGroup(const ButtonGroupConfig& config = {})
            : m_Config(config), m_FirstButton(true)
        {
        }

        bool AddButton(const std::string& label, const ButtonConfig& config = {}, const ButtonCallback& callback = nullptr)
        {
            if (!m_FirstButton && m_Config.SameLine)
            {
                ImGui::SameLine(0.0f, m_Config.Spacing);
            }
            m_FirstButton = false;

            return Button(label, config, callback);
        }

        bool AddIconButton(const char* icon, const ButtonConfig& config = {}, const ButtonCallback& callback = nullptr)
        {
            if (!m_FirstButton && m_Config.SameLine)
            {
                ImGui::SameLine(0.0f, m_Config.Spacing);
            }
            m_FirstButton = false;

            return IconButton(icon, config, callback);
        }

    private:
        ButtonGroupConfig m_Config;
        bool m_FirstButton;
    };

    struct ConfirmButtonConfig
    {
        std::string ConfirmText{"Are you sure?"};
        std::string ConfirmButtonText{"Yes"};
        std::string CancelButtonText{"No"};
        ButtonConfig ButtonStyle{};
        ButtonConfig ConfirmStyle{};
        ButtonConfig CancelStyle{};
    };

    inline bool ConfirmButton(const std::string& label, const ConfirmButtonConfig& config = {}, const ButtonCallback& callback = nullptr)
    {
        ScopeID id(label);
        
        bool clicked = Button(label, config.ButtonStyle);
        
        if (clicked)
        {
            ImGui::OpenPopup("confirm_popup");
        }

        bool confirmed = false;
        if (ImGui::BeginPopupModal("confirm_popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", config.ConfirmText.c_str());
            ImGui::Separator();
            
            ButtonGroup group;
            if (group.AddButton(config.ConfirmButtonText, config.ConfirmStyle))
            {
                confirmed = true;
                ImGui::CloseCurrentPopup();
            }
            
            if (group.AddButton(config.CancelButtonText, config.CancelStyle))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        
        if (confirmed && callback)
            callback();
        
        return confirmed;
    }
}