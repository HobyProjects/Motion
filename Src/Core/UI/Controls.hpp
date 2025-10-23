#pragma once

#include <string>
#include <vector>
#include <functional>
#include <algorithm>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/pattern_formatter.h>

#include "Texture.hpp"

#define MOTION_UI_TEXTURESLOT_CALLBACK(CALLBACK_FUNC) [this](auto&&... args) -> decltype(auto) { return this->CALLBACK_FUNC(std::forward<decltype(args)>(args)...); }
namespace Motion
{
    inline std::uint32_t GetUID()
    {
        static std::atomic_uint32_t g{ 1 };
        return g.fetch_add(1, std::memory_order_relaxed);
    }

    struct ScopeID
    {
        explicit ScopeID(std::uint32_t id) { ImGui::PushID(static_cast<int>(id)); }
        explicit ScopeID(const char* id) { ImGui::PushID(id); }
        ~ScopeID() { ImGui::PopID(); }
    };

    struct StyleVar
    {
        StyleVar(ImGuiStyleVar var, float v) { ImGui::PushStyleVar(var, v); }
        StyleVar(ImGuiStyleVar var, const ImVec2& v) { ImGui::PushStyleVar(var, v); }
        ~StyleVar() { ImGui::PopStyleVar(); }
    };

    struct StyleColor
    {
        StyleColor(ImGuiCol idx, ImU32 col) { ImGui::PushStyleColor(idx, col); }
        StyleColor(ImGuiCol idx, const ImVec4& col) { ImGui::PushStyleColor(idx, col); }
        ~StyleColor() { ImGui::PopStyleColor(); }
    };

    struct DisableScope
    {
        explicit DisableScope(bool disabled) : disabled_(disabled)
        {
            if (disabled_) ImGui::BeginDisabled();
        }
        ~DisableScope() { if (disabled_) ImGui::EndDisabled(); }
        bool disabled_;
    };

    struct GridSpec
    {
        float labelWidth = 160.0f;
        float innerSpacing = 0.0f;
        bool  twoColumns = true;
    };

    inline bool BeginPropertyGrid(const char* id, const GridSpec& spec = {})
    {
        ImGuiTableFlags flags = ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerV;
        if (!ImGui::BeginTable(id, spec.twoColumns ? 2 : 1, flags)) return false;
        if (spec.twoColumns) 
        {
            ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, spec.labelWidth);
            ImGui::TableSetupColumn("controls", ImGuiTableColumnFlags_WidthStretch);
        }
        return true;
    }

    inline void EndPropertyGrid() { ImGui::EndTable(); }

    inline void PropertyLabel(std::string_view label)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label.data());
    }

    inline void NextPropertyRow()
    {
        ImGui::TableNextColumn();
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
    }

    inline void HelpMarker(const char* desc) 
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) 
        {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 42.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

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


    bool DragFloat(const char* label,  float* v, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");
    bool DragFloat2(const char* label, glm::vec2&  v, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");
    bool DragFloat3(const char* label, glm::vec3&  v, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");

    bool SliderFloat(const char* label, float* v, float minV, float maxV, const char* fmt = "%.3f");
    bool ColorEdit4(const char* label, glm::vec4& color, bool withAlpha = true);
    bool ColorEdit3(const char* label, glm::vec3& color);

    bool TextBox(const char* label, std::string& value, bool readOnly = false, size_t maxLen = 1024);
    bool ToggleSwitch(const char* label, bool& value);
    bool SearchBox(const char* id, std::string& query, const char* hint = "Search...");
    bool ComboBox(const char* label, const std::vector<std::string>& options, int& index, std::function<void(std::int32_t, const std::string&)> onChanged = nullptr);
    bool CollapsibleSection(const char* label, bool defaultOpen = true);

    enum class TextureSlotAction : int { Upload = 0, Reload, Clear, None };
    TextureSlotAction TextureSlot(const char* label, std::shared_ptr<ITexture>& tex, TextureType type, std::function<void(TextureSlotAction, std::shared_ptr<ITexture>&)> onAction = nullptr, bool showLabelAbove = false, int previewSize = 150);
    void EmptyTextureSlot(ImDrawList* dl, const ImRect& r, float cell = 10.0f);

    inline bool BeginTopBar(float height, ImU32 bg_col, float rounding, float pad_x)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0,0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,   ImVec2(8,4));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding,  ImVec2(8,4));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.y < height) height = avail.y;

        bool opened = ImGui::BeginChild("##TopBarChild", ImVec2(0, height), false,
                                        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetWindowPos();
        ImVec2 p1 = ImVec2(p0.x + ImGui::GetWindowSize().x, p0.y + height);
        dl->AddRectFilled(p0, p1, bg_col, rounding);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + pad_x);

        return opened;
    }

    inline void EndTopBar()
    {
        ImGui::EndChild();
        ImGui::PopStyleVar(4);
    }

    template <typename DrawContent>
    inline bool TopBarDropdown(const char* id, const char* label, DrawContent&& draw)
    {
        bool pressed = false;
        ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0,0,0,0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1,1,1,0.05f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(1,1,1,0.08f));
        if (ImGui::Button(label)) { ImGui::OpenPopup(id); pressed = true; }
        ImGui::PopStyleColor(3);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,8));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f,0.08f,0.10f,0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border,  ImVec4(0.18f,0.18f,0.22f,1.0f));

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
