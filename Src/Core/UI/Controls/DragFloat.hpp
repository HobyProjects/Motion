#pragma once

#include <string>
#include <functional>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Responsive.hpp"
#include "Scope.hpp"

namespace Motion
{
    struct DragFloatConfig
    {
        float Speed{0.1f};
        float MinV{-FLT_MAX};
        float MaxV{FLT_MAX};
        float ResetValue{0.0f};
        const char* Fmt{"%.3f"};
        const char* Tooltip{nullptr};
        ResponsiveLayout::Options Layout{};
    };

    template<typename T>
    using DragFloatCallback = std::function<void(const T&)>;

    inline void DragFloat(const std::string& label, float* v, const DragFloatConfig& config = {}, const DragFloatCallback<float>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::DragFloat("##value", v, config.Speed, config.MinV, config.MaxV, config.Fmt, ImGuiSliderFlags_AlwaysClamp);
        if(ImGui::IsItemHovered() && config.Tooltip)
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }

        if(changed && callback) 
            callback(*v);
    }

    inline void DragFloat2(const std::string& label, glm::vec2& v, const DragFloatConfig& config = {}, const DragFloatCallback<glm::vec2>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::DragFloat2("##value", glm::value_ptr(v), config.Speed, config.MinV, config.MaxV, config.Fmt, ImGuiSliderFlags_AlwaysClamp);
        if(ImGui::IsItemHovered() && config.Tooltip)
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }

        if(ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("context");

        if(ImGui::BeginPopup("context"))
        {
            ImGui::TextDisabled("X: %.3f", v.x);
            ImGui::SameLine();
            if(ImGui::Button("Copy##x", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.x).c_str());
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::TextDisabled("Y: %.3f", v.y);
            ImGui::SameLine();
            if(ImGui::Button("Copy##y", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.y).c_str());
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();
            
            if(ImGui::MenuItem("Reset"))
            {
                v = glm::vec2(config.ResetValue, config.ResetValue);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        
        if(changed && callback) 
            callback(v);
    }

    inline void DragFloat3(const std::string& label, glm::vec3& v, const DragFloatConfig& config = {}, const DragFloatCallback<glm::vec3>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::DragFloat3("##value", glm::value_ptr(v), config.Speed, config.MinV, config.MaxV, config.Fmt, ImGuiSliderFlags_AlwaysClamp);
        if(ImGui::IsItemHovered() && config.Tooltip)
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }

        if(ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("context");

        if(ImGui::BeginPopup("context"))
        {
            ImGui::TextDisabled("X: %.3f", v.x);
            ImGui::SameLine();
            if(ImGui::Button("Copy##x", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.x).c_str());
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::TextDisabled("Y: %.3f", v.y);
            ImGui::SameLine();
            if(ImGui::Button("Copy##y", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.y).c_str());
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::TextDisabled("Z: %.3f", v.z);
            ImGui::SameLine();
            if(ImGui::Button("Copy##z", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.z).c_str());
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();
            
            if(ImGui::MenuItem("Reset"))
            {
                v = glm::vec3(config.ResetValue, config.ResetValue, config.ResetValue);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        
        if(changed && callback) 
            callback(v);
    }
}