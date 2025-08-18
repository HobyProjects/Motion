#include "CorePCH.hpp"
#include "Controls.hpp"

namespace Motion::UI
{
    static void BeginRow(const char* label)
    {
        ImGui::TableNextColumn();
        PropertyLabel(label);
        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
    }

    bool DragFloat(const char* label, float* v, float speed, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::DragFloat("##v", v, speed, minV, maxV, fmt, ImGuiSliderFlags_AlwaysClamp);
    }

    bool DragFloat2(const char* label, float v[2], float speed, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::DragFloat2("##v", v, speed, minV, maxV, fmt, ImGuiSliderFlags_AlwaysClamp);
    }

    bool DragFloat2(const char* label, glm::vec2& v, float speed, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::DragFloat2("##v", glm::value_ptr(v), speed, minV, maxV, fmt, ImGuiSliderFlags_AlwaysClamp);
    }

    bool DragFloat3(const char* label, float v[3], float speed, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::DragFloat3("##v", v, speed, minV, maxV, fmt, ImGuiSliderFlags_AlwaysClamp);
    }

    bool DragFloat3(const char* label, glm::vec3& v, float speed, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::DragFloat3("##v", glm::value_ptr(v), speed, minV, maxV, fmt, ImGuiSliderFlags_AlwaysClamp);
    }

    bool DragFloat3(const char* label, glm::quat& v, float speed, float minV, float maxV, const char* fmt)
    {
        glm::vec3 euler = glm::eulerAngles(v);
        bool changed = DragFloat3(label, euler, speed, minV, maxV, fmt);
        if (changed) v = glm::quat(euler);
        return changed;
    }

    bool DragFloat3WithReset(const char* label, float v[3], float resetValue, float speed, float minV, float maxV)
    {
        ScopeID idScope(label);
        BeginRow(label);
        bool changed = false;
        const float btnW = ImGui::GetFrameHeight(); // square
        ImGui::PushID(label);
        const char* buttonLabels[3] = { "X", "Y", "Z" };
        for (int i = 0; i < 3; ++i) {
            ImGui::PushID(i);
            if (ImGui::Button(buttonLabels[i], ImVec2(btnW, 0))) { v[i] = resetValue; changed = true; }
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::SetNextItemWidth((ImGui::GetContentRegionAvail().x - 12.0f) / 3.0f - btnW);
            changed |= ImGui::DragFloat("##ax", &v[i], speed, minV, maxV, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            if (i != 2) ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::PopID();
        return changed;
    }

    bool DragFloat3WithReset(const char* label, glm::vec3& v, float resetValue, float speed, float minV, float maxV)
    {
        ScopeID idScope(label);
        BeginRow(label);
        bool changed = false;
        const float btnW = ImGui::GetFrameHeight(); // square
        ImGui::PushID(label);
        const char* buttonLabels[3] = { "X", "Y", "Z" };
        for (int i = 0; i < 3; ++i) {
            ImGui::PushID(i);
            if (ImGui::Button(buttonLabels[i], ImVec2(btnW, 0))) { v[i] = resetValue; changed = true; }
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::SetNextItemWidth((ImGui::GetContentRegionAvail().x - 12.0f) / 3.0f - btnW);
            changed |= ImGui::DragFloat("##ax", &v[i], speed, minV, maxV, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            if (i != 2) ImGui::SameLine();
            ImGui::PopID();
        }

        ImGui::PopID();
        return changed;
    }

    bool DragFloat3WithReset(const char* label, glm::quat& v, float resetValue, float speed, float minV, float maxV)
    {
        ScopeID idScope(label);
        BeginRow(label);
        glm::vec3 euler = glm::eulerAngles(v);
        bool changed = false;
        const float btnW = ImGui::GetFrameHeight(); // square
        ImGui::PushID(label);
        const char* buttonLabels[3] = { "X", "Y", "Z" };
        for (int i = 0; i < 3; ++i) {
            ImGui::PushID(i);
            if (ImGui::Button(buttonLabels[i], ImVec2(btnW, 0))) { v[i] = resetValue; changed = true; }
            ImGui::SameLine(0.0f, 6.0f);
            ImGui::SetNextItemWidth((ImGui::GetContentRegionAvail().x - 12.0f) / 3.0f - btnW);
            changed |= ImGui::DragFloat("##ax", &euler[i], speed, minV, maxV, "%.3f", ImGuiSliderFlags_AlwaysClamp);
            if (i != 2) ImGui::SameLine();
            ImGui::PopID();
        }
        ImGui::PopID();
        if (changed) v = glm::quat(euler);
        return changed;
    }

    bool SliderFloat(const char* label, float* v, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::SliderFloat("##v", v, minV, maxV, fmt);
    }

    bool ColorEdit4(const char* label, float color[4], bool withAlpha)
    {
        ScopeID idScope(label);
        BeginRow(label);
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB;
        if (!withAlpha) flags |= ImGuiColorEditFlags_NoAlpha;
        return ImGui::ColorEdit4("##c", color, flags);
    }

    bool ColorEdit4(const char* label, glm::vec4& color, bool withAlpha)
    {
        ScopeID idScope(label);
        BeginRow(label);
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB;
        if (!withAlpha) flags |= ImGuiColorEditFlags_NoAlpha;
        return ImGui::ColorEdit4("##c", glm::value_ptr(color), flags);
    }

    bool ColorEdit3(const char* label, float color[3])
    {
        ScopeID idScope(label);
        BeginRow(label);
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB;
        return ImGui::ColorEdit3("##c", color, flags);
    }

    bool ColorEdit3(const char* label, glm::vec3& color)
    {
        ScopeID idScope(label);
        BeginRow(label);
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB;
        return ImGui::ColorEdit3("##c", glm::value_ptr(color), flags);
    }

    // String editing with automatic capacity growth (no shared statics)
    static int ResizeCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            auto* str = static_cast<std::string*>(data->UserData);
            str->resize(data->BufTextLen);
            data->Buf = str->data();
        }
        return 0;
    }

    bool TextBox(const char* label, std::string& value, bool readOnly, size_t maxLen)
    {
        ScopeID idScope(label);
        BeginRow(label);
        bool changed = false;
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackResize;
        if (readOnly) flags |= ImGuiInputTextFlags_ReadOnly;

        // Ensure capacity (soft cap)
        if (value.capacity() < maxLen) value.reserve(maxLen);

        changed = ImGui::InputText("##txt", value.data(), value.capacity() + 1, flags, ResizeCallback, &value);
        if (value.size() > maxLen) value.resize(maxLen);
        return !readOnly && changed;
    }

    // -------------------- High-level widgets --------------------
    bool ToggleSwitch(const char* label, bool& value)
    {
        ScopeID idScope(label);
        BeginRow(label);
        ImVec2 p = ImGui::GetCursorScreenPos();
        float h = ImGui::GetFrameHeight();
        float w = h * 2.0f;
        ImGui::InvisibleButton("##tgl", ImVec2(w, h));
        bool hovered = ImGui::IsItemHovered();
        bool pressed = ImGui::IsItemClicked();
        if (pressed) value = !value;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 bg = ImGui::GetColorU32(value ? ImGuiCol_CheckMark : ImGuiCol_FrameBg);
        if (hovered) bg = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), bg, h * 0.5f);
        float t = value ? (w - h) : 0.0f;
        dl->AddCircleFilled(ImVec2(p.x + h * 0.5f + t, p.y + h * 0.5f), h * 0.40f,
            ImGui::GetColorU32(ImGuiCol_Text));

        return pressed;
    }

    bool SearchBox(const char* id, std::string& query, const char* hint)
    {
        ScopeID idScope(id);
        ImGui::TableNextColumn();
        ImGui::TableNextColumn(); // full width row
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::PushID(id);
        bool changed = ImGui::InputTextWithHint("##search", hint, query.data(),
            query.capacity() > 0 ? query.capacity() + 1 : 0,
            ImGuiInputTextFlags_CallbackResize, ResizeCallback, &query);
        ImGui::PopID();
        return changed;
    }

    bool ComboBox(const char* label, const std::vector<std::string>& options, int& index, ComboChangedFn onChanged)
    {
        ScopeID idScope(label);
        BeginRow(label);
        if (options.empty()) {
            ImGui::TextDisabled("No options");
            return false;
        }
        if (index < 0 || index >= (int)options.size()) index = 0;

        bool changed = false;
        if (ImGui::BeginCombo("##combo", options[index].c_str()))
        {
            for (int i = 0; i < (int)options.size(); ++i)
            {
                bool isSel = (i == index);
                if (ImGui::Selectable(options[i].c_str(), isSel))
                {
                    index = i; changed = true;
                    if (onChanged) onChanged(index, options[index]);
                }

                if (isSel) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    bool TagChips(const char* label, std::vector<Tag>& tags, ActionFn onAdd, ActionFn onRemove)
    {
        ScopeID idScope(label);
        BeginRow(label);
        bool changed = false;
        float avail = ImGui::GetContentRegionAvail().x;
        float x = 0.0f;
        for (size_t i = 0; i < tags.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            std::string text = tags[i].text;
            ImVec2 sz = ImGui::CalcTextSize(text.c_str());
            ImVec2 pad(10, 4);
            ImVec2 chip(sz.x + pad.x * 2 + 16, sz.y + pad.y * 2); // +close button width
            if (x + chip.x > avail) { x = 0; ImGui::NewLine(); }

            ImGui::BeginGroup();
            ImGui::Selectable((" " + text + "  x").c_str(), &tags[i].selected, 0, chip);
            bool hovered = ImGui::IsItemHovered();
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                tags[i].selected = !tags[i].selected; changed = true;
            }
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && onRemove)
            {
                onRemove(); changed = true;
            }
            ImGui::EndGroup();
            ImGui::SameLine(0, 6);
            x += chip.x + 6;
            ImGui::PopID();
        }

        if (onAdd)
        {
            if (ImGui::Button("+ Add Tag")) { onAdd(); changed = true; }
        }
        return changed;
    }

    bool TextureCard(const char* label, std::shared_ptr<ITexture>& texture, bool canReload, bool canClear, TextureActionCallback onAction, ImVec2 uv0, ImVec2 uv1, ImVec2 cardSize, ImVec2 previewMax)
    {
        ScopeID idScope(label);
        ImGui::TableNextColumn();
        PropertyLabel(label);
        ImGui::TableNextColumn();

        // Card
        bool changed = false;
        const ImVec2 card = cardSize;
        ImGui::BeginChild(("##card" + std::to_string(GetUID())).c_str(), card, true, ImGuiWindowFlags_NoScrollbar);
        {
            // Preview
            ImVec2 avail = ImGui::GetContentRegionAvail();
            ImVec2 sz = previewMax;
            sz.x = std::min(sz.x, avail.x);
            sz.y = std::min(sz.y, avail.y - ImGui::GetFrameHeight() - 8.0f);
            ImVec2 p0 = ImGui::GetCursorScreenPos();
            ImGui::Dummy(ImVec2((avail.x - sz.x) * 0.5f, 0));
            ImGui::SameLine();

            if (texture)
            {
                ImTextureID textureID = (ImTextureID)(intptr_t)texture->GetID();
                ImGui::Image(textureID ? textureID : (ImTextureID)(intptr_t)0, ImVec2(100, 100));
            }
            else
            {
                ImGui::Image((ImTextureID)(intptr_t)0, ImVec2(100, 100));
            }

            ImGui::SetCursorPosY(card.y - ImGui::GetFrameHeight() - 6.0f);
            ImGui::Separator();
            if (ImGui::Button(ICON_MD_UPLOAD))
            {
                if (onAction)
                    onAction(TextureAction::RequestUpload, texture);
            }

            ImGui::SameLine();
            if (canReload)
            {
                if (ImGui::Button(ICON_MD_REFRESH) && onAction)
                    onAction(TextureAction::RequestReload, texture);
            }

            ImGui::SameLine();
            if (canClear && ImGui::Button(ICON_MD_CLEAR))
            {
                if (onAction)
                    onAction(TextureAction::RequestClear, texture);
            }
        }
        ImGui::EndChild();
        return changed;
    }

    bool CollapsibleSection(const char* label, bool defaultOpen)
    {
        ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen * defaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap;
        bool open = ImGui::CollapsingHeader(label, f);
        return open;
    }

    void ToolbarBegin(const char* id)
    {
        ImGui::PushID(id);
        ImGui::BeginGroup();
    }

    bool ToolbarButton(const char* id, const char* iconText, const char* tooltip)
    {
        bool pressed = false;
        ImGui::PushID(id);
        pressed = ImGui::Button(iconText);
        if (tooltip && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", tooltip);
        }
        ImGui::SameLine(0.0f, 6.0f);
        ImGui::PopID();
        return pressed;
    }

    void ToolbarEnd()
    {
        ImGui::EndGroup();
        ImGui::NewLine();
        ImGui::PopID();
    }
}