#pragma once

#include <string>
#include <functional>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Responsive.hpp"
#include "Scope.hpp"

namespace Motion
{
    struct ColorEditConfig
    {
        ImGuiColorEditFlags Flags{ImGuiColorEditFlags_None};
        ResponsiveLayout::Options Layout{};
    };

    template<typename T>
    using ColorEditCallback = std::function<void(const T&)>;

    inline void ColorEdit3(const std::string& label, glm::vec3& v, const ColorEditConfig& config = {}, const ColorEditCallback<glm::vec3>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::ColorEdit3("##value", glm::value_ptr(v), config.Flags);
        
        if(ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("context");

        if(ImGui::BeginPopup("context"))
        {
            ImGui::TextDisabled("R: %.3f", v.x);
            ImGui::SameLine();
            if(ImGui::Button("Copy##r", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.x).c_str());
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::TextDisabled("G: %.3f", v.y);
            ImGui::SameLine();
            if(ImGui::Button("Copy##g", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.y).c_str());
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::TextDisabled("B: %.3f", v.z);
            ImGui::SameLine();
            if(ImGui::Button("Copy##b", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.z).c_str());
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();
            
            if(ImGui::MenuItem("Copy Hex"))
            {
                int r = static_cast<int>(v.x * 255.0f);
                int g = static_cast<int>(v.y * 255.0f);
                int b = static_cast<int>(v.z * 255.0f);
                char hex[8];
                snprintf(hex, sizeof(hex), "#%02X%02X%02X", r, g, b);
                ImGui::SetClipboardText(hex);
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();
            
            if(ImGui::MenuItem("Reset to White"))
            {
                v = glm::vec3(1.0f);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            if(ImGui::MenuItem("Reset to Black"))
            {
                v = glm::vec3(0.0f);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        
        if(changed && callback) 
            callback(v);
    }

    inline void ColorEdit4(const std::string& label, glm::vec4& v, const ColorEditConfig& config = {}, const ColorEditCallback<glm::vec4>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::ColorEdit4("##value", glm::value_ptr(v), config.Flags);
        
        if(ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("context");

        if(ImGui::BeginPopup("context"))
        {
            ImGui::TextDisabled("R: %.3f", v.x);
            ImGui::SameLine();
            if(ImGui::Button("Copy##r", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.x).c_str());
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::TextDisabled("G: %.3f", v.y);
            ImGui::SameLine();
            if(ImGui::Button("Copy##g", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.y).c_str());
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::TextDisabled("B: %.3f", v.z);
            ImGui::SameLine();
            if(ImGui::Button("Copy##b", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.z).c_str());
                ImGui::CloseCurrentPopup();
            }

            ImGui::TextDisabled("A: %.3f", v.w);
            ImGui::SameLine();
            if(ImGui::Button("Copy##a", ImVec2(50, 0)))
            {
                ImGui::SetClipboardText(std::to_string(v.w).c_str());
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();
            
            if(ImGui::MenuItem("Copy Hex (RGBA)"))
            {
                int r = static_cast<int>(v.x * 255.0f);
                int g = static_cast<int>(v.y * 255.0f);
                int b = static_cast<int>(v.z * 255.0f);
                int a = static_cast<int>(v.w * 255.0f);
                char hex[10];
                snprintf(hex, sizeof(hex), "#%02X%02X%02X%02X", r, g, b, a);
                ImGui::SetClipboardText(hex);
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();
            
            if(ImGui::MenuItem("Reset to White"))
            {
                v = glm::vec4(1.0f);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            if(ImGui::MenuItem("Reset to Black"))
            {
                v = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            if(ImGui::MenuItem("Reset to Transparent"))
            {
                v = glm::vec4(0.0f);
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
        
        if(changed && callback) 
            callback(v);
    }

    inline void ColorEdit4(const std::string& label, ImVec4& v, const ColorEditConfig& config = {}, const ColorEditCallback<glm::vec4>& callback = nullptr)
    {
        glm::vec4 glmColor(v.x, v.y, v.z, v.w);
        ColorEdit4(label, glmColor, config, [&](const glm::vec4& newColor)
        {
            v.x = newColor.x;
            v.y = newColor.y;
            v.z = newColor.z;
            v.w = newColor.w;
            if(callback)
                callback(glmColor);
        });
    }

    inline void ColorEdit4(const std::string& label, ImU32& color, const ColorEditConfig& config = {}, const ColorEditCallback<glm::vec4>& callback = nullptr)
    {
        glm::vec4 glmColor;
        glmColor.r = static_cast<float>((color >> IM_COL32_R_SHIFT) & 0xFF) / 255.0f;
        glmColor.g = static_cast<float>((color >> IM_COL32_G_SHIFT) & 0xFF) / 255.0f;
        glmColor.b = static_cast<float>((color >> IM_COL32_B_SHIFT) & 0xFF) / 255.0f;
        glmColor.a = static_cast<float>((color >> IM_COL32_A_SHIFT) & 0xFF) / 255.0f;

        ColorEdit4(label, glmColor, config, [&](const glm::vec4& newColor)
        {
            color = IM_COL32(
                static_cast<ImU32>(newColor.r * 255.0f),
                static_cast<ImU32>(newColor.g * 255.0f),
                static_cast<ImU32>(newColor.b * 255.0f),
                static_cast<ImU32>(newColor.a * 255.0f)
            );
            
            if(callback)
                callback(glmColor);
        });
    }
}