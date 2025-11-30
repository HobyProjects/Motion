#pragma once

#include <string>
#include <functional>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Responsive.hpp"
#include "Scope.hpp"

namespace Motion
{
    enum class ToggleSwitchStyle
    {
        iOS,           
        Android,      
        Modern,       
        Retro,         
        Minimal         
    };

    enum class ToggleSwitchSize
    {
        Small,         
        Medium,     
        Large          
    };

    struct ToggleSwitchConfig
    {
        ToggleSwitchStyle Style{ToggleSwitchStyle::iOS};
        ToggleSwitchSize Size{ToggleSwitchSize::Medium};
        
        ImVec4 OnColor{0.0f, 0.0f, 0.0f, 0.0f};  
        ImVec4 OffColor{0.0f, 0.0f, 0.0f, 0.0f};   
        ImVec4 KnobColor{0.0f, 0.0f, 0.0f, 0.0f};   
        ImVec4 BorderColor{0.0f, 0.0f, 0.0f, 0.0f}; 
        
        bool Animated{true};
        float AnimationSpeed{0.15f};
        
        bool Disabled{false};
        bool ShowLabel{true};
        ResponsiveLayout::Options Layout{};
    };

    using ToggleSwitchCallback = std::function<void(bool)>;

    class ToggleSwitchTheme
    {
    public:
        static ImVec2 GetSize(ToggleSwitchSize size, ToggleSwitchStyle style)
        {
            switch(size)
            {
                case ToggleSwitchSize::Small:
                    return style == ToggleSwitchStyle::Android ? ImVec2(32.0f, 18.0f) : ImVec2(36.0f, 20.0f);
                    
                case ToggleSwitchSize::Medium:
                    return style == ToggleSwitchStyle::Android ? ImVec2(40.0f, 22.0f) : ImVec2(44.0f, 24.0f);
                    
                case ToggleSwitchSize::Large:
                    return style == ToggleSwitchStyle::Android ? ImVec2(52.0f, 28.0f) : ImVec2(56.0f, 32.0f);
                    
                default:
                    return ImVec2(44.0f, 24.0f);
            }
        }

        static ImVec4 GetOnColor(ToggleSwitchStyle style)
        {
            switch(style)
            {
                case ToggleSwitchStyle::iOS:
                    return ImVec4(0.2f, 0.8f, 0.4f, 1.0f);  // Green
                    
                case ToggleSwitchStyle::Android:
                    return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Blue
                    
                case ToggleSwitchStyle::Modern:
                    return ImVec4(0.5f, 0.3f, 0.9f, 1.0f);  // Purple
                    
                case ToggleSwitchStyle::Retro:
                    return ImVec4(1.0f, 0.6f, 0.2f, 1.0f);  // Orange
                    
                case ToggleSwitchStyle::Minimal:
                    return ImVec4(0.3f, 0.3f, 0.3f, 1.0f);  // Dark gray
                    
                default:
                    return ImVec4(0.2f, 0.8f, 0.4f, 1.0f);
            }
        }

        static ImVec4 GetOffColor(ToggleSwitchStyle style)
        {
            switch(style)
            {
                case ToggleSwitchStyle::iOS:
                case ToggleSwitchStyle::Android:
                case ToggleSwitchStyle::Modern:
                    return ImVec4(0.5f, 0.5f, 0.5f, 0.5f);  // Gray transparent
                    
                case ToggleSwitchStyle::Retro:
                    return ImVec4(0.3f, 0.3f, 0.3f, 1.0f);  // Dark
                    
                case ToggleSwitchStyle::Minimal:
                    return ImVec4(0.6f, 0.6f, 0.6f, 0.3f);  // Light gray transparent
                    
                default:
                    return ImVec4(0.5f, 0.5f, 0.5f, 0.5f);
            }
        }

        static ImVec4 GetKnobColor(ToggleSwitchStyle style)
        {
            switch(style)
            {
                case ToggleSwitchStyle::iOS:
                case ToggleSwitchStyle::Android:
                case ToggleSwitchStyle::Modern:
                case ToggleSwitchStyle::Minimal:
                    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);  // White
                    
                case ToggleSwitchStyle::Retro:
                    return ImVec4(0.9f, 0.9f, 0.9f, 1.0f);  // Off-white
                    
                default:
                    return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
            }
        }

        static ImVec4 GetBorderColor(ToggleSwitchStyle style, bool isOn)
        {
            switch(style)
            {
                case ToggleSwitchStyle::iOS:
                    return ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // No border
                    
                case ToggleSwitchStyle::Android:
                    return isOn ? ImVec4(0.4f, 0.7f, 1.0f, 0.8f) : ImVec4(0.5f, 0.5f, 0.5f, 0.4f);
                    
                case ToggleSwitchStyle::Modern:
                    return isOn ? ImVec4(0.5f, 0.3f, 0.9f, 0.6f) : ImVec4(0.5f, 0.5f, 0.5f, 0.3f);
                    
                case ToggleSwitchStyle::Retro:
                    return ImVec4(0.2f, 0.2f, 0.2f, 1.0f);  // Dark border
                    
                case ToggleSwitchStyle::Minimal:
                    return ImVec4(0.0f, 0.0f, 0.0f, 0.0f);  // No border
                    
                default:
                    return ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            }
        }
    };

    class ToggleSwitchRenderer
    {
    public:
        static void DrawiOS(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size,
                           bool isOn, float t, const ImVec4& onColor, const ImVec4& offColor,
                           const ImVec4& knobColor, bool disabled)
        {
            float radius = size.y * 0.5f;
            float knobRadius = radius * 0.8f;
            
            ImVec4 bgColor = ImVec4(
                ImLerp(offColor.x, onColor.x, t),
                ImLerp(offColor.y, onColor.y, t),
                ImLerp(offColor.z, onColor.z, t),
                ImLerp(offColor.w, onColor.w, t)
            );
            
            if(disabled)
                bgColor = ImVec4(bgColor.x * 0.5f, bgColor.y * 0.5f, bgColor.z * 0.5f, bgColor.w * 0.5f);
            
            ImVec2 trackMin = pos;
            ImVec2 trackMax = ImVec2(pos.x + size.x, pos.y + size.y);
            drawList->AddRectFilled(trackMin, trackMax, ImGui::GetColorU32(bgColor), radius);
            
            float knobX = ImLerp(pos.x + radius, pos.x + size.x - radius, t);
            ImVec2 knobCenter = ImVec2(knobX, pos.y + radius);
            
            ImVec2 shadowOffset = ImVec2(0.0f, 1.0f);
            drawList->AddCircleFilled(
                ImVec2(knobCenter.x + shadowOffset.x, knobCenter.y + shadowOffset.y),
                knobRadius,
                ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.3f))
            );
            
            ImVec4 finalKnobColor = disabled ? ImVec4(knobColor.x * 0.7f, knobColor.y * 0.7f, knobColor.z * 0.7f, knobColor.w) : knobColor;
            drawList->AddCircleFilled(knobCenter, knobRadius, ImGui::GetColorU32(finalKnobColor));
        }

        static void DrawAndroid(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size,
                               bool isOn, float t, const ImVec4& onColor, const ImVec4& offColor,
                               const ImVec4& knobColor, const ImVec4& borderColor, bool disabled)
        {
            float radius = size.y * 0.5f;
            float knobRadius = radius * 0.7f;
            
            ImVec4 bgColor = ImVec4(
                ImLerp(offColor.x, onColor.x, t),
                ImLerp(offColor.y, onColor.y, t),
                ImLerp(offColor.z, onColor.z, t),
                ImLerp(offColor.w, onColor.w, t)
            );
            
            if(disabled)
                bgColor = ImVec4(bgColor.x * 0.5f, bgColor.y * 0.5f, bgColor.z * 0.5f, bgColor.w * 0.5f);
            
            ImVec2 trackMin = pos;
            ImVec2 trackMax = ImVec2(pos.x + size.x, pos.y + size.y);
            drawList->AddRectFilled(trackMin, trackMax, ImGui::GetColorU32(bgColor), radius);
            
            if(borderColor.w > 0.0f)
            {
                drawList->AddRect(trackMin, trackMax, ImGui::GetColorU32(borderColor), radius, 0, 1.5f);
            }
            
            float knobX = ImLerp(pos.x + radius, pos.x + size.x - radius, t);
            ImVec2 knobCenter = ImVec2(knobX, pos.y + radius);
            
            if(t > 0.0f && t < 1.0f)
            {
                float rippleRadius = knobRadius * (1.0f + t * 0.3f);
                ImVec4 rippleColor = bgColor;
                rippleColor.w *= 0.3f;
                drawList->AddCircleFilled(knobCenter, rippleRadius, ImGui::GetColorU32(rippleColor));
            }
            
            ImVec4 finalKnobColor = disabled ? ImVec4(knobColor.x * 0.7f, knobColor.y * 0.7f, knobColor.z * 0.7f, knobColor.w) : knobColor;
            drawList->AddCircleFilled(knobCenter, knobRadius, ImGui::GetColorU32(finalKnobColor));
        }

        static void DrawModern(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size,
                              bool isOn, float t, const ImVec4& onColor, const ImVec4& offColor,
                              const ImVec4& knobColor, const ImVec4& borderColor, bool disabled)
        {
            float radius = size.y * 0.5f;
            float knobRadius = radius * 0.75f;
            
            ImVec4 bgColor = ImVec4(
                ImLerp(offColor.x, onColor.x, t),
                ImLerp(offColor.y, onColor.y, t),
                ImLerp(offColor.z, onColor.z, t),
                ImLerp(offColor.w, onColor.w, t)
            );
            
            if(disabled)
                bgColor = ImVec4(bgColor.x * 0.5f, bgColor.y * 0.5f, bgColor.z * 0.5f, bgColor.w * 0.5f);
            
            ImVec2 trackMin = pos;
            ImVec2 trackMax = ImVec2(pos.x + size.x, pos.y + size.y);
            
            ImU32 topColor = ImGui::GetColorU32(bgColor);
            ImU32 bottomColor = ImGui::GetColorU32(ImVec4(bgColor.x * 0.8f, bgColor.y * 0.8f, bgColor.z * 0.8f, bgColor.w));
            drawList->AddRectFilledMultiColor(trackMin, trackMax, topColor, topColor, bottomColor, bottomColor);
            drawList->AddRectFilled(trackMin, trackMax, ImGui::GetColorU32(bgColor), radius);
            
            if(borderColor.w > 0.0f)
            {
                ImVec4 glowColor = borderColor;
                glowColor.w *= t;
                drawList->AddRect(trackMin, trackMax, ImGui::GetColorU32(glowColor), radius, 0, 2.0f);
            }
            
            float knobX = ImLerp(pos.x + radius, pos.x + size.x - radius, t);
            ImVec2 knobCenter = ImVec2(knobX, pos.y + radius);
            
            ImVec4 finalKnobColor = disabled ? ImVec4(knobColor.x * 0.7f, knobColor.y * 0.7f, knobColor.z * 0.7f, knobColor.w) : knobColor;
            drawList->AddCircleFilled(knobCenter, knobRadius, ImGui::GetColorU32(finalKnobColor));
            
            ImVec2 shineCenter = ImVec2(knobCenter.x - knobRadius * 0.3f, knobCenter.y - knobRadius * 0.3f);
            drawList->AddCircleFilled(shineCenter, knobRadius * 0.4f, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.4f)));
        }

        static void DrawRetro(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size,
                             bool isOn, float t, const ImVec4& onColor, const ImVec4& offColor,
                             const ImVec4& knobColor, const ImVec4& borderColor, bool disabled)
        {
            float radius = 4.0f; 
            ImVec4 bgColor = ImVec4(
                ImLerp(offColor.x, onColor.x, t),
                ImLerp(offColor.y, onColor.y, t),
                ImLerp(offColor.z, onColor.z, t),
                ImLerp(offColor.w, onColor.w, t)
            );
            
            if(disabled)
                bgColor = ImVec4(bgColor.x * 0.5f, bgColor.y * 0.5f, bgColor.z * 0.5f, bgColor.w * 0.5f);
            
            ImVec2 trackMin = pos;
            ImVec2 trackMax = ImVec2(pos.x + size.x, pos.y + size.y);
            drawList->AddRectFilled(trackMin, trackMax, ImGui::GetColorU32(bgColor), radius);
            drawList->AddRect(trackMin, trackMax, ImGui::GetColorU32(borderColor), radius, 0, 2.0f);
        
            float knobWidth = size.x * 0.45f;
            float knobHeight = size.y * 0.85f;
            float knobX = ImLerp(pos.x + 2.0f, pos.x + size.x - knobWidth - 2.0f, t);
            
            ImVec2 knobMin = ImVec2(knobX, pos.y + (size.y - knobHeight) * 0.5f);
            ImVec2 knobMax = ImVec2(knobX + knobWidth, knobMin.y + knobHeight);
            
            ImVec4 finalKnobColor = disabled ? ImVec4(knobColor.x * 0.7f, knobColor.y * 0.7f, knobColor.z * 0.7f, knobColor.w) : knobColor;
            drawList->AddRectFilled(knobMin, knobMax, ImGui::GetColorU32(finalKnobColor), 2.0f);
            drawList->AddRect(knobMin, knobMax, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.3f)), 2.0f);
        }

        static void DrawMinimal(ImDrawList* drawList, const ImVec2& pos, const ImVec2& size,
                               bool isOn, float t, const ImVec4& onColor, const ImVec4& offColor,
                               const ImVec4& knobColor, bool disabled)
        {
            float radius = size.y * 0.5f;
            float knobRadius = radius * 0.6f;
            
            ImVec4 bgColor = ImVec4(
                ImLerp(offColor.x, onColor.x, t),
                ImLerp(offColor.y, onColor.y, t),
                ImLerp(offColor.z, onColor.z, t),
                ImLerp(offColor.w, onColor.w, t)
            );
            
            if(disabled)
                bgColor = ImVec4(bgColor.x * 0.5f, bgColor.y * 0.5f, bgColor.z * 0.5f, bgColor.w * 0.5f);
            
            float lineThickness = 2.0f;
            ImVec2 lineStart = ImVec2(pos.x + radius, pos.y + size.y * 0.5f);
            ImVec2 lineEnd = ImVec2(pos.x + size.x - radius, pos.y + size.y * 0.5f);
            drawList->AddLine(lineStart, lineEnd, ImGui::GetColorU32(bgColor), lineThickness);
            
            float knobX = ImLerp(pos.x + radius, pos.x + size.x - radius, t);
            ImVec2 knobCenter = ImVec2(knobX, pos.y + radius);
            
            ImVec4 finalKnobColor = disabled ? ImVec4(knobColor.x * 0.7f, knobColor.y * 0.7f, knobColor.z * 0.7f, knobColor.w) : knobColor;
            drawList->AddCircleFilled(knobCenter, knobRadius, ImGui::GetColorU32(finalKnobColor));
            drawList->AddCircle(knobCenter, knobRadius, ImGui::GetColorU32(bgColor), 0, 2.0f);
        }
    };

    struct ToggleSwitchState
    {
        float Animation{0.0f};
    };

    inline bool ToggleSwitch(const std::string& label, bool* value, const ToggleSwitchConfig& config = {}, const ToggleSwitchCallback& callback = nullptr)
    {
        ScopeID id(label);
        ImVec4 onColor = config.OnColor.w > 0.0f ? config.OnColor : ToggleSwitchTheme::GetOnColor(config.Style);
        ImVec4 offColor = config.OffColor.w > 0.0f ? config.OffColor : ToggleSwitchTheme::GetOffColor(config.Style);
        ImVec4 knobColor = config.KnobColor.w > 0.0f ? config.KnobColor : ToggleSwitchTheme::GetKnobColor(config.Style);
        ImVec4 borderColor = config.BorderColor.w > 0.0f ? config.BorderColor : ToggleSwitchTheme::GetBorderColor(config.Style, *value);
        ImVec2 switchSize = ToggleSwitchTheme::GetSize(config.Size, config.Style);
        
        if(config.ShowLabel)
        {
            ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        }
        
        ImGuiID switchID = ImGui::GetID("##switch");
        ImGuiStorage* storage = ImGui::GetStateStorage();
        float* animPtr = storage->GetFloatRef(switchID, *value ? 1.0f : 0.0f);
        
        if(config.Animated)
        {
            float target = *value ? 1.0f : 0.0f;
            *animPtr = ImLerp(*animPtr, target, config.AnimationSpeed);
        }
        else
        {
            *animPtr = *value ? 1.0f : 0.0f;
        }
        
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        ImGui::InvisibleButton("##toggle", switchSize, ImGuiButtonFlags_None);
        bool hovered = ImGui::IsItemHovered();
        bool clicked = ImGui::IsItemClicked() && !config.Disabled;
        
        if(clicked)
        {
            *value = !*value;
            if(callback)
                callback(*value);
        }
        
        switch(config.Style)
        {
            case ToggleSwitchStyle::iOS:
                ToggleSwitchRenderer::DrawiOS(drawList, pos, switchSize, *value, *animPtr, 
                                             onColor, offColor, knobColor, config.Disabled);
                break;
                
            case ToggleSwitchStyle::Android:
                ToggleSwitchRenderer::DrawAndroid(drawList, pos, switchSize, *value, *animPtr,
                                                  onColor, offColor, knobColor, borderColor, config.Disabled);
                break;
                
            case ToggleSwitchStyle::Modern:
                ToggleSwitchRenderer::DrawModern(drawList, pos, switchSize, *value, *animPtr,
                                                onColor, offColor, knobColor, borderColor, config.Disabled);
                break;
                
            case ToggleSwitchStyle::Retro:
                ToggleSwitchRenderer::DrawRetro(drawList, pos, switchSize, *value, *animPtr,
                                               onColor, offColor, knobColor, borderColor, config.Disabled);
                break;
                
            case ToggleSwitchStyle::Minimal:
                ToggleSwitchRenderer::DrawMinimal(drawList, pos, switchSize, *value, *animPtr,
                                                  onColor, offColor, knobColor, config.Disabled);
                break;
        }
        
        if(hovered && !config.Disabled)
        {
            ImVec2 hoverMin = pos;
            ImVec2 hoverMax = ImVec2(pos.x + switchSize.x, pos.y + switchSize.y);
            drawList->AddRect(hoverMin, hoverMax, ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.2f)), switchSize.y * 0.5f);
        }

        if(config.Disabled)
        {
            ImVec2 overlayMin = pos;
            ImVec2 overlayMax = ImVec2(pos.x + switchSize.x, pos.y + switchSize.y);
            drawList->AddRectFilled(overlayMin, overlayMax, ImGui::GetColorU32(ImVec4(0.0f, 0.0f, 0.0f, 0.1f)), switchSize.y * 0.5f);
        }
        
        return clicked;
    }

    namespace ToggleSwitchPresets
    {
        inline ToggleSwitchConfig iOS()
        {
            ToggleSwitchConfig config;
            config.Style = ToggleSwitchStyle::iOS;
            config.Size = ToggleSwitchSize::Medium;
            config.Animated = true;
            return config;
        }

        inline ToggleSwitchConfig Android()
        {
            ToggleSwitchConfig config;
            config.Style = ToggleSwitchStyle::Android;
            config.Size = ToggleSwitchSize::Medium;
            config.Animated = true;
            return config;
        }

        inline ToggleSwitchConfig Modern()
        {
            ToggleSwitchConfig config;
            config.Style = ToggleSwitchStyle::Modern;
            config.Size = ToggleSwitchSize::Medium;
            config.Animated = true;
            return config;
        }

        inline ToggleSwitchConfig Retro()
        {
            ToggleSwitchConfig config;
            config.Style = ToggleSwitchStyle::Retro;
            config.Size = ToggleSwitchSize::Medium;
            config.Animated = true;
            return config;
        }

        inline ToggleSwitchConfig Minimal()
        {
            ToggleSwitchConfig config;
            config.Style = ToggleSwitchStyle::Minimal;
            config.Size = ToggleSwitchSize::Medium;
            config.Animated = true;
            return config;
        }

        inline ToggleSwitchConfig Small()
        {
            ToggleSwitchConfig config;
            config.Style = ToggleSwitchStyle::iOS;
            config.Size = ToggleSwitchSize::Small;
            config.Animated = true;
            return config;
        }

        inline ToggleSwitchConfig Large()
        {
            ToggleSwitchConfig config;
            config.Style = ToggleSwitchStyle::iOS;
            config.Size = ToggleSwitchSize::Large;
            config.Animated = true;
            return config;
        }

        inline ToggleSwitchConfig CustomColor(const ImVec4& onColor, const ImVec4& offColor)
        {
            ToggleSwitchConfig config;
            config.Style = ToggleSwitchStyle::iOS;
            config.Size = ToggleSwitchSize::Medium;
            config.Animated = true;
            config.OnColor = onColor;
            config.OffColor = offColor;
            return config;
        }
    }
}