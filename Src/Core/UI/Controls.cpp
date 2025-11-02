#include "CorePCH.hpp"

namespace Motion
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

    bool DragFloat2(const char* label, glm::vec2& v, float speed, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::DragFloat2("##v", glm::value_ptr(v), speed, minV, maxV, fmt, ImGuiSliderFlags_AlwaysClamp);
    }

    bool DragFloat3(const char* label, glm::vec3& v, float speed, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::DragFloat3("##v", glm::value_ptr(v), speed, minV, maxV, fmt, ImGuiSliderFlags_AlwaysClamp);
    }

    bool SliderFloat(const char* label, float* v, float minV, float maxV, const char* fmt)
    {
        ScopeID idScope(label);
        BeginRow(label);
        return ImGui::SliderFloat("##v", v, minV, maxV, fmt);
    }

    bool ColorEdit4(const char* label, glm::vec4& color, bool withAlpha)
    {
        ScopeID idScope(label);
        BeginRow(label);
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB;
        if (!withAlpha) 
            flags |= ImGuiColorEditFlags_NoAlpha;
        return ImGui::ColorEdit4("##c", glm::value_ptr(color), flags);
    }

    bool ColorEdit3(const char* label, glm::vec3& color)
    {
        ScopeID idScope(label);
        BeginRow(label);
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB;
        return ImGui::ColorEdit3("##c", glm::value_ptr(color), flags);
    }

    static int ResizeCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
        {
            auto* str = static_cast<std::string*>(data->UserData);
            IM_ASSERT(str != nullptr);
            IM_ASSERT(data->Buf == str->data());
            str->resize(data->BufTextLen);
            data->Buf = str->data();
        }
        return 0;
    }

    bool TextBox(const char* label, std::string& value, bool readOnly, size_t maxLen)
    {
        ScopeID idScope(label);
        BeginRow(label);
        
        ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackResize;
        if (readOnly) 
            flags |= ImGuiInputTextFlags_ReadOnly;

        // Ensure string has enough capacity for editing
        if (value.capacity() < maxLen + 1) 
            value.reserve(maxLen + 1);

        bool changed = ImGui::InputText("##txt", value.data(), value.capacity() + 1, 
                                       flags, ResizeCallback, &value);
        
        // Enforce max length constraint
        if (value.size() > maxLen) 
            value.resize(maxLen);
        
        return !readOnly && changed;
    }

    bool ToggleSwitch(const char* label, bool& value)
    {
        ScopeID idScope(label);
        BeginRow(label);
        
        ImVec2 p = ImGui::GetCursorScreenPos();
        float h = ImGui::GetFrameHeight();
        float w = h * 2.0f;
        float radius = h * 0.5f;
        
        ImGui::InvisibleButton("##tgl", ImVec2(w, h));
        bool hovered = ImGui::IsItemHovered();
        bool pressed = ImGui::IsItemClicked();
        
        if (pressed) 
            value = !value;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImU32 bg = ImGui::GetColorU32(value ? ImGuiCol_CheckMark : ImGuiCol_FrameBg);
        if (hovered) 
            bg = ImGui::GetColorU32(ImGuiCol_ButtonHovered);
        
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), bg, radius);
        
        float knobOffset = value ? (w - h) : 0.0f;
        dl->AddCircleFilled(ImVec2(p.x + radius + knobOffset, p.y + radius), 
                           h * 0.40f, ImGui::GetColorU32(ImGuiCol_Text));

        return pressed;
    }

    bool SearchBox(const char* id, std::string& query, const char* hint)
    {
        ScopeID idScope(id);
        ImGui::TableNextColumn();
        ImGui::TableNextColumn(); // full width row
        ImGui::SetNextItemWidth(-FLT_MIN);
        
        // Ensure string has capacity for editing
        constexpr size_t minCapacity = 256;
        if (query.capacity() < minCapacity)
            query.reserve(minCapacity);
        
        bool changed = ImGui::InputTextWithHint("##search", hint ? hint : "Search...", 
                                               query.data(), query.capacity() + 1,
                                               ImGuiInputTextFlags_CallbackResize, 
                                               ResizeCallback, &query);
        return changed;
    }

    bool ComboBox(const char* label, const std::vector<std::string>& options, int& index, 
                  std::function<void(std::int32_t, const std::string&)> onChanged)
    {
        ScopeID idScope(label);
        BeginRow(label);
        
        if (options.empty()) 
        {
            ImGui::TextDisabled("No options");
            return false;
        }
        
        // Clamp index to valid range
        if (index < 0 || index >= static_cast<int>(options.size())) 
            index = 0;

        bool changed = false;
        if (ImGui::BeginCombo("##combo", options[index].c_str()))
        {
            for (int i = 0; i < static_cast<int>(options.size()); ++i)
            {
                bool isSelected = (i == index);
                if (ImGui::Selectable(options[i].c_str(), isSelected))
                {
                    index = i;
                    changed = true;
                    if (onChanged) 
                        onChanged(index, options[index]);
                }

                if (isSelected) 
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        return changed;
    }

    bool CollapsibleSection(const char* label, bool defaultOpen)
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed | 
                                  ImGuiTreeNodeFlags_SpanAvailWidth | 
                                  ImGuiTreeNodeFlags_AllowItemOverlap;
        if (defaultOpen) 
            flags |= ImGuiTreeNodeFlags_DefaultOpen;
        
        return ImGui::CollapsingHeader(label, flags);
    }

    void ToolbarBegin(const char* id)
    {
        ImGui::PushID(id);
        ImGui::BeginGroup();
    }

    bool ToolbarButton(const char* id, const char* iconText, const char* tooltip)
    {
        ImGui::PushID(id);
        bool pressed = ImGui::Button(iconText);
        
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
        ImGui::PopID();
    }

    // Helper function to load texture via file dialog
    static bool LoadTextureFromDialog(std::shared_ptr<ITexture>& tex, TextureType type)
    {
        DialogBoxes::InitializeCOM();

        OpenDialogOptions options{};
        options.Title = L"Import Texture";
        options.DefaultExtension = L"png";
        options.AllowMultiSelect = false;
        options.InitialDirectory = std::filesystem::current_path();
        options.Filters = 
        {
            {L"Image Files", L"*.jpg;*.jpeg;*.png;*.bmp;*.tga;*.hdr"},
            {L"All Files",   L"*.*"}
        };

        std::filesystem::path path = DialogBoxes::OpenFileDialog(options);
        DialogBoxes::UninitializeCOM();
        
        if (!path.empty())
        {
            if (!tex)
                tex = ITexture::Create(path, type);
            else
                tex->ReloadFromFile(path, type);
            return true;
        }
        
        return false;
    }

    static ImTextureID AsImTextureID(const std::shared_ptr<ITexture>& t)
    {
        if (!t) 
            return (ImTextureID)(uintptr_t)0;
        return (ImTextureID)(uintptr_t)t->GetID();
    }

    static std::string NiceFilename(const std::shared_ptr<ITexture>& t)
    {
        if (!t) 
            return "(none)";
        
        const auto& spec = t->GetSpecification();
        if (!spec.TextureFile.empty())
            return std::filesystem::path(spec.TextureFile).filename().string();
        
        if (spec.Width && spec.Height)
            return fmt::format("{}x{}", spec.Width, spec.Height);
        
        return "(runtime)";
    }

    void EmptyTextureSlot(ImDrawList* dl, const ImRect& r, float cell)
    {
        const ImU32 c0 = IM_COL32(110, 110, 110, 255);
        const ImU32 c1 = IM_COL32(70, 70, 70, 255);

        for (float y = r.Min.y; y < r.Max.y; y += cell)
        {
            for (float x = r.Min.x; x < r.Max.x; x += cell)
            {
                bool alt = (static_cast<int>((x - r.Min.x) / cell) + 
                           static_cast<int>((y - r.Min.y) / cell)) & 1;
                ImRect q(ImVec2(x, y), 
                        ImVec2(std::min(x + cell, r.Max.x), 
                               std::min(y + cell, r.Max.y)));
                dl->AddRectFilled(q.Min, q.Max, alt ? c0 : c1);
            }
        }
        dl->AddRect(r.Min, r.Max, IM_COL32(50, 50, 50, 255), 6.0f, 0, 1.0f);
    }

    TextureSlotAction TextureSlot(const char* label, std::shared_ptr<ITexture>& tex, 
                                 TextureType type, 
                                 std::function<void(TextureSlotAction, std::shared_ptr<ITexture>&)> onAction, 
                                 bool showLabelAbove, int previewSize)
    {
        TextureSlotAction result = TextureSlotAction::None;
        ImGui::PushID(label);
        
        constexpr float padding = 8.0f;
        constexpr float cornerRadius = 8.0f;
        const ImVec2 size(static_cast<float>(previewSize), static_cast<float>(previewSize));

        // Calculate card layout
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + size.x + padding * 2, 
                          p0.y + size.y + padding * 2 + ImGui::GetTextLineHeight() * 1.5f);
        ImRect cardRect(p0, p1);

        // Draw card background
        auto* dl = ImGui::GetWindowDrawList();
        ImU32 colBg = ImGui::GetColorU32(ImGuiCol_FrameBg);
        ImU32 colHover = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
        bool hovered = ImGui::IsMouseHoveringRect(cardRect.Min, cardRect.Max);

        dl->AddRectFilled(cardRect.Min, cardRect.Max, hovered ? colHover : colBg, cornerRadius);

        // Draw texture preview
        ImVec2 texPos = ImVec2(p0.x + padding, p0.y + padding);
        ImRect texRect(texPos, ImVec2(texPos.x + size.x, texPos.y + size.y));

        if (tex)
        {
            dl->AddImage(AsImTextureID(tex), texRect.Min, texRect.Max, 
                        ImVec2(0, 1), ImVec2(1, 0));
        }
        else
        {
            dl->AddRect(texRect.Min, texRect.Max, 
                       ImGui::GetColorU32(ImGuiCol_Border), cornerRadius);
            ImVec2 textPos = ImVec2(texRect.Min.x + 4, 
                                   texRect.Min.y + size.y * 0.5f - ImGui::GetTextLineHeight() * 0.5f);
            ImGui::SetCursorScreenPos(textPos);
            ImGui::TextDisabled("Click to load");
        }

        // Invisible button for interaction
        ImGui::SetCursorScreenPos(texRect.Min);
        ImGui::InvisibleButton("tex_card_btn", size);

        // Tooltip with larger preview
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            const int bigPreview = std::min(previewSize * 2, 512);
            const ImVec2 bigSize(static_cast<float>(bigPreview), static_cast<float>(bigPreview));
            
            if (tex)
                ImGui::Image(AsImTextureID(tex), bigSize, ImVec2(0, 1), ImVec2(1, 0));
            else
                ImGui::Dummy(bigSize);
            
            ImGui::Separator();
            ImGui::TextUnformatted(std::format("Expected: {}", GetTextureTypeString(type)).c_str());
            
            if (tex)
            {
                const auto& sp = tex->GetSpecification();
                if (!sp.TextureFile.empty()) 
                    ImGui::TextUnformatted(sp.TextureFile.c_str());
                ImGui::Text("Size: %u x %u", sp.Width, sp.Height);
            }
            ImGui::EndTooltip();
        }

        // Handle left-click to load texture
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            if (LoadTextureFromDialog(tex, type))
            {
                if (onAction) 
                    onAction(TextureSlotAction::Upload, tex);
                result = TextureSlotAction::Upload;
            }
        }

        // Context menu
        if (ImGui::BeginPopupContextItem("tex_card_ctx"))
        {
            if (ImGui::MenuItem("Load Texture"))
            {
                if (LoadTextureFromDialog(tex, type))
                {
                    if (onAction) 
                        onAction(TextureSlotAction::Upload, tex);
                    result = TextureSlotAction::Upload;
                }
            }

            bool canReload = tex && !tex->GetSpecification().TextureFile.empty();
            if (ImGui::MenuItem("Reload", nullptr, false, canReload))
            {
                const auto& file = tex->GetSpecification().TextureFile;
                tex->ReloadFromFile(file, type);
                if (onAction) 
                    onAction(TextureSlotAction::Reload, tex);
                result = TextureSlotAction::Reload;
            }

            if (tex)
            {
                ImGui::Separator();
                if (ImGui::MenuItem("Flip Vertically"))
                {
                    const auto& file = tex->GetSpecification().TextureFile;
                    bool flip = !tex->GetSpecification().FlipOnLoadDefault;
                    tex->ReloadFromFile(file, type, flip);
                    if (onAction) 
                        onAction(TextureSlotAction::Reload, tex);
                    result = TextureSlotAction::Reload;
                }
                ImGui::Separator();
            }

            if (ImGui::MenuItem("Clear", nullptr, false, tex != nullptr))
            {
                tex.reset();
                if (onAction) 
                    onAction(TextureSlotAction::Clear, tex);
                result = TextureSlotAction::Clear;
            }

            ImGui::EndPopup();
        }

        // Drag and drop support
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET_PATH"))
            {
                const char* path = static_cast<const char*>(payload->Data);
                if (path && *path)
                {
                    if (!tex)
                        tex = ITexture::Create(path, type);
                    else
                        tex->ReloadFromFile(path, type);
                    
                    if (onAction) 
                        onAction(TextureSlotAction::Upload, tex);
                    result = TextureSlotAction::Upload;
                }
            }
            ImGui::EndDragDropTarget();
        }

        // Draw texture type label
        ImGui::SetCursorScreenPos(ImVec2(p0.x + padding, texRect.Max.y + 4));
        ImGui::PushTextWrapPos(texRect.Max.x);
        ImGui::TextUnformatted(GetTextureTypeString(type).c_str());
        ImGui::PopTextWrapPos();

        ImGui::Dummy(ImVec2(0, 12));
        ImGui::PopID();
        return result;
    }
}