#pragma once

#include <string>
#include <functional>
#include <algorithm>

#include <imgui/imgui.h>
#include <imgui/imgui_stdlib.h>

#include "Responsive.hpp"
#include "Scope.hpp"

namespace Motion
{
    struct TextBoxConfig
    {
        bool ReadOnly{false};
        bool Password{false};
        bool Multiline{false};
        bool AllowTabInput{false};
        bool NoHorizontalScroll{false};
        bool SelectAllOnFocus{false};
        bool EnterReturnsTrue{false}; 
        
        bool NumbersOnly{false};
        bool HexadecimalOnly{false};
        bool UpperCaseOnly{false};
        bool NoSpaces{false};
        
        ResponsiveLayout::Options Layout{};
        
        ImGuiInputTextFlags GetFlags() const
        {
            ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
            
            if(ReadOnly)            flags |= ImGuiInputTextFlags_ReadOnly;
            if(Password)            flags |= ImGuiInputTextFlags_Password;
            if(AllowTabInput)       flags |= ImGuiInputTextFlags_AllowTabInput;
            if(NoHorizontalScroll)  flags |= ImGuiInputTextFlags_NoHorizontalScroll;
            if(SelectAllOnFocus)    flags |= ImGuiInputTextFlags_AutoSelectAll;
            if(EnterReturnsTrue)    flags |= ImGuiInputTextFlags_EnterReturnsTrue;
            
            if(NumbersOnly)             flags |= ImGuiInputTextFlags_CharsDecimal;
            else if(HexadecimalOnly)    flags |= ImGuiInputTextFlags_CharsHexadecimal;
            else if(UpperCaseOnly)      flags |= ImGuiInputTextFlags_CharsUppercase;
            
            if(NoSpaces) flags |= ImGuiInputTextFlags_CharsNoBlank;
            
            return flags;
        }
    };

    using TextBoxCallback = std::function<void(const std::string&)>;

    inline void TextBox(const std::string& label, std::string& text, const TextBoxConfig& config = {}, const TextBoxCallback& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::InputText("##value", &text, config.GetFlags());
        
        if(!config.ReadOnly && ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("context");

        if(ImGui::BeginPopup("context"))
        {
            if(ImGui::MenuItem("Copy", nullptr, false, !text.empty()))
            {
                ImGui::SetClipboardText(text.c_str());
                ImGui::CloseCurrentPopup();
            }

            if(ImGui::MenuItem("Paste"))
            {
                const char* clipboard = ImGui::GetClipboardText();
                if(clipboard)
                {
                    text = clipboard;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Clear", nullptr, false, !text.empty()))
            {
                text.clear();
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();

            ImGui::TextDisabled("Length: %zu", text.length());

            ImGui::EndPopup();
        }
        
        if(changed && callback)
            callback(text);
    }

    inline void TextBoxMultiline(const std::string& label, std::string& text, const ImVec2& size = ImVec2(0, 0), const TextBoxConfig& config = {}, const TextBoxCallback& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::InputTextMultiline("##value", &text, size, config.GetFlags());
        
        // Right-click context menu (disabled for read-only)
        if(!config.ReadOnly && ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("context");

        if(ImGui::BeginPopup("context"))
        {
            if(ImGui::MenuItem("Copy", nullptr, false, !text.empty()))
            {
                ImGui::SetClipboardText(text.c_str());
                ImGui::CloseCurrentPopup();
            }

            if(ImGui::MenuItem("Paste"))
            {
                const char* clipboard = ImGui::GetClipboardText();
                if(clipboard)
                {
                    text = clipboard;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Clear", nullptr, false, !text.empty()))
            {
                text.clear();
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();

            ImGui::TextDisabled("Length: %zu", text.length());
            ImGui::TextDisabled("Lines: %d", static_cast<int>(std::count(text.begin(), text.end(), '\n')) + 1);

            ImGui::EndPopup();
        }
        
        if(changed && callback)
            callback(text);
    }

    inline void TextBoxWithHint(const std::string& label, std::string& text, const std::string& hint, const TextBoxConfig& config = {}, const TextBoxCallback& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = ImGui::InputTextWithHint("##value", hint.c_str(), &text, config.GetFlags());
        
        // Right-click context menu (disabled for read-only)
        if(!config.ReadOnly && ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("context");

        if(ImGui::BeginPopup("context"))
        {
            if(ImGui::MenuItem("Copy", nullptr, false, !text.empty()))
            {
                ImGui::SetClipboardText(text.c_str());
                ImGui::CloseCurrentPopup();
            }

            if(ImGui::MenuItem("Paste"))
            {
                const char* clipboard = ImGui::GetClipboardText();
                if(clipboard)
                {
                    text = clipboard;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::Separator();

            if(ImGui::MenuItem("Clear", nullptr, false, !text.empty()))
            {
                text.clear();
                changed = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();

            ImGui::TextDisabled("Length: %zu", text.length());

            ImGui::EndPopup();
        }
        
        if(changed && callback)
            callback(text);
    }
}