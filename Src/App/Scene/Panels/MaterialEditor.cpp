#include "CorePCH.hpp"
#include "MaterialEditor.hpp"

namespace Motion
{
    enum class PropertyAction : int
    {
        Changed = 0,
        Reset,
        None
    };
    
    static PropertyAction ColorCard(const char* label, glm::vec4& color, const char* description = nullptr, bool showAlpha = true)
    {
        PropertyAction result = PropertyAction::None;
        ImGui::PushID(label);
        
        const float cardWidth = 260.0f;
        const float previewSize = 80.0f;
        const float padding = 12.0f;
        const float cornerRadius = 8.0f;
        
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + cardWidth, p0.y + previewSize + padding * 2);
        ImRect cardRect(p0, p1);
        
        auto* dl = ImGui::GetWindowDrawList();
        bool hovered = ImGui::IsMouseHoveringRect(cardRect.Min, cardRect.Max);
        
        // Card background
        ImU32 bgColor = hovered ? 
            ImGui::GetColorU32(ImGuiCol_FrameBgHovered) : 
            ImGui::GetColorU32(ImGuiCol_FrameBg);
        dl->AddRectFilled(cardRect.Min, cardRect.Max, bgColor, cornerRadius);
        
        // Color preview box
        ImVec2 previewPos = ImVec2(p0.x + padding, p0.y + padding);
        ImRect previewRect(previewPos, ImVec2(previewPos.x + previewSize, previewPos.y + previewSize));
        
        ImU32 colorU32 = ImGui::ColorConvertFloat4ToU32(ImVec4(color.r, color.g, color.b, color.a));
        dl->AddRectFilled(previewRect.Min, previewRect.Max, colorU32, cornerRadius - 2.0f);
        dl->AddRect(previewRect.Min, previewRect.Max, 
                   ImGui::GetColorU32(ImGuiCol_Border), cornerRadius - 2.0f, 0, 1.5f);
        
        // Label and description
        ImVec2 textPos = ImVec2(previewRect.Max.x + padding, p0.y + padding);
        ImGui::SetCursorScreenPos(textPos);
        
        ImGui::BeginGroup();
        ImGui::TextUnformatted(label);
        
        if (description)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::PushTextWrapPos(cardRect.Max.x - padding);
            ImGui::TextWrapped("%s", description);
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
        ImGui::EndGroup();
        
        // Invisible button for click interaction
        ImGui::SetCursorScreenPos(cardRect.Min);
        ImGui::InvisibleButton("##color_card", ImVec2(cardWidth, previewSize + padding * 2));
        
        // Color picker popup
        if (ImGui::IsItemClicked())
        {
            ImGui::OpenPopup("##color_picker_popup");
        }
        
