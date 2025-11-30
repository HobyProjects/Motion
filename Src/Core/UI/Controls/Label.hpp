#pragma once

#include <string>
#include <imgui/imgui.h>
#include <glm/glm.hpp>

#include "Responsive.hpp"
#include "Scope.hpp"

namespace Motion
{
    enum class LabelAlign
    {
        Left,
        Center,
        Right
    };

    enum class LabelWeight
    {
        Normal,
        Bold,
        Light
    };

    struct LabelConfig
    {
        bool Disabled{false};          
        bool Wrapped{false};            
        bool Bullet{false};            
        bool Separator{false};
        bool SeparatorBefore{false};
        bool Selectable{false};
        bool CopyOnRightClick{true};
        
        LabelAlign Alignment{LabelAlign::Left};
        LabelWeight Weight{LabelWeight::Normal};
        
        ImVec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
        float Alpha{1.0f};
        
        const char* Tooltip{nullptr};
        
        ResponsiveLayout::Options Layout{};
    };

    enum class HeadingLevel
    {
        H1,  // 2.0x font size
        H2,  // 1.75x font size
        H3,  // 1.5x font size
        H4,  // 1.25x font size
        H5,  // 1.1x font size
        H6   // 1.0x font size (same as normal)
    };

    struct HeadingConfig
    {
        bool Separator{true};
        bool SeparatorBefore{false};
        bool Underline{false};
        bool Bullet{false};
        
        ImVec4 Color{1.0f, 1.0f, 1.0f, 1.0f};
        float Alpha{1.0f};
        
        const char* Tooltip{nullptr};
    };

    namespace LabelInternal
    {
        inline float GetHeadingScale(HeadingLevel level)
        {
            switch(level)
            {
                case HeadingLevel::H1: return 2.0f;
                case HeadingLevel::H2: return 1.75f;
                case HeadingLevel::H3: return 1.5f;
                case HeadingLevel::H4: return 1.25f;
                case HeadingLevel::H5: return 1.1f;
                case HeadingLevel::H6: return 1.0f;
                default: return 1.0f;
            }
        }

        inline const char* GetHeadingFont(HeadingLevel level)
        {
            switch(level)
            {
                case HeadingLevel::H1: return "JetBrainsMono-Bold-H1";
                case HeadingLevel::H2: return "JetBrainsMono-Bold-H2";
                case HeadingLevel::H3: return "JetBrainsMono-Bold-H3";
                case HeadingLevel::H4: return "JetBrainsMono-Bold-H4";
                case HeadingLevel::H5: return "JetBrainsMono-Bold-H5";
                case HeadingLevel::H6: return "JetBrainsMono-Bold-H6";
                default: return "JetBrainsMono-Bold-H5";
            }
        }

        inline void ApplyAlignment(const char* text, LabelAlign align, float availWidth = 0.0f)
        {
            if(align == LabelAlign::Left)
                return;

            if(availWidth == 0.0f)
                availWidth = ImGui::GetContentRegionAvail().x;

            ImVec2 textSize = ImGui::CalcTextSize(text);
            
            switch(align)
            {
                case LabelAlign::Center:
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - textSize.x) * 0.5f);
                    break;
                case LabelAlign::Right:
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (availWidth - textSize.x));
                    break;
                default:
                    break;
            }
        }

        inline void RenderText(const char* text, const LabelConfig& config)
        {
            ImVec4 finalColor = config.Color;
            finalColor.w *= config.Alpha;

            if(config.Disabled)
            {
                ImGui::TextDisabled("%s", text);
            }
            else if(config.Color.x != 1.0f || config.Color.y != 1.0f || 
                    config.Color.z != 1.0f || config.Alpha != 1.0f)
            {
                ImGui::TextColored(finalColor, "%s", text);
            }
            else if(config.Wrapped)
            {
                ImGui::TextWrapped("%s", text);
            }
            else
            {
                ImGui::Text("%s", text);
            }
        }
    }

    inline void Label(const std::string& label, const std::string& text, const LabelConfig& config = {})
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        if(config.SeparatorBefore)
            ImGui::Separator();

        if(config.Bullet)
            ImGui::Bullet();

        LabelInternal::ApplyAlignment(text.c_str(), config.Alignment);
        
        if(config.Selectable)
        {
            ImGui::Selectable(text.c_str(), false, ImGuiSelectableFlags_AllowItemOverlap);
        }
        else
        {
            LabelInternal::RenderText(text.c_str(), config);
        }
        
        if(config.Separator)
            ImGui::Separator();
        
        if(config.Tooltip && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        else if(ImGui::IsItemHovered() && text.length() > 50)
        {
            ImGui::SetTooltip("%s", text.c_str());
        }
        
        if(config.CopyOnRightClick && ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("label_context");

        if(config.CopyOnRightClick && ImGui::BeginPopup("label_context"))
        {
            if(ImGui::MenuItem("Copy", nullptr, false, !text.empty()))
            {
                ImGui::SetClipboardText(text.c_str());
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }

    inline void LabelSimple(const std::string& text, const LabelConfig& config = {})
    {
        if(config.SeparatorBefore)
            ImGui::Separator();

        if(config.Bullet)
            ImGui::Bullet();

        LabelInternal::ApplyAlignment(text.c_str(), config.Alignment);
        LabelInternal::RenderText(text.c_str(), config);
        
        if(config.Separator)
            ImGui::Separator();
        
        if(config.Tooltip && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
    }

    inline void Heading(const std::string& text, HeadingLevel level = HeadingLevel::H3, const HeadingConfig& config = {})
    {
        ScopeID id(text);
        if(config.SeparatorBefore) ImGui::Separator();
        ImFont* font = UserInterface::FontManager::GetFont(LabelInternal::GetHeadingFont(level));
        ImGui::PushFont(font);

        ImVec4 finalColor = config.Color;
        finalColor.w *= config.Alpha;

        if(config.Bullet) ImGui::Bullet();
        if(config.Color.x != 1.0f || config.Color.y != 1.0f || config.Color.z != 1.0f || config.Alpha != 1.0f)
        {
            ImGui::TextColored(finalColor, "%s", text.c_str());
        }
        else
        {
            ImGui::Text("%s", text.c_str());
        }
        
        ImGui::PopFont();

        if(config.Underline)
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 pos = ImGui::GetItemRectMin();
            ImVec2 size = ImGui::GetItemRectSize();
            ImU32 color = ImGui::GetColorU32(finalColor);
            
            drawList->AddLine(
                ImVec2(pos.x, pos.y + size.y),
                ImVec2(pos.x + size.x, pos.y + size.y),
                color,
                2.0f
            );
        }
        
        if(config.Separator)
            ImGui::Separator();

        if(config.Tooltip && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
    }

    inline void LabelValue(const std::string& label, const std::string& value, const LabelConfig& config = {})
    {
        Label(label, value, config);
    }

    inline void LabelValue(const std::string& label, float value, const char* format = "%.3f", const LabelConfig& config = {})
    {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), format, value);
        Label(label, buffer, config);
    }

    inline void LabelValue(const std::string& label, int value, const LabelConfig& config = {})
    {
        Label(label, std::to_string(value), config);
    }

    inline void LabelValue(const std::string& label, size_t value, const LabelConfig& config = {})
    {
        Label(label, std::to_string(value), config);
    }

    inline void LabelValue(const std::string& label, bool value, const LabelConfig& config = {})
    {
        Label(label, value ? "True" : "False", config);
    }

    inline void LabelValue(const std::string& label, const glm::vec2& value, const char* format = "%.3f", const LabelConfig& config = {})
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), format, value.x);
        std::string result = "X: " + std::string(buffer) + ", Y: ";
        snprintf(buffer, sizeof(buffer), format, value.y);
        result += buffer;
        Label(label, result, config);
    }

    inline void LabelValue(const std::string& label, const glm::vec3& value, const char* format = "%.3f", const LabelConfig& config = {})
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), format, value.x);
        std::string result = "X: " + std::string(buffer) + ", Y: ";
        snprintf(buffer, sizeof(buffer), format, value.y);
        result += std::string(buffer) + ", Z: ";
        snprintf(buffer, sizeof(buffer), format, value.z);
        result += buffer;
        Label(label, result, config);
    }

    inline void LabelValue(const std::string& label, const glm::vec4& value, const char* format = "%.3f", const LabelConfig& config = {})
    {
        char buffer[128];
        snprintf(buffer, sizeof(buffer), format, value.x);
        std::string result = "X: " + std::string(buffer) + ", Y: ";
        snprintf(buffer, sizeof(buffer), format, value.y);
        result += std::string(buffer) + ", Z: ";
        snprintf(buffer, sizeof(buffer), format, value.z);
        result += std::string(buffer) + ", W: ";
        snprintf(buffer, sizeof(buffer), format, value.w);
        result += buffer;
        Label(label, result, config);
    }

}