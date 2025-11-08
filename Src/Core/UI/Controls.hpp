#pragma once

#include <string>
#include <vector>
#include <functional>
#include <algorithm>
#include <atomic>
#include <memory>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/pattern_formatter.h>

#include "Texture.hpp"

// Macro to simplify callback binding in UI code
#define MOTION_UI_TEXTURESLOT_CALLBACK(CALLBACK_FUNC) \
    [this](auto&&... args) -> decltype(auto) { \
        return this->CALLBACK_FUNC(std::forward<decltype(args)>(args)...); \
    }

namespace Motion
{
    // ============================================================================
    // Utility Functions
    // ============================================================================
    
    /// Generate unique IDs for ImGui widgets
    inline std::uint32_t GetUID()
    {
        static std::atomic_uint32_t g_counter{ 1 };
        return g_counter.fetch_add(1, std::memory_order_relaxed);
    }

    // ============================================================================
    // RAII Scope Helpers
    // ============================================================================
    
    /// RAII wrapper for ImGui::PushID/PopID
    struct ScopeID
    {
        explicit ScopeID(std::uint32_t id) { ImGui::PushID(static_cast<int>(id)); }
        explicit ScopeID(const char* id) { ImGui::PushID(id); }
        ~ScopeID() { ImGui::PopID(); }
        
        ScopeID(const ScopeID&) = delete;
        ScopeID& operator=(const ScopeID&) = delete;
    };

    /// RAII wrapper for ImGui::PushStyleVar/PopStyleVar
    struct StyleVar
    {
        explicit StyleVar(ImGuiStyleVar var, float v) { ImGui::PushStyleVar(var, v); }
        explicit StyleVar(ImGuiStyleVar var, const ImVec2& v) { ImGui::PushStyleVar(var, v); }
        ~StyleVar() { ImGui::PopStyleVar(); }
        
        StyleVar(const StyleVar&) = delete;
        StyleVar& operator=(const StyleVar&) = delete;
    };

    /// RAII wrapper for ImGui::PushStyleColor/PopStyleColor
    struct StyleColor
    {
        explicit StyleColor(ImGuiCol idx, ImU32 col) { ImGui::PushStyleColor(idx, col); }
        explicit StyleColor(ImGuiCol idx, const ImVec4& col) { ImGui::PushStyleColor(idx, col); }
        ~StyleColor() { ImGui::PopStyleColor(); }
        
        StyleColor(const StyleColor&) = delete;
        StyleColor& operator=(const StyleColor&) = delete;
    };

    /// RAII wrapper for ImGui::BeginDisabled/EndDisabled
    struct DisableScope
    {
        explicit DisableScope(bool disabled) : disabled_(disabled)
        {
            if (disabled_) 
                ImGui::BeginDisabled();
        }
        
        ~DisableScope() 
        { 
            if (disabled_) 
                ImGui::EndDisabled(); 
        }
        
        DisableScope(const DisableScope&) = delete;
        DisableScope& operator=(const DisableScope&) = delete;
        
    private:
        bool disabled_;
    };

    // ============================================================================
    // Property Grid System
    // ============================================================================
    
    /// Configuration for property grid layout
    struct GridSpec
    {
        float labelWidth = 200.0f;
        float innerSpacing = 0.0f;
        bool  twoColumns = true;
    };

    /// Begin a property grid table for label-value pairs
    inline bool BeginPropertyGrid(const char* id, const GridSpec& spec = {})
    {
        ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchProp | 
                               ImGuiTableFlags_BordersInnerV;
        
        if (!ImGui::BeginTable(id, spec.twoColumns ? 2 : 1, flags)) 
            return false;
        
        if (spec.twoColumns) 
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, spec.labelWidth);
            ImGui::TableSetupColumn("controls", ImGuiTableColumnFlags_WidthStretch);
        }
        return true;
    }

    /// End the property grid table
    inline void EndPropertyGrid() 
    { 
        ImGui::EndTable(); 
    }

    /// Display a property label with proper alignment
    inline void PropertyLabel(std::string_view label)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label.data(), label.data() + label.size());
    }

    /// Move to the next row in the property grid
    inline void NextPropertyRow()
    {
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
    }

    /// Display a help marker (?) with tooltip
    inline void HelpMarker(const char* desc) 
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) 
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 42.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    /// Display a simple message box modal
    inline void ShowMessageBox(const char* title, const std::string& message)
    {
        ImGui::OpenPopup(title);
        if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextWrapped("%s", message.c_str());
            ImGui::Dummy(ImVec2(0, 6));
            if (ImGui::Button("OK", ImVec2(80, 0)))
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }
    }

    // ============================================================================
    // Property Controls
    // ============================================================================
    
    /// Drag float control with label
    bool DragFloat(const char* label, float* v, 
                   float speed = 0.1f, 
                   float minV = -FLT_MAX, 
                   float maxV = FLT_MAX, 
                   const char* fmt = "%.3f");
    
    /// Drag vec2 control with label
    bool DragFloat2(const char* label, glm::vec2& v, 
                    float speed = 0.1f, 
                    float minV = -FLT_MAX, 
                    float maxV = FLT_MAX, 
                    const char* fmt = "%.3f");
    
    /// Drag vec3 control with label
    bool DragFloat3(const char* label, glm::vec3& v, 
                    float speed = 0.1f, 
                    float minV = -FLT_MAX, 
                    float maxV = FLT_MAX, 
                    const char* fmt = "%.3f");

    /// Slider control with label
    bool SliderFloat(const char* label, float* v, 
                     float minV, float maxV, 
                     const char* fmt = "%.3f");
    
    /// Color picker for vec4 (RGBA)
    bool ColorEdit4(const char* label, glm::vec4& color, 
                    bool withAlpha = true);
    
    /// Color picker for vec3 (RGB)
    bool ColorEdit3(const char* label, glm::vec3& color);

    /// Text input box with optional read-only mode
    bool TextBox(const char* label, std::string& value, 
                 bool readOnly = false, 
                 size_t maxLen = 1024);
    
    /// Toggle switch control (alternative to checkbox)
    bool ToggleSwitch(const char* label, bool& value);
    
    /// Search input box with hint text
    bool SearchBox(const char* id, std::string& query, 
                   const char* hint = "Search...");
    
    /// Combo box with callback on selection change
    bool ComboBox(const char* label, 
                  const std::vector<std::string>& options, 
                  int& index, 
                  std::function<void(std::int32_t, const std::string&)> onChanged = nullptr);
    
    /// Collapsible section header
    bool CollapsibleSection(const char* label, bool defaultOpen = true);

    // ============================================================================
    // Texture Slot Control
    // ============================================================================
    
    /// Actions that can occur in a texture slot
    enum class TextureSlotAction : int 
    { 
        Upload = 0,  ///< New texture uploaded
        Reload,      ///< Texture reloaded from file
        Clear,       ///< Texture cleared/removed
        None         ///< No action occurred
    };
    
    /// Interactive texture slot with preview, drag-drop, and file dialog support
    /// @param label Unique identifier for the slot
    /// @param tex Shared pointer to texture (may be modified)
    /// @param type Expected texture type
    /// @param onAction Callback invoked when texture changes
    /// @param showLabelAbove Whether to show label above the slot
    /// @param previewSize Size of the preview thumbnail in pixels
    /// @return Action that occurred during this frame
    TextureSlotAction TextureSlot(const char* label, 
                                 std::shared_ptr<ITexture>& tex, 
                                 TextureType type, 
                                 std::function<void(TextureSlotAction, std::shared_ptr<ITexture>&)> onAction = nullptr, 
                                 bool showLabelAbove = false, 
                                 int previewSize = 150);
    
    /// Draw an empty checkered pattern for texture slots
    void EmptyTextureSlot(ImDrawList* dl, const ImRect& r, float cell = 10.0f);

    // ============================================================================
    // Toolbar Helpers
    // ============================================================================
    
    /// Begin a top bar with custom styling
    inline bool BeginTopBar(float height, ImU32 bg_col, float rounding = 0.0f, float pad_x = 8.0f)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(8, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(8, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.y < height) 
            height = avail.y;

        bool opened = ImGui::BeginChild("##TopBarChild", ImVec2(0, height), false,
                                        ImGuiWindowFlags_NoScrollbar | 
                                        ImGuiWindowFlags_NoScrollWithMouse);
        
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowPos();
        ImVec2 p1 = ImVec2(p0.x + ImGui::GetWindowSize().x, p0.y + height);
        dl->AddRectFilled(p0, p1, bg_col, rounding);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad_x);

        return opened;
    }

    /// End the top bar
    inline void EndTopBar()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar(4);
    }

    /// Create a dropdown button in the top bar
    template <typename DrawContent>
    inline bool TopBarDropdown(const char* id, const char* label, DrawContent&& draw)
    {
        bool pressed = false;
        
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.05f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1, 1, 1, 0.08f));
        
        if (ImGui::Button(label)) 
        { 
            ImGui::OpenPopup(id); 
            pressed = true; 
        }
        
        ImGui::PopStyleColor(3);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f, 0.08f, 0.10f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.18f, 0.18f, 0.22f, 1.0f));

        bool open = ImGui::BeginPopup(id);
        if (open)
        {
            draw();
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);

        return pressed;
    }
}