        if (ImGui::BeginPopup("##color_picker_popup"))
        {
            ImGuiColorEditFlags flags = ImGuiColorEditFlags_Float | 
                                       ImGuiColorEditFlags_DisplayRGB |
                                       ImGuiColorEditFlags_PickerHueWheel;
            
            if (!showAlpha)
                flags |= ImGuiColorEditFlags_NoAlpha;
            
            if (ImGui::ColorPicker4("##picker", glm::value_ptr(color), flags))
            {
                result = PropertyAction::Changed;
            }
            
            ImGui::Spacing();
            if (ImGui::Button("Done", ImVec2(-FLT_MIN, 0)))
            {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        // Context menu
        if (ImGui::BeginPopupContextItem("##color_ctx"))
        {
            if (ImGui::MenuItem("Reset to White"))
            {
                color = glm::vec4(1.0f, 1.0f, 1.0f, showAlpha ? 1.0f : color.a);
                result = PropertyAction::Reset;
            }
            if (ImGui::MenuItem("Reset to Black"))
            {
                color = glm::vec4(0.0f, 0.0f, 0.0f, showAlpha ? 1.0f : color.a);
                result = PropertyAction::Reset;
            }
            ImGui::EndPopup();
        }
        
        ImGui::SetCursorScreenPos(ImVec2(p0.x, cardRect.Max.y + 8.0f));
        ImGui::PopID();
        return result;
    }
    
    static PropertyAction SliderCard(const char* label, float& value, float minValue, float maxValue, const char* description = nullptr, const char* format = "%.2f")
    {
        PropertyAction result = PropertyAction::None;
        ImGui::PushID(label);
        
        const float cardWidth = 260.0f;
        const float cardHeight = 90.0f;
        const float padding = 12.0f;
        const float cornerRadius = 8.0f;
        
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + cardWidth, p0.y + cardHeight);
        ImRect cardRect(p0, p1);
        
        auto* dl = ImGui::GetWindowDrawList();
        bool hovered = ImGui::IsMouseHoveringRect(cardRect.Min, cardRect.Max);
        
        // Card background
        ImU32 bgColor = hovered ? 
            ImGui::GetColorU32(ImGuiCol_FrameBgHovered) : 
            ImGui::GetColorU32(ImGuiCol_FrameBg);
        dl->AddRectFilled(cardRect.Min, cardRect.Max, bgColor, cornerRadius);
        
        ImGui::SetCursorScreenPos(ImVec2(p0.x + padding, p0.y + padding));
        
        // Label
        ImGui::BeginGroup();
        ImGui::TextUnformatted(label);
        
        // Description
        if (description)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::PushTextWrapPos(cardRect.Max.x - padding * 2);
            ImGui::TextWrapped("%s", description);
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
        
        ImGui::Spacing();
        
        // Slider
        ImGui::SetNextItemWidth(cardWidth - padding * 2);
        if (ImGui::SliderFloat("##slider", &value, minValue, maxValue, format))
        {
            result = PropertyAction::Changed;
        }
        
        ImGui::EndGroup();
        
        // Context menu
        if (ImGui::BeginPopupContextItem("##slider_ctx"))
        {
            if (ImGui::MenuItem("Reset to Default"))
            {
                value = (minValue + maxValue) * 0.5f; // Reset to middle
                result = PropertyAction::Reset;
            }
            if (ImGui::MenuItem("Set to Minimum"))
            {
                value = minValue;
                result = PropertyAction::Reset;
            }
            if (ImGui::MenuItem("Set to Maximum"))
            {
                value = maxValue;
                result = PropertyAction::Reset;
            }
            ImGui::EndPopup();
        }
        
        ImGui::SetCursorScreenPos(ImVec2(p0.x, cardRect.Max.y + 8.0f));
        ImGui::PopID();
        return result;
    }
    
    static PropertyAction PresetCard(const char* label, const std::vector<std::string>& options, int& selectedIndex, const char* description = nullptr)
    {
        PropertyAction result = PropertyAction::None;
        ImGui::PushID(label);
        
        const float cardWidth = 260.0f;
        const float cardHeight = 90.0f;
        const float padding = 12.0f;
        const float cornerRadius = 8.0f;
        
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 p1 = ImVec2(p0.x + cardWidth, p0.y + cardHeight);
        ImRect cardRect(p0, p1);
        
        auto* dl = ImGui::GetWindowDrawList();
        bool hovered = ImGui::IsMouseHoveringRect(cardRect.Min, cardRect.Max);
        
        // Card background
        ImU32 bgColor = hovered ? 
            ImGui::GetColorU32(ImGuiCol_FrameBgHovered) : 
            ImGui::GetColorU32(ImGuiCol_FrameBg);
        dl->AddRectFilled(cardRect.Min, cardRect.Max, bgColor, cornerRadius);
        
        ImGui::SetCursorScreenPos(ImVec2(p0.x + padding, p0.y + padding));
        
        ImGui::BeginGroup();
        ImGui::TextUnformatted(label);
        
        if (description)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::PushTextWrapPos(cardRect.Max.x - padding * 2);
            ImGui::TextWrapped("%s", description);
            ImGui::PopTextWrapPos();
            ImGui::PopStyleColor();
        }
        
        ImGui::Spacing();
        
        // Combo box
        ImGui::SetNextItemWidth(cardWidth - padding * 2);
        
