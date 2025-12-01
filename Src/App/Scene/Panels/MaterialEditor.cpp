#include "CorePCH.hpp"
#include "MaterialEditor.hpp"

namespace Motion
{
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
        if(!scene) return;

        auto& context = scene->GetContext();
        
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 16));
        ImGui::Begin("Material Editor", &context.Panels->ShowEntityMaterials);

        if (context.Entities->SelectedEntity == entt::null)
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_INFO_OUTLINE "No Entity Selected", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("Select an entity to edit its material properties", lblConfig);
            
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        auto* mc = context.Entities->Registry.try_get<MaterialComponent>(context.Entities->SelectedEntity);
        if (!mc || !mc->MaterialPointer)
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_INFO_OUTLINE " No Material", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("This entity doesn't have a material assigned", lblConfig);
            
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }

        DrawMaterialUI(mc->MaterialPointer);
        
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void MaterialEditor::DrawMaterialUI(std::shared_ptr<Material>& mat)
    {
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_INFO_OUTLINE " PBR Material", HeadingLevel::H2, config);

            LabelConfig lblConfig;
            lblConfig.Wrapped = true;
            LabelSimple("Materials define how light interacts with surfaces - controlling color, reflectivity, roughness, and surface details", lblConfig); 
        }
        
        ImGui::Spacing();
        
        if (!m_Materials.empty())
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_APP_REGISTRATION " Quick Presets", HeadingLevel::H3, config);
            LabelSimple("Select from common material presets");
            
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

            ComboBoxConfig presetConfig;
            presetConfig.Tooltip = "Select you're preset for the entity";
            ComboBox("Presets", index, names, presetConfig, [&](const std::string& itemName, int itemIndex){
                if(itemIndex == 0 || itemName == "None")
                {
                    mat->SetBaseMaterial(nullptr);
                }
                else
                {
                    if (auto it = std::ranges::find_if(m_Materials, 
                        [&](const auto& m) { return m->Name == itemName; }); 
                        it != m_Materials.end())
                    {
                        mat->SetBaseMaterial(*it);
                    }
                }
            });   
        }

        ImGui::Spacing();
        
        if (!mat->Has<CoreMaterialComponents>())
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_AUTO_AWESOME " No Properties", HeadingLevel::H3, config);
            LabelSimple("No material properties available for editing.");
            
            ImGui::End();
            ImGui::PopStyleVar();
            return;
        }
        
        auto& C = mat->Get<CoreMaterialComponents>();

        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_AUTO_AWESOME " Surface Properties", HeadingLevel::H3, config);
            
            ColorEditConfig colorConfig;
            colorConfig.Flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_Float;
            ColorEdit4("Base Color", C.BaseColorFactor, colorConfig);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Main surface color (RGBA)");

            SliderFloatConfig metallicConfig;
            metallicConfig.MinV = 0.0f;
            metallicConfig.MaxV = 1.0f;
            metallicConfig.Fmt = "%.2f";
            SliderFloat("Metallic", &C.MetallicFactor, metallicConfig);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("0 = Dielectric (non-metal)\n1 = Metal");

            SliderFloatConfig roughnessConfig;
            roughnessConfig.MinV = 0.0f;
            roughnessConfig.MaxV = 1.0f;
            roughnessConfig.Fmt = "%.2f";
            SliderFloat("Roughness", &C.RoughnessFactor, roughnessConfig);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("0 = Smooth/Glossy\n1 = Rough/Matte");

            SliderFloatConfig opacityConfig;
            opacityConfig.MinV = 0.0f;
            opacityConfig.MaxV = 1.0f;
            opacityConfig.Fmt = "%.2f";
            SliderFloat("Opacity", &C.OpacityFactor, opacityConfig);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Surface transparency\n0 = Transparent\n1 = Opaque");
        }
        
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_AUTO_AWESOME " Surface Details", HeadingLevel::H3, config);

            SliderFloatConfig normalConfig;
            normalConfig.MinV = 0.0f;
            normalConfig.MaxV = 2.0f;
            normalConfig.Fmt = "%.2f";
            SliderFloat("Normal Strength", &C.NormalScale, normalConfig);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Surface bump/detail intensity");

            SliderFloatConfig aoConfig;
            aoConfig.MinV = 0.0f;
            aoConfig.MaxV = 1.0f;
            aoConfig.Fmt = "%.2f";
            SliderFloat("Ambient Occlusion", &C.OcclusionStrength, aoConfig);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Shadow depth in crevices and corners");
        }
        
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_LIGHTBULB_OUTLINE " Emission", HeadingLevel::H3, config);

            glm::vec4 emissiveColor(C.EmissiveFactor.r, C.EmissiveFactor.g, C.EmissiveFactor.b, 1.0f);

            ColorEditConfig emissiveConfig;
            emissiveConfig.Flags = ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoAlpha;
            ColorEdit4("Emissive Color", emissiveColor, emissiveConfig, [&](glm::vec4 color){
                C.EmissiveFactor = glm::vec4(color.r, color.g, color.b, color.a);
            });
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Glow color");

            SliderFloatConfig strengthConfig;
            strengthConfig.MinV = 0.0f;
            strengthConfig.MaxV = 10.0f;
            strengthConfig.Fmt = "%.1f";
            strengthConfig.Tooltip = "Brightness/intensity of the glow";
            SliderFloat("Emission Strength", &C.EmissiveStrength, strengthConfig);
        }
        
        ImGui::Spacing();
        
        {
            HeadingConfig config;
            config.Separator = true;
            Heading(ICON_MD_TEXTURE " Texture Maps", HeadingLevel::H3, config);
            
            LabelConfig labelConfig;
            labelConfig.Wrapped = true;
            LabelSimple("Manage texture maps for different material properties", labelConfig);
            LabelSimple("Supported maps: Base Color, Metallic, Roughness, Normal, Occlusion, Emissive", labelConfig);
            
            struct TextureMapping
            {
                const char* Label;
                std::shared_ptr<ITexture>& Texture;
                TextureType Type;
            };
            
            std::vector<TextureMapping> textures = {
                {"Base Color",  C.BaseColorTexture,  TextureType::BaseColorTexture},
                {"Metallic",    C.MetallicTexture,   TextureType::MetallicTexture},
                {"Roughness",   C.RoughnessTexture,  TextureType::RoughnessTexture},
                {"Normal",      C.NormalTexture,     TextureType::NormalTexture},
                {"Occlusion",   C.OcclusionTexture,  TextureType::AmbientOcclusionTexture},
                {"Emissive",    C.EmissiveTexture,   TextureType::EmissiveTexture},
            };
            
            TextureSlotConfig slotConfig = TextureSlotPresets::Card();
            slotConfig.ShowFilename = false;
            slotConfig.CardPadding = 10.0f;

            for (size_t i = 0; i < textures.size(); ++i)
            {
                auto& mapping = textures[i];
                TextureSlot(mapping.Label, mapping.Texture, mapping.Type, slotConfig, [&](TextureSlotAction action, std::shared_ptr<ITexture>& tex) {
                    if(action == TextureSlotAction::Upload)
                        MOTION_INFO("Texture uploaded: {}", mapping.Label);
                });

                ImGui::SameLine();
            }
        }
    }
}