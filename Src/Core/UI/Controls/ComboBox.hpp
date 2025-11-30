#pragma once

#include <string>
#include <vector>
#include <functional>
#include <algorithm>

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "Responsive.hpp"
#include "Scope.hpp"

namespace Motion
{
    enum class ComboBoxStyle
    {
        Default,
        Neumorphic,
        Flat,
        Rounded
    };

    struct ComboBoxConfig
    {
        ComboBoxStyle Style{ComboBoxStyle::Default};
        
        bool Searchable{false};
        bool Disabled{false};
        bool ShowPreview{true};
        bool AllowClear{false};
        bool HeightInItems{true};  // If true, PopupHeight is in items, else pixels
        
        int PopupHeight{8};  // Number of items or pixels depending on HeightInItems
        
        const char* Placeholder{"Select..."};
        const char* Tooltip{nullptr};
        const char* EmptyText{"No items available"};
        
        ResponsiveLayout::Options Layout{};
        
        ImGuiComboFlags GetFlags() const
        {
            ImGuiComboFlags flags = ImGuiComboFlags_None;
            
            if (!ShowPreview)
                flags |= ImGuiComboFlags_NoPreview;
            
            if (HeightInItems)
            {
                switch (PopupHeight)
                {
                    case 4:  flags |= ImGuiComboFlags_HeightSmall; break;
                    case 8:  flags |= ImGuiComboFlags_HeightRegular; break;
                    case 20: flags |= ImGuiComboFlags_HeightLarge; break;
                    default: flags |= ImGuiComboFlags_HeightRegular; break;
                }
            }
            else
            {
                flags |= ImGuiComboFlags_HeightRegular;
            }
            
            return flags;
        }
    };

    template<typename T>
    using ComboBoxCallback = std::function<void(const T&, int)>;

    namespace ComboBoxInternal
    {
        inline void ApplyStyle(ComboBoxStyle style, bool& wasStylePushed)
        {
            wasStylePushed = false;
            
            switch (style)
            {
                case ComboBoxStyle::Neumorphic:
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
                    wasStylePushed = true;
                    break;
                }
                
                case ComboBoxStyle::Rounded:
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);
                    wasStylePushed = true;
                    break;
                }
                
                case ComboBoxStyle::Flat:
                {
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
                    wasStylePushed = true;
                    break;
                }
                
                default:
                    break;
            }
        }

        inline void PopStyle(ComboBoxStyle style, bool wasStylePushed)
        {
            if (!wasStylePushed)
                return;
                
            switch (style)
            {
                case ComboBoxStyle::Neumorphic:
                    ImGui::PopStyleColor(4);
                    ImGui::PopStyleVar(2);
                    break;
                    
                case ComboBoxStyle::Rounded:
                case ComboBoxStyle::Flat:
                    ImGui::PopStyleVar(2);
                    break;
                    
                default:
                    break;
            }
        }

        inline bool FilterItems(const std::string& item, const std::string& searchQuery)
        {
            if (searchQuery.empty())
                return true;

            std::string lowerItem = item;
            std::string lowerQuery = searchQuery;
            
            std::transform(lowerItem.begin(), lowerItem.end(), lowerItem.begin(), ::tolower);
            std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);
            
            return lowerItem.find(lowerQuery) != std::string::npos;
        }
    }

    inline bool ComboBox(const std::string& label, int& currentIndex, const std::vector<std::string>& items, 
                        const ComboBoxConfig& config = {}, const ComboBoxCallback<std::string>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = false;
        bool wasStylePushed = false;
        
        ComboBoxInternal::ApplyStyle(config.Style, wasStylePushed);
        
        // Get preview text
        const char* preview = config.Placeholder;
        if (currentIndex >= 0 && currentIndex < static_cast<int>(items.size()))
        {
            preview = items[currentIndex].c_str();
        }
        
        // Disable if needed
        if (config.Disabled)
        {
            ImGui::BeginDisabled();
        }
        
        static std::string searchQuery;
        
        if (ImGui::BeginCombo("##combo", preview, config.GetFlags()))
        {
            // Searchable combo box
            if (config.Searchable)
            {
                ImGui::SetKeyboardFocusHere();
                ImGui::InputTextWithHint("##search", "Search...", &searchQuery);
                ImGui::Separator();
            }
            
            if (items.empty())
            {
                ImGui::TextDisabled("%s", config.EmptyText);
            }
            else
            {
                // Clear button
                if (config.AllowClear && currentIndex >= 0)
                {
                    if (ImGui::Selectable("(Clear Selection)", false))
                    {
                        currentIndex = -1;
                        changed = true;
                        searchQuery.clear();
                    }
                    ImGui::Separator();
                }
                
                // Render items
                for (int i = 0; i < static_cast<int>(items.size()); i++)
                {
                    // Filter based on search
                    if (config.Searchable && !ComboBoxInternal::FilterItems(items[i], searchQuery))
                        continue;
                    
                    const bool isSelected = (currentIndex == i);
                    
                    if (ImGui::Selectable(items[i].c_str(), isSelected))
                    {
                        currentIndex = i;
                        changed = true;
                        searchQuery.clear();
                    }
                    
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
            }
            
            ImGui::EndCombo();
        }
        else
        {
            // Clear search when combo is closed
            searchQuery.clear();
        }
        
        if (config.Disabled)
        {
            ImGui::EndDisabled();
        }
        
        ComboBoxInternal::PopStyle(config.Style, wasStylePushed);
        
        // Tooltip
        if (config.Tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        
        // Right-click context menu
        if (!config.Disabled && ImGui::IsItemClicked(ImGuiMouseButton_Right))
            ImGui::OpenPopup("combo_context");

        if (ImGui::BeginPopup("combo_context"))
        {
            if (currentIndex >= 0 && currentIndex < static_cast<int>(items.size()))
            {
                if (ImGui::MenuItem("Copy Selected"))
                {
                    ImGui::SetClipboardText(items[currentIndex].c_str());
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::Separator();
            }
            
            if (config.AllowClear && currentIndex >= 0)
            {
                if (ImGui::MenuItem("Clear Selection"))
                {
                    currentIndex = -1;
                    changed = true;
                    ImGui::CloseCurrentPopup();
                }
                
                ImGui::Separator();
            }
            
            ImGui::TextDisabled("Items: %zu", items.size());
            if (currentIndex >= 0)
                ImGui::TextDisabled("Selected: %d", currentIndex);

            ImGui::EndPopup();
        }
        
        if (changed && callback && currentIndex >= 0 && currentIndex < static_cast<int>(items.size()))
            callback(items[currentIndex], currentIndex);
        
        return changed;
    }

    // ComboBox with enum support
    template<typename EnumType>
    inline bool ComboBoxEnum(const std::string& label, EnumType& currentValue, 
                            const std::vector<std::pair<EnumType, std::string>>& items,
                            const ComboBoxConfig& config = {}, 
                            const ComboBoxCallback<EnumType>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = false;
        bool wasStylePushed = false;
        
        ComboBoxInternal::ApplyStyle(config.Style, wasStylePushed);
        
        // Find current item
        int currentIndex = -1;
        const char* preview = config.Placeholder;
        
        for (size_t i = 0; i < items.size(); i++)
        {
            if (items[i].first == currentValue)
            {
                currentIndex = static_cast<int>(i);
                preview = items[i].second.c_str();
                break;
            }
        }
        
        if (config.Disabled)
        {
            ImGui::BeginDisabled();
        }
        
        if (ImGui::BeginCombo("##combo", preview, config.GetFlags()))
        {
            if (items.empty())
            {
                ImGui::TextDisabled("%s", config.EmptyText);
            }
            else
            {
                for (size_t i = 0; i < items.size(); i++)
                {
                    const bool isSelected = (currentIndex == static_cast<int>(i));
                    
                    if (ImGui::Selectable(items[i].second.c_str(), isSelected))
                    {
                        currentValue = items[i].first;
                        currentIndex = static_cast<int>(i);
                        changed = true;
                    }
                    
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
            }
            
            ImGui::EndCombo();
        }
        
        if (config.Disabled)
        {
            ImGui::EndDisabled();
        }
        
        ComboBoxInternal::PopStyle(config.Style, wasStylePushed);
        
        if (config.Tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        
        if (changed && callback)
            callback(currentValue, currentIndex);
        
        return changed;
    }

    // Multi-selection ComboBox
    struct MultiComboBoxConfig : public ComboBoxConfig
    {
        const char* MultiSelectFormat{"%d selected"};
        int MaxVisibleSelections{3};
    };

    inline bool MultiComboBox(const std::string& label, std::vector<bool>& selected, 
                             const std::vector<std::string>& items,
                             const MultiComboBoxConfig& config = {})
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = false;
        bool wasStylePushed = false;
        
        // Ensure selected vector matches items size
        if (selected.size() != items.size())
        {
            selected.resize(items.size(), false);
        }
        
        ComboBoxInternal::ApplyStyle(config.Style, wasStylePushed);
        
        // Build preview text
        std::string preview;
        int selectedCount = 0;
        for (size_t i = 0; i < selected.size(); i++)
        {
            if (selected[i])
            {
                selectedCount++;
                if (selectedCount <= config.MaxVisibleSelections)
                {
                    if (!preview.empty())
                        preview += ", ";
                    preview += items[i];
                }
            }
        }
        
        if (selectedCount == 0)
        {
            preview = config.Placeholder;
        }
        else if (selectedCount > config.MaxVisibleSelections)
        {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), config.MultiSelectFormat, selectedCount);
            preview = buffer;
        }
        
        if (config.Disabled)
        {
            ImGui::BeginDisabled();
        }
        
        static std::string searchQuery;
        
        if (ImGui::BeginCombo("##combo", preview.c_str(), config.GetFlags()))
        {
            if (config.Searchable)
            {
                ImGui::SetKeyboardFocusHere();
                ImGui::InputTextWithHint("##search", "Search...", &searchQuery);
                ImGui::Separator();
            }
            
            if (items.empty())
            {
                ImGui::TextDisabled("%s", config.EmptyText);
            }
            else
            {
                // Select All / Clear All buttons
                if (ImGui::Button("Select All", ImVec2(ImGui::GetContentRegionAvail().x * 0.48f, 0)))
                {
                    for (size_t i = 0; i < selected.size(); i++)
                        selected[i] = true;
                    changed = true;
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Clear All", ImVec2(-1, 0)))
                {
                    for (size_t i = 0; i < selected.size(); i++)
                        selected[i] = false;
                    changed = true;
                }
                
                ImGui::Separator();
                
                // Render items
                for (size_t i = 0; i < items.size(); i++)
                {
                    if (config.Searchable && !ComboBoxInternal::FilterItems(items[i], searchQuery))
                        continue;
                    
                    bool itemSelected = selected[i];
                    if (ImGui::Checkbox(items[i].c_str(), &itemSelected))
                    {
                        selected[i] = itemSelected;
                        changed = true;
                    }
                }
            }
            
            ImGui::EndCombo();
        }
        else
        {
            searchQuery.clear();
        }
        
        if (config.Disabled)
        {
            ImGui::EndDisabled();
        }
        
        ComboBoxInternal::PopStyle(config.Style, wasStylePushed);
        
        if (config.Tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        
        return changed;
    }

    // Grouped ComboBox
    struct ComboBoxGroup
    {
        std::string GroupName;
        std::vector<std::string> Items;
    };

    inline bool GroupedComboBox(const std::string& label, int& currentIndex, 
                               const std::vector<ComboBoxGroup>& groups,
                               const ComboBoxConfig& config = {}, 
                               const ComboBoxCallback<std::string>& callback = nullptr)
    {
        ScopeID id(label);
        ResponsiveLayout::BeginLabelControl(label.c_str(), config.Layout);
        
        bool changed = false;
        bool wasStylePushed = false;
        
        // Build flat item list with group tracking
        std::vector<std::pair<int, int>> itemMapping; // group index, item index
        std::vector<std::string> flatItems;
        int totalItems = 0;
        
        for (size_t g = 0; g < groups.size(); g++)
        {
            for (size_t i = 0; i < groups[g].Items.size(); i++)
            {
                itemMapping.push_back({static_cast<int>(g), static_cast<int>(i)});
                flatItems.push_back(groups[g].Items[i]);
                totalItems++;
            }
        }
        
        ComboBoxInternal::ApplyStyle(config.Style, wasStylePushed);
        
        // Get preview text
        const char* preview = config.Placeholder;
        if (currentIndex >= 0 && currentIndex < totalItems)
        {
            preview = flatItems[currentIndex].c_str();
        }
        
        if (config.Disabled)
        {
            ImGui::BeginDisabled();
        }
        
        if (ImGui::BeginCombo("##combo", preview, config.GetFlags()))
        {
            if (groups.empty())
            {
                ImGui::TextDisabled("%s", config.EmptyText);
            }
            else
            {
                int flatIndex = 0;
                for (const auto& group : groups)
                {
                    if (!group.GroupName.empty())
                    {
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", group.GroupName.c_str());
                        ImGui::Separator();
                    }
                    
                    for (const auto& item : group.Items)
                    {
                        const bool isSelected = (currentIndex == flatIndex);
                        
                        if (ImGui::Selectable(("  " + item).c_str(), isSelected))
                        {
                            currentIndex = flatIndex;
                            changed = true;
                        }
                        
                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                        
                        flatIndex++;
                    }
                    
                    ImGui::Spacing();
                }
            }
            
            ImGui::EndCombo();
        }
        
        if (config.Disabled)
        {
            ImGui::EndDisabled();
        }
        
        ComboBoxInternal::PopStyle(config.Style, wasStylePushed);
        
        if (config.Tooltip && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
        {
            ImGui::SetTooltip("%s", config.Tooltip);
        }
        
        if (changed && callback && currentIndex >= 0 && currentIndex < totalItems)
            callback(flatItems[currentIndex], currentIndex);
        
        return changed;
    }
}