        if (options.empty())
        {
            ImGui::TextDisabled("No options available");
        }
        else
        {
            // Clamp index
            if (selectedIndex < 0 || selectedIndex >= static_cast<int>(options.size()))
                selectedIndex = 0;
            
            if (ImGui::BeginCombo("##combo", options[selectedIndex].c_str()))
            {
                for (int i = 0; i < static_cast<int>(options.size()); ++i)
                {
                    bool isSelected = (i == selectedIndex);
                    if (ImGui::Selectable(options[i].c_str(), isSelected))
                    {
                        selectedIndex = i;
                        result = PropertyAction::Changed;
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
        }
        
        ImGui::EndGroup();
        
        ImGui::SetCursorScreenPos(ImVec2(p0.x, cardRect.Max.y + 8.0f));
        ImGui::PopID();
        return result;
    }

    void MaterialEditor::OnCreate(Scene* scene)
    {
        static const std::array<const char*, 5> Paths = 
        {
            "Assets/Materials/Metal/Base.yaml",
            "Assets/Materials/Marble/Base.yaml",
            "Assets/Materials/Plastic/Base.yaml",
            "Assets/Materials/Rubber/Base.yaml",
            "Assets/Materials/Stone/Base.yaml",
        };

        m_Materials.reserve(Paths.size());
        
        for (auto* p : Paths) 
            m_Materials.push_back(Material::CreateBase(p));
    }

    void MaterialEditor::OnRender(Scene* scene)
    {
        if (!scene) return;

        auto& context = scene->GetContext();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 12.0f));
        
        ImGui::SetNextWindowSize(ImVec2(580.0f, 720.0f), ImGuiCond_FirstUseEver);
        
        if (ImGui::Begin("Material Editor", &context.Panels->ShowEntityMaterials, ImGuiWindowFlags_NoDocking))
        {
            if (context.Entities->SelectedEntity == entt::null || 
                !context.Entities->Registry.valid(context.Entities->SelectedEntity))
            {
                ImVec2 center       = ImGui::GetMainViewport()->GetCenter();
                ImVec2 windowSize   = ImGui::GetWindowSize();
                ImVec2 textSize     = ImGui::CalcTextSize("No Entity Selected");
                
                ImGui::SetCursorPos(ImVec2(
                    (windowSize.x - textSize.x) * 0.5f,
                    (windowSize.y - textSize.y) * 0.5f - 40.0f
                ));
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("No Entity Selected");
                
                ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("Select an entity to edit its material").x) * 0.5f);
                ImGui::TextWrapped("Select an entity to edit its material");
                ImGui::PopStyleColor();
                
                ImGui::End();
                ImGui::PopStyleVar(2);
                return;
            }

            auto* mat = context.Entities->Registry.try_get<MaterialComponent>(context.Entities->SelectedEntity);
            if (!mat)
            {
                ImVec2 windowSize = ImGui::GetWindowSize();
                ImVec2 textSize = ImGui::CalcTextSize("No Material Component");
                
                ImGui::SetCursorPos(ImVec2(
                    (windowSize.x - textSize.x) * 0.5f,
                    (windowSize.y - textSize.y) * 0.5f - 40.0f
                ));
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("No Material Component");
                
                ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize("This entity doesn't have a material").x) * 0.5f);
                ImGui::TextWrapped("This entity doesn't have a material");
                ImGui::PopStyleColor();
                
                ImGui::End();
                ImGui::PopStyleVar(2);
                return;      
            }

            if (mat->MaterialPointer)
            {
                DrawMaterialUI(mat->MaterialPointer);
            }
            else
            {
                ImVec2 windowSize = ImGui::GetWindowSize();
                ImVec2 textSize = ImGui::CalcTextSize("No Material Assigned");
                
                ImGui::SetCursorPos(ImVec2(
                    (windowSize.x - textSize.x) * 0.5f,
                    (windowSize.y - textSize.y) * 0.5f - 40.0f
                ));
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::Text("No Material Assigned");
                ImGui::PopStyleColor();
            }

            ImGui::End();
        }
        
        ImGui::PopStyleVar(2);
    }

    void MaterialEditor::DrawMaterialUI(std::shared_ptr<Material>& mat)
    {
        if (!mat) return;
        if (ImGui::BeginChild("##MaterialContent", ImVec2(0, 0), false, 
                             ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.12f, 0.18f, 0.5f));
            if (ImGui::BeginChild("##InfoPanel", ImVec2(0, 80.0f), true, 
                                 ImGuiWindowFlags_NoScrollbar))
            {
                ImGui::Spacing();
                ImGui::Indent(12.0f);
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
                ImGui::TextWrapped("Materials define how light interacts with surfaces - "
                                 "controlling color, reflectivity, and surface details");
                ImGui::PopStyleColor();
                
                ImGui::Unindent(12.0f);
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            if (ImGui::CollapsingHeader("Material Preset", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent(8.0f);
                
                auto base = mat->GetBaseMaterial();
                std::vector<std::string> names;
                names.reserve(m_Materials.size() + 1);
                names.push_back("None");
                
                std::ranges::transform(m_Materials, std::back_inserter(names),
                    [](const auto& m) { return m->Name; });

                std::int32_t index = 0;
                if (base)
                {
                    if (auto it = std::ranges::find(names, base->Name); it != names.end())
                        index = static_cast<std::int32_t>(std::distance(names.begin(), it));
                }

                if (PresetCard("Base Preset", names, index, 
                              "Quick presets for common materials") == PropertyAction::Changed)
                {
                    if (index == 0 || names[index] == "None")
                    {
                        mat->SetBaseMaterial(nullptr);
                    }
                    else
                    {
                        if (auto it = std::ranges::find_if(m_Materials, 
                            [&](const auto& m) { return m->Name == names[index]; }); 
                            it != m_Materials.end())
                        {
                            mat->SetBaseMaterial(*it);
                        }
                    }
                }
                
                ImGui::Unindent(8.0f);
                ImGui::Spacing();
            }
            
            if (!mat->Has<CoreMaterialComponents>())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("No material properties available");
                ImGui::PopStyleColor();
                ImGui::EndChild();
                return;
            }
            
            auto& C = mat->Get<CoreMaterialComponents>();
            
            if (ImGui::CollapsingHeader("Surface Properties", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent(8.0f);
                
                const int columns = 2;
                float availWidth = ImGui::GetContentRegionAvail().x;
                float cardWidth = (availWidth - 16.0f) / columns; 
                
                ColorCard("Base Color", C.BaseColorFactor, 
                         "Main surface color", true);
                
                ImGui::SameLine();
                
                SliderCard("Metallic", C.MetallicFactor, 0.0f, 1.0f,
                          "0=Dielectric, 1=Metal");
                
                SliderCard("Roughness", C.RoughnessFactor, 0.0f, 1.0f,
                          "0=Smooth, 1=Rough");
                
                ImGui::SameLine();
                
                SliderCard("Opacity", C.OpacityFactor, 0.0f, 1.0f,
                          "Surface transparency");
                
                ImGui::Unindent(8.0f);
                ImGui::Spacing();
            }
            
            if (ImGui::CollapsingHeader("Surface Details"))
            {
                ImGui::Indent(8.0f);
                
                SliderCard("Normal Strength", C.NormalScale, 0.0f, 2.0f,
                          "Surface bump intensity");
                
                ImGui::SameLine();
                
                SliderCard("Ambient Occlusion", C.OcclusionStrength, 0.0f, 1.0f,
                          "Shadow depth in crevices");
                
                ImGui::Unindent(8.0f);
                ImGui::Spacing();
            }
            
            
            if (ImGui::CollapsingHeader("Emission"))
            {
                ImGui::Indent(8.0f);
                
                glm::vec4 emissiveColor(C.EmissiveFactor.r, C.EmissiveFactor.g, C.EmissiveFactor.b, 1.0f);
                ColorCard("Emissive Color", 
                         emissiveColor,
                         "Glow color", false);
                
                ImGui::SameLine();
                
                SliderCard("Emission Strength", C.EmissiveStrength, 0.0f, 10.0f,
                          "Brightness of glow", "%.1f");
                
                ImGui::Unindent(8.0f);
                ImGui::Spacing();
            }
            
            if (ImGui::CollapsingHeader("Texture Maps", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Indent(8.0f);
                
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.5f, 1.0f));
                ImGui::TextWrapped("Click slots to load textures. Right-click for options.");
                ImGui::PopStyleColor();
                ImGui::Spacing();
                
                struct TexRow 
                { 
                    const char* Label; 
                    std::shared_ptr<ITexture>& Tex; 
                    TextureType Type; 
                };
                
                std::vector<TexRow> textures =
                {
                    {"Base Color",  C.BaseColorTexture, TextureType::BaseColorTexture},
                    {"Metallic",    C.MetallicTexture,  TextureType::MetallicTexture},
                    {"Roughness",   C.RoughnessTexture, TextureType::RoughnessTexture},
                    {"Normal",      C.NormalTexture,    TextureType::NormalTexture},
                    {"Occlusion",   C.OcclusionTexture, TextureType::AmbientOcclusionTexture},
                    {"Emissive",    C.EmissiveTexture,  TextureType::EmissiveTexture},
                };

                const int texColumns = 3;
                for (size_t i = 0; i < textures.size(); ++i)
                {
                    TextureSlot(textures[i].Label, textures[i].Tex, textures[i].Type, 
                               nullptr, false, 140);
                    
                    if ((i + 1) % texColumns != 0 && i < textures.size() - 1)
                        ImGui::SameLine();
                }
                
                ImGui::Unindent(8.0f);
            }
            
            ImGui::Spacing();
            ImGui::Spacing();
        }
        ImGui::EndChild();
    }
}