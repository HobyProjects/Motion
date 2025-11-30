#pragma once

#include <string>
#include <functional>
#include <vector>
#include <map>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Responsive.hpp"
#include "Scope.hpp"
#include "Button.hpp"
#include "Label.hpp"

namespace Motion
{
    enum class CardStyle
    {
        Neumorphic,     
        Flat,            
        Outlined,        
        Elevated,        
        Glass,           
        Gradient        
    };

    enum class CardSize
    {
        Small,           
        Medium,          
        Large,           
        Auto            
    };

    struct CardConfig
    {
        CardStyle Style{CardStyle::Neumorphic};
        CardSize Size{CardSize::Medium};
        
        ImVec2 MinSize{200.0f, 100.0f};
        ImVec2 MaxSize{FLT_MAX, FLT_MAX};
        ImVec2 FixedSize{0.0f, 0.0f};  
        
        float PaddingX{16.0f};
        float PaddingY{16.0f};
        
        float Rounding{12.0f};
        
        ImVec4 BackgroundColor{0.0f, 0.0f, 0.0f, 0.0f};  
        ImVec4 BorderColor{0.0f, 0.0f, 0.0f, 0.0f};
        ImVec4 ShadowColor{0.0f, 0.0f, 0.0f, 0.0f};
        
        ImVec4 GradientTop{0.2f, 0.3f, 0.5f, 1.0f};
        ImVec4 GradientBottom{0.1f, 0.2f, 0.4f, 1.0f};
        
        bool Hoverable{false};
        bool Clickable{false};
        bool Draggable{false};
        bool Collapsible{false};
        bool DefaultOpen{true};
        
        float BorderThickness{1.0f};
        float ShadowOffset{4.0f};
        float ShadowBlur{8.0f};
        
        bool AnimateHover{true};
        float AnimationSpeed{0.15f};
        ResponsiveLayout::Options Layout{};
    };

    struct CardState
    {
        bool IsHovered{false};
        bool IsClicked{false};
        bool IsCollapsed{false};
        float HoverAnimation{0.0f};
        ImVec2 DragOffset{0.0f, 0.0f};
    };

    class CardTheme
    {
    public:
        static ImVec4 GetBackgroundColor(CardStyle style, bool isDark = true)
        {
            const ImVec4& bgColor = isDark ? ImGui::GetStyle().Colors[ImGuiCol_WindowBg] 
                                            : ImGui::GetStyle().Colors[ImGuiCol_FrameBg];
            
            switch(style)
            {
                case CardStyle::Neumorphic:
                    return AdjustBrightness(bgColor, isDark ? 1.05f : 0.95f);
                
                case CardStyle::Flat:
                    return AdjustBrightness(bgColor, isDark ? 1.08f : 0.92f);
                
                case CardStyle::Outlined:
                    return bgColor;
                
                case CardStyle::Elevated:
                    return AdjustBrightness(bgColor, isDark ? 1.1f : 0.9f);
                
                case CardStyle::Glass:
                    return ImVec4(bgColor.x, bgColor.y, bgColor.z, 0.7f);
                
                case CardStyle::Gradient:
                    return bgColor; // Gradient handled separately
                
                default:
                    return bgColor;
            }
        }

        static ImVec4 GetShadowColor(CardStyle style, bool isDark = true)
        {
            switch(style)
            {
                case CardStyle::Neumorphic:
                    return isDark ? ImVec4(0.0f, 0.0f, 0.0f, 0.3f) 
                                  : ImVec4(0.0f, 0.0f, 0.0f, 0.15f);
                
                case CardStyle::Elevated:
                    return isDark ? ImVec4(0.0f, 0.0f, 0.0f, 0.5f)
                                  : ImVec4(0.0f, 0.0f, 0.0f, 0.25f);
                
                case CardStyle::Flat:
                case CardStyle::Outlined:
                case CardStyle::Glass:
                case CardStyle::Gradient:
                default:
                    return ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }

        static ImVec4 GetBorderColor(CardStyle style, bool isDark = true)
        {
            const ImVec4& borderColor = ImGui::GetStyle().Colors[ImGuiCol_Border];
            
            switch(style)
            {
                case CardStyle::Outlined:
                    return borderColor;
                
                case CardStyle::Glass:
                    return ImVec4(1.0f, 1.0f, 1.0f, 0.2f);
                
                default:
                    return ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }

        static ImVec2 GetPadding(CardSize size)
        {
            switch(size)
            {
                case CardSize::Small:
                    return ImVec2(12.0f, 10.0f);
                case CardSize::Medium:
                    return ImVec2(16.0f, 14.0f);
                case CardSize::Large:
                    return ImVec2(24.0f, 20.0f);
                case CardSize::Auto:
                default:
                    return ImVec2(16.0f, 14.0f);
            }
        }

        static float GetRounding(CardSize size)
        {
            switch(size)
            {
                case CardSize::Small:
                    return 8.0f;
                case CardSize::Medium:
                    return 12.0f;
                case CardSize::Large:
                    return 16.0f;
                case CardSize::Auto:
                default:
                    return 12.0f;
            }
        }

    private:
        static ImVec4 AdjustBrightness(const ImVec4& color, float factor)
        {
            return ImVec4(
                ImClamp(color.x * factor, 0.0f, 1.0f),
                ImClamp(color.y * factor, 0.0f, 1.0f),
                ImClamp(color.z * factor, 0.0f, 1.0f),
                color.w
            );
        }
    };

    class CardRenderer
    {
    public:
        static void DrawNeumorphicShadows(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, 
                                         float rounding, const ImVec4& shadowColor, float offset, float blur)
        {
            ImVec2 darkShadowMin = ImVec2(min.x + offset, min.y + offset);
            ImVec2 darkShadowMax = ImVec2(max.x + offset, max.y + offset);
            
            for(int i = 0; i < 4; ++i)
            {
                float alpha = shadowColor.w * (1.0f - (i / 4.0f)) * 0.5f;
                ImU32 color = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, alpha));
                drawList->AddRectFilled(
                    ImVec2(darkShadowMin.x + i, darkShadowMin.y + i),
                    ImVec2(darkShadowMax.x + i, darkShadowMax.y + i),
                    color, rounding + i
                );
            }

            ImVec2 lightShadowMin = ImVec2(min.x - offset * 0.5f, min.y - offset * 0.5f);
            ImVec2 lightShadowMax = ImVec2(max.x - offset * 0.5f, max.y - offset * 0.5f);
            
            for(int i = 0; i < 3; ++i)
            {
                float alpha = shadowColor.w * (1.0f - (i / 3.0f)) * 0.3f;
                ImU32 color = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, alpha));
                drawList->AddRectFilled(
                    ImVec2(lightShadowMin.x - i, lightShadowMin.y - i),
                    ImVec2(lightShadowMax.x - i, lightShadowMax.y - i),
                    color, rounding + i
                );
            }
        }

        static void DrawElevatedShadow(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, 
                                      float rounding, const ImVec4& shadowColor, float offset, float blur)
        {
            int layers = static_cast<int>(blur);
            for(int i = 0; i < layers; ++i)
            {
                float alpha = shadowColor.w * (1.0f - (static_cast<float>(i) / layers));
                ImU32 color = ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, alpha * 0.3f));
                
                float layerOffset = offset * (static_cast<float>(i) / layers);
                drawList->AddRectFilled(
                    ImVec2(min.x + layerOffset, min.y + layerOffset),
                    ImVec2(max.x + layerOffset, max.y + layerOffset),
                    color, rounding
                );
            }
        }

        static void DrawGlassEffect(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, 
                                   float rounding, const ImVec4& backgroundColor)
        {
            drawList->AddRectFilled(min, max, ImGui::GetColorU32(backgroundColor), rounding);
            
            ImVec2 highlightMin = min;
            ImVec2 highlightMax = ImVec2(max.x, min.y + (max.y - min.y) * 0.5f);
            drawList->AddRectFilledMultiColor(
                highlightMin, highlightMax,
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.1f)),
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.1f)),
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.0f)),
                ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.0f))
            );
        }

        static void DrawGradientBackground(ImDrawList* drawList, const ImVec2& min, const ImVec2& max, 
                                          float rounding, const ImVec4& topColor, const ImVec4& bottomColor)
        {
            drawList->AddRectFilledMultiColor(
                min, max,
                ImGui::GetColorU32(topColor),
                ImGui::GetColorU32(topColor),
                ImGui::GetColorU32(bottomColor),
                ImGui::GetColorU32(bottomColor)
            );
            
            if(rounding > 0.0f)
            {
                drawList->AddRect(min, max, ImGui::GetColorU32(topColor), rounding);
            }
        }
    };

    class Card
    {
    public:
        Card(const std::string& label, const CardConfig& config = {}): m_Label(label), m_Config(config)
        {
            if(m_Config.BackgroundColor.w == 0.0f) m_Config.BackgroundColor = CardTheme::GetBackgroundColor(m_Config.Style);
            if(m_Config.ShadowColor.w == 0.0f) m_Config.ShadowColor = CardTheme::GetShadowColor(m_Config.Style);
            if(m_Config.BorderColor.w == 0.0f) m_Config.BorderColor = CardTheme::GetBorderColor(m_Config.Style);
            
            if(m_Config.PaddingX == 16.0f && m_Config.PaddingY == 16.0f)
            {
                ImVec2 padding = CardTheme::GetPadding(m_Config.Size);
                m_Config.PaddingX = padding.x;
                m_Config.PaddingY = padding.y;
            }
            
            if(m_Config.Rounding == 12.0f) m_Config.Rounding = CardTheme::GetRounding(m_Config.Size);
        }

        bool Begin()
        {
            ImGui::PushID(m_Label.c_str());

            m_ID    = ImGui::GetID(m_Label.c_str());
            m_State = GetState(m_ID);
            
            if(m_Config.Collapsible && !m_State.IsCollapsed) m_State.IsCollapsed = !m_Config.DefaultOpen;
            m_Min = ImGui::GetCursorScreenPos();
            
            if(m_Config.FixedSize.x > 0.0f)
            {
                m_Size.x = m_Config.FixedSize.x;
            }
            else
            {
                float availWidth = ImGui::GetContentRegionAvail().x;
                m_Size.x = ImClamp(availWidth, m_Config.MinSize.x, m_Config.MaxSize.x);
            }
            
            if(m_Config.FixedSize.y > 0.0f)
                m_Size.y = m_Config.FixedSize.y;
            else
                m_Size.y = m_Config.MinSize.y;
            
            m_Max = ImVec2(m_Min.x + m_Size.x, m_Min.y + m_Size.y);
            
            ImVec2 mousePos = ImGui::GetMousePos();
            bool wasHovered = m_State.IsHovered;
            m_State.IsHovered = mousePos.x >= m_Min.x && mousePos.x <= m_Max.x && mousePos.y >= m_Min.y && mousePos.y <= m_Max.y;
            if(m_Config.AnimateHover)
            {
                float targetAnimation = m_State.IsHovered ? 1.0f : 0.0f;
                float animSpeed = m_Config.AnimationSpeed * ImGui::GetIO().DeltaTime * 60.0f;
                m_State.HoverAnimation = ImLerp(m_State.HoverAnimation, targetAnimation, animSpeed);
            }
            
            if(m_Config.Clickable && m_State.IsHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                m_State.IsClicked = true;
                if(m_Config.Collapsible) m_State.IsCollapsed = !m_State.IsCollapsed;
            }
            else if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            {
                m_State.IsClicked = false;
            }
            
            RenderCard();
            
            ImGui::SetCursorScreenPos(ImVec2(m_Min.x + m_Config.PaddingX, m_Min.y + m_Config.PaddingY));
            return !m_State.IsCollapsed;
        }

        void End()
        { 
            if(!m_State.IsCollapsed)
            {
                ImVec2 groupMin = ImGui::GetItemRectMin();
                ImVec2 groupMax = ImGui::GetItemRectMax();
                float contentHeight = (groupMax.y - groupMin.y) + m_Config.PaddingY * 2.0f;
                
                if(m_Config.FixedSize.y == 0.0f)
                {
                    m_Size.y = std::max(contentHeight, m_Config.MinSize.y);
                    m_Max.y = m_Min.y + m_Size.y;
                }
            }
            
            ImGui::SetCursorScreenPos(ImVec2(m_Min.x, m_Max.y + ImGui::GetStyle().ItemSpacing.y));
            ImGui::Dummy(ImVec2(m_Size.x, 0.0f)); // Fix for ImGui boundary assertion
            
            SaveState(m_ID, m_State);
            ImGui::PopID();
        }

        bool IsHovered() const { return m_State.IsHovered; }
        bool IsClicked() const { return m_State.IsClicked; }
        bool IsCollapsed() const { return m_State.IsCollapsed; }

    private:
        void RenderCard()
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            float hoverOffset = m_State.HoverAnimation * 2.0f;
            ImVec2 renderMin = ImVec2(m_Min.x, m_Min.y - hoverOffset);
            ImVec2 renderMax = ImVec2(m_Max.x, m_Max.y - hoverOffset);
            
            switch(m_Config.Style)
            {
                case CardStyle::Neumorphic:
                    CardRenderer::DrawNeumorphicShadows(drawList, renderMin, renderMax, m_Config.Rounding, m_Config.ShadowColor, m_Config.ShadowOffset, m_Config.ShadowBlur);
                    drawList->AddRectFilled(renderMin, renderMax, ImGui::GetColorU32(m_Config.BackgroundColor), m_Config.Rounding);
                    break;
                
                case CardStyle::Flat:
                    drawList->AddRectFilled(renderMin, renderMax, ImGui::GetColorU32(m_Config.BackgroundColor), m_Config.Rounding);
                    break;
                
                case CardStyle::Outlined:
                    drawList->AddRectFilled(renderMin, renderMax, ImGui::GetColorU32(m_Config.BackgroundColor), m_Config.Rounding);
                    drawList->AddRect(renderMin, renderMax, ImGui::GetColorU32(m_Config.BorderColor), m_Config.Rounding, 0, m_Config.BorderThickness);
                    break;
                
                case CardStyle::Elevated:
                    CardRenderer::DrawElevatedShadow(drawList, renderMin, renderMax, 
                        m_Config.Rounding, m_Config.ShadowColor, 
                        m_Config.ShadowOffset * (1.0f + m_State.HoverAnimation * 0.5f), 
                        m_Config.ShadowBlur);
                    drawList->AddRectFilled(renderMin, renderMax, ImGui::GetColorU32(m_Config.BackgroundColor), m_Config.Rounding);
                    break;
                
                case CardStyle::Glass:
                    CardRenderer::DrawGlassEffect(drawList, renderMin, renderMax, m_Config.Rounding, m_Config.BackgroundColor);
                    drawList->AddRect(renderMin, renderMax, ImGui::GetColorU32(m_Config.BorderColor), m_Config.Rounding, 0, 1.0f);
                    break;
                
                case CardStyle::Gradient:
                    CardRenderer::DrawGradientBackground(drawList, renderMin, renderMax, m_Config.Rounding, m_Config.GradientTop, m_Config.GradientBottom);
                    break;
            }
            
            if(m_Config.Hoverable && m_State.IsHovered)
            {
                ImU32 highlightColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.05f * m_State.HoverAnimation));
                drawList->AddRectFilled(renderMin, renderMax, highlightColor, m_Config.Rounding);
            }
            
            if(m_Config.Collapsible)
            {
                ImVec2 arrowPos = ImVec2(renderMax.x - 24.0f, renderMin.y + 12.0f);
                const char* arrow = m_State.IsCollapsed ? "▶" : "▼";
                drawList->AddText(arrowPos, ImGui::GetColorU32(ImGuiCol_Text), arrow);
            }
        }

        static CardState GetState(ImGuiID id)
        {
            ImGuiStorage* storage = ImGui::GetStateStorage();
            CardState state;
            state.IsCollapsed = storage->GetBool(id, false);
            state.HoverAnimation = storage->GetFloat(ImHashStr("hover", 0, id), 0.0f);
            return state;
        }

        static void SaveState(ImGuiID id, const CardState& state)
        {
            ImGuiStorage* storage = ImGui::GetStateStorage();
            storage->SetBool(id, state.IsCollapsed);
            storage->SetFloat(ImHashStr("hover", 0, id), state.HoverAnimation);
        }

    private:
        std::string m_Label;
        CardConfig m_Config;
        CardState m_State;

        ImGuiID m_ID;
        ImVec2 m_Min;
        ImVec2 m_Max;
        ImVec2 m_Size;
    };

    namespace CardPresets
    {
        inline CardConfig Neumorphic()
        {
            CardConfig config;
            config.Style = CardStyle::Neumorphic;
            config.Size = CardSize::Medium;
            config.Hoverable = true;
            config.AnimateHover = true;
            return config;
        }

        inline CardConfig Flat()
        {
            CardConfig config;
            config.Style = CardStyle::Flat;
            config.Size = CardSize::Medium;
            return config;
        }

        inline CardConfig Outlined()
        {
            CardConfig config;
            config.Style = CardStyle::Outlined;
            config.Size = CardSize::Medium;
            config.BorderThickness = 1.5f;
            return config;
        }

        inline CardConfig Elevated()
        {
            CardConfig config;
            config.Style = CardStyle::Elevated;
            config.Size = CardSize::Medium;
            config.Hoverable = true;
            config.AnimateHover = true;
            config.ShadowOffset = 6.0f;
            config.ShadowBlur = 12.0f;
            return config;
        }

        inline CardConfig Glass()
        {
            CardConfig config;
            config.Style = CardStyle::Glass;
            config.Size = CardSize::Medium;
            config.BackgroundColor = ImVec4(0.1f, 0.1f, 0.1f, 0.6f);
            return config;
        }

        inline CardConfig Gradient(const ImVec4& topColor, const ImVec4& bottomColor)
        {
            CardConfig config;
            config.Style = CardStyle::Gradient;
            config.Size = CardSize::Medium;
            config.GradientTop = topColor;
            config.GradientBottom = bottomColor;
            return config;
        }

        inline CardConfig Compact()
        {
            CardConfig config;
            config.Style = CardStyle::Neumorphic;
            config.Size = CardSize::Small;
            return config;
        }

        inline CardConfig Hero()
        {
            CardConfig config;
            config.Style = CardStyle::Elevated;
            config.Size = CardSize::Large;
            config.Hoverable = true;
            config.AnimateHover = true;
            config.ShadowOffset = 8.0f;
            config.ShadowBlur = 16.0f;
            return config;
        }

        inline CardConfig Interactive()
        {
            CardConfig config;
            config.Style = CardStyle::Neumorphic;
            config.Size = CardSize::Medium;
            config.Hoverable = true;
            config.Clickable = true;
            config.AnimateHover = true;
            return config;
        }

        inline CardConfig Collapsible()
        {
            CardConfig config;
            config.Style = CardStyle::Neumorphic;
            config.Size = CardSize::Medium;
            config.Collapsible = true;
            config.DefaultOpen = true;
            return config;
        }
    }
}