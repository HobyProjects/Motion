#pragma once

#include <string>
#include <vector>
#include <functional>
#include <algorithm>

#include <imgui/imgui.h>
#include <glm/glm.hpp>

#define MOTION_UI_TEXTURESLOT_CALLBACK(CALLBACK_FUNC) [this](auto&&... args) -> decltype(auto) { return this->CALLBACK_FUNC(std::forward<decltype(args)>(args)...); }
namespace Motion::UI
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
        if (spec.twoColumns) {
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

    using ActionFn = std::function<void()>;
    using ComboChangedFn = std::function<void(int, const std::string&)>;

    bool DragFloat(const char* label, float* v, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");
    bool DragFloat2(const char* label, float v[2], float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");
    bool DragFloat2(const char* label, glm::vec2& v, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");
    bool DragFloat3(const char* label, float v[3], float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");
    bool DragFloat3(const char* label, glm::vec3& v, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");
    bool DragFloat3(const char* label, glm::quat& v, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX, const char* fmt = "%.3f");
    bool DragFloat3WithReset(const char* label, float v[3], float resetValue = 0.0f, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX);
    bool DragFloat3WithReset(const char* label, glm::vec3& v, float resetValue = 0.0f, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX);
    bool DragFloat3WithReset(const char* label, glm::quat& v, float resetValue = 0.0f, float speed = 0.1f, float minV = -FLT_MAX, float maxV = FLT_MAX);

    bool SliderFloat(const char* label, float* v, float minV, float maxV, const char* fmt = "%.3f");
    bool ColorEdit4(const char* label, float color[4], bool withAlpha = true);
    bool ColorEdit4(const char* label, glm::vec4& color, bool withAlpha = true);
    bool ColorEdit3(const char* label, float color[3]);
    bool ColorEdit3(const char* label, glm::vec3& color);

    bool TextBox(const char* label, std::string& value, bool readOnly = false, size_t maxLen = 1024);

    bool ToggleSwitch(const char* label, bool& value);
    bool SearchBox(const char* id, std::string& query, const char* hint = "Search...");
    bool ComboBox(const char* label, const std::vector<std::string>& options, int& index, ComboChangedFn onChanged = nullptr);

    bool CollapsibleSection(const char* label, bool defaultOpen = true);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    struct Tag
    {
        std::string text{};
        bool selected{ false };
    };
    bool TagChips(const char* label, std::vector<Tag>& tags, ActionFn onAdd = nullptr, ActionFn onRemove = nullptr);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ToolbarBegin(const char* id);
    bool ToolbarButton(const char* id, const char* iconText, const char* tooltip = nullptr);
    void ToolbarEnd();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    inline void SmallHelp(const char* text)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip(); ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40.0f);
            ImGui::TextUnformatted(text);
            ImGui::PopTextWrapPos(); ImGui::EndTooltip();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    enum class TextureSlotResult
    {
        None,
        LoadedNew,
        Reloaded,
        Cleared
    };

    enum class TextureSlotAction : int { Upload = 0, Reload, Clear, None };
    using TextureSlotActionCallback = std::function<void(TextureSlotAction, std::shared_ptr<ITexture>&)>;

    TextureSlotAction TextureSlot(const char* label, std::shared_ptr<ITexture>& tex, TextureType type, TextureSlotActionCallback onAction = nullptr, bool showLabelAbove = false, int previewSize = 100);
    void EmptyTextureSlot(ImDrawList* dl, const ImRect& r, float cell = 10.0f);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct GridTableOptions
    {
        ImVec2 cellPadding{ 8, 6 };   // inside each cell
        bool   drawCellBg = false;
        ImU32  cellBgColor = IM_COL32(40, 40, 40, 40);
        float  rounding = 6.0f;
        ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoHostExtendX | ImGuiTableFlags_BordersInner | ImGuiTableFlags_RowBg;
    };

    struct GridTableState
    {
        int cols = 0;
        int cellIndex = 0;
        ImVec2 cellSize;
        ImVec2 pad;
        bool drawBg = false;
        ImU32 bgCol = 0;
        float rounding = 0.0f;
        ImGuiID id = 0;
        bool began = false;
    };

    bool GridBegin(const char* id, int cols, ImVec2 cellSize = ImVec2(120, 120), const GridTableOptions& opt = {});
    void GridEnd();
    bool GridCellBegin();
    void GridCellEnd();

    struct Grid
    {
        bool  ok = false;
        int   cols = 1;

        Grid(const char* id, int columns, ImVec2 cellSize, const GridTableOptions& opts = {}) : cols(columns)
        {
            ok = UI::GridBegin(id, columns, cellSize, opts);
        }

        ~Grid()
        {
            if (ok) UI::GridEnd();
        }

        template <class Fn>
        Grid& cell(Fn&& fn)
        {
            if (!ok) return *this;
            if (UI::GridCellBegin())
            {
                std::forward<Fn>(fn)();
                UI::GridCellEnd();
            }

            return *this;
        }

        template <class Fn>
        Grid& times(int n, Fn&& fn)
        {
            for (int i = 0; i < n; ++i)
                cell([&] { std::forward<Fn>(fn)(i); });

            return *this;
        }

        Grid& newline();
    };

}