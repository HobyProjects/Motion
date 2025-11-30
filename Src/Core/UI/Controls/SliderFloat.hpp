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
    struct SliderFloatConfig
    {
        float MinV{0.0f};
        float MaxV{1.0f};
        const char* Fmt{"%.3f"};
        const char* Tooltip{nullptr};
        ResponsiveLayout::Options Layout{};
    };

    template<typename T>
    using SliderFloatCallback = std::function<void(const T&)>;

    inline void SliderFloat(const std::string& label, float* v, const SliderFloatConfig& config = {}, const SliderFloatCallback<float>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::SliderFloat("##value", v, config.MinV, config.MaxV, config.Fmt, ImGuiSliderFlags_AlwaysClamp);
        if(ImGui::IsItemHovered() && config.Tooltip)
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }

        if(changed && callback) 
            callback(*v);
    }

    inline void SliderFloat2(const std::string& label, glm::vec2& v, const SliderFloatConfig& config = {}, const SliderFloatCallback<glm::vec2>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::SliderFloat2("##value", glm::value_ptr(v), config.MinV, config.MaxV, config.Fmt, ImGuiSliderFlags_AlwaysClamp);
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
            
            if(ImGui::MenuItem("Reset to Min"))
            {
                v = glm::vec2(config.MinV);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            if(ImGui::MenuItem("Reset to Max"))
            {
                v = glm::vec2(config.MaxV);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        
        if(changed && callback) 
            callback(v);
    }

    inline void SliderFloat3(const std::string& label, glm::vec3& v, const SliderFloatConfig& config = {}, const SliderFloatCallback<glm::vec3>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::SliderFloat3("##value", glm::value_ptr(v), config.MinV, config.MaxV, config.Fmt, ImGuiSliderFlags_AlwaysClamp);
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
            
            if(ImGui::MenuItem("Reset to Min"))
            {
                v = glm::vec3(config.MinV);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            if(ImGui::MenuItem("Reset to Max"))
            {
                v = glm::vec3(config.MaxV);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        
        if(changed && callback) 
            callback(v);
    }
}