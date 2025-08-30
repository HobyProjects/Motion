#include "CorePCH.hpp"
#include "PropertiesPanel.hpp"

namespace Motion
{
    SceneEntityPropertiesPanel::SceneEntityPropertiesPanel()
    {
        auto& AM = AssetManager::GetInstance();
        m_BaseMaterial.push_back(AM.Get<BaseMaterial>("MarbleBaseMaterial"));
        m_BaseMaterial.push_back(AM.Get<BaseMaterial>("MetalBaseMaterial"));
        m_BaseMaterial.push_back(AM.Get<BaseMaterial>("PlasticBaseMaterial"));
        m_BaseMaterial.push_back(AM.Get<BaseMaterial>("RubberBaseMaterial"));
        m_BaseMaterial.push_back(AM.Get<BaseMaterial>("StoneBaseMaterial"));
    }

    void SceneEntityPropertiesPanel::DrawMaterialUI(ScenePanelContext&, std::shared_ptr<Material>& mat)
    {
        if (!mat) return;

        if (ImGui::TreeNodeEx("Material Properties", ImGuiTreeNodeFlags_Framed))
        {
            DrawAttributes(mat);
            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx("Textures", ImGuiTreeNodeFlags_Framed))
        {
            DrawTexturesSlots(mat);
            ImGui::TreePop();
        }
    }

    void SceneEntityPropertiesPanel::DrawAttributes(std::shared_ptr<Material>& mat)
    {
        if(UI::BeginPropertyGrid("##base-material"))
        {
            auto base = mat->GetBaseMaterial();
            std::vector<std::string> materialNames;
            materialNames.reserve(m_BaseMaterial.size() + 1);
            materialNames.push_back("None");

            std::ranges::transform(m_BaseMaterial, std::back_inserter(materialNames), [](const auto& matPtr) { return matPtr->GetName(); });

            std::int32_t index = 0;
            if (base)
            {
                auto it = std::ranges::find(materialNames, base->GetName());
                if (it != materialNames.end())
                    index = static_cast<std::int32_t>(std::distance(materialNames.begin(), it));
            }

            UI::ComboBox("Base Material", materialNames, index, [&](std::int32_t selectedIndex, const std::string& selectedName)
            {
                if (selectedName == "None")
                {
                    mat->SetBaseMaterial(nullptr);
                    return;
                }

                auto it = std::ranges::find_if(m_BaseMaterial, [&](const auto& matPtr)
                {
                    return matPtr->GetName() == selectedName;
                });

                if (it != m_BaseMaterial.end())
                    mat->SetBaseMaterial(*it);
            });

            UI::EndPropertyGrid();
        }

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

        if(mat->HasTexture<CorePBR>())
        {
            auto& C = mat->GetTexture<CorePBR>();

            if(UI::BeginPropertyGrid("##core-pbr"))
            {
                UI::ColorEdit4("Base Color",            C.BaseColorFactor);
                UI::SliderFloat("Metallic Factor",      &C.MetallicFactor, 0.0f, 1.0f, "%.3f");
                UI::SliderFloat("Roughness Factor",     &C.RoughnessFactor, 0.0f, 1.0f, "%.3f");
                UI::SliderFloat("Normal Scaling",       &C.NormalScale,     0.0f, 1.0f, "%.3f");
                UI::SliderFloat("Occlusion Strength",   &C.OcclusionStrength, 0.0f, 1.0f, "%.3f");
                UI::ColorEdit3("Emissive Factor",       C.EmissiveFactor);
                UI::SliderFloat("Emissive Strength",    &C.EmissiveStrength, 0.0f, 1.0f, "%.3f");
                UI::SliderFloat("Opacity Factor",       &C.OpacityFactor, 0.0f, 1.0f, "%.3f");

                UI::EndPropertyGrid();
            }
        }
        else
        {
            ImGui::TextDisabled(ICON_MD_INFO " No textures assigned");
        }
    }

    void SceneEntityPropertiesPanel::DrawTexturesSlots(std::shared_ptr<Material>& mat)
    {
        if(mat->HasTexture<CorePBR>())
        {
            auto& C = mat->GetTexture<CorePBR>();

            struct TextureEntry { const char* Label; std::shared_ptr<ITexture>& Tex; TextureType Type; };
            std::vector<TextureEntry> textures = 
            {
                {"Base Color",  C.BaseColorTexture, TextureType::BaseColorTexture},
                {"Metallic",    C.MetallicTexture,  TextureType::MetallicTexture},
                {"Roughness",   C.RoughnessTexture, TextureType::RoughnessTexture},
                {"Normal",      C.NormalTexture,    TextureType::NormalTexture},
                {"Occlusion",   C.OcclusionTexture, TextureType::AmbientOcclusionTexture},
                {"Emissive",    C.EmissiveTexture,  TextureType::EmissiveTexture},
            };

            const int columns = 4;                     // number of cards per row
            const float cardSpacing = 3.0f;            // space between cards

            ImGui::BeginTable("##core-pbr", columns, ImGuiTableFlags_NoBordersInBody);

            for(size_t i = 0; i < textures.size(); i++)
            {
                if(i % columns == 0)
                    ImGui::TableNextRow();

                ImGui::TableNextColumn();
                UI::TextureSlot(textures[i].Label, textures[i].Tex, textures[i].Type);
            }

            ImGui::EndTable();
        }
        else
        {
            ImGui::TextDisabled(ICON_MD_INFO " No textures assigned");
        }
    }

    void SceneEntityPropertiesPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin(ICON_MD_SETTINGS " Properties");

        // Safer selected-entity resolution
        std::shared_ptr<Entity> selectedEntity = ctx.ActiveScene ? ctx.ActiveScene->GetSelectedEntity() : nullptr;
        const bool hasSelection = selectedEntity && selectedEntity != EntityFactory::EMPTYENTITY;
        static const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | 
            ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | 
            ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;


        if (!hasSelection)
        {
            ImGui::TextDisabled("%s  No entity selected", ICON_MD_INFO);
            ImGui::End();
            return;
        }

        if (selectedEntity->HasComponent<TagComponent>())
        {
            UI::BeginPropertyGrid("##tag-grid");
            auto& tag = selectedEntity->GetComponent<TagComponent>();
            UI::TextBox(ICON_MD_LABEL" Name Tag", tag.Tag, false, 256);
            UI::EndPropertyGrid();
        }

        
        if (selectedEntity->HasComponent<TransformComponent>())
        {
            auto& component = selectedEntity->GetComponent<TransformComponent>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, ICON_MD_OPEN_WITH " Transform"))
            {
                UI::BeginPropertyGrid("##transform-grid");
                UI::DragFloat3(ICON_MD_DIRECTIONS " Translation",          component.Translation, 0.1f);
                UI::DragFloat3(ICON_MD_ROTATE_90_DEGREES_CW " Rotation",   component.Rotation, 0.1f, -glm::pi<float>(), glm::pi<float>());
                UI::DragFloat3(ICON_MD_ZOOM_OUT_MAP " Scale",              component.Scale, 0.1f);
                UI::EndPropertyGrid();
                ImGui::TreePop();
            }
        }

        if (selectedEntity->HasComponent<StaticMeshComponent>())
        {
            auto& component = selectedEntity->GetComponent<StaticMeshComponent>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, ICON_MD_IMAGE " Materials"))
            {
                // Persist across frames
                static bool s_ShowMaterialEditor = false;

                if (ImGui::Button(ICON_MD_EDIT " Open Material Editor"))
                    s_ShowMaterialEditor = true;

                if (s_ShowMaterialEditor)
                {
                    if (ImGui::Begin(ICON_MD_IMAGE " Material Editor", &s_ShowMaterialEditor))
                    {
                        auto selected = ctx.ActiveScene->GetSelectedEntity();
                        if (!selected || selected == EntityFactory::EMPTYENTITY)
                        {
                            ImGui::TextDisabled(ICON_MD_INFO " No entity selected.");
                        }
                        else if (!selected->HasComponent<StaticMeshComponent>())
                        {
                            ImGui::TextDisabled(ICON_MD_INFO " Selected entity has no Static Mesh Component.");
                        }
                        else
                        {
                            auto& smc = selected->GetComponent<StaticMeshComponent>();
                            if (!smc.Model)
                            {
                                ImGui::TextDisabled(ICON_MD_INFO " Entity has no model.");
                            }
                            else
                            {
                                // Resolve material for selected mesh (create if missing)
                                std::shared_ptr<Material> mat = nullptr;
                                if (m_SelectedMesh >= 0 && m_SelectedMesh < (int)smc.Model->GetMeshesCount())
                                {
                                    auto& model = *smc.Model;
                                    auto mesh = model[m_SelectedMesh];
                                    if (mesh && !mesh->Materials)
                                    {
                                        auto& factory = MaterialBuilder::GetInstance();
                                        mesh->Materials = factory.Create(nullptr);
                                    }
                                    if (mesh) mat = mesh->Materials;
                                }

                                if (ImGui::BeginTable("##content-splitted", 2,
                                        ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody))
                                {
                                    ImGui::TableSetupColumn("##side-bar", ImGuiTableColumnFlags_WidthFixed, 260.0f);
                                    ImGui::TableSetupColumn("##inspector-panel", ImGuiTableColumnFlags_WidthStretch);
                                    ImGui::TableNextRow();

                                    // Sidebar
                                    ImGui::TableSetColumnIndex(0);
                                    if (ImGui::BeginChild("##side-bar-list", ImVec2(0.0f, 0.0f), true))
                                    {
                                        auto& model = *smc.Model;
                                        const uint32_t count = model.GetMeshesCount();
                                        if (count > 0)
                                        {
                                            for (uint32_t i = 0; i < count; ++i)
                                            {
                                                const bool isSelected = (m_SelectedMesh == (int)i);
                                                if (ImGui::Selectable(
                                                        fmt::format("[{}] - {}", model[i]->Index, model[i]->Name).c_str(),
                                                        isSelected))
                                                {
                                                    m_SelectedMesh = (int)i;
                                                }
                                            }
                                        }
                                        else
                                        {
                                            ImGui::TextDisabled(ICON_MD_INFO " Model has no meshes.");
                                        }
                                    }
                                    ImGui::EndChild();

                                    // Inspector
                                    ImGui::TableSetColumnIndex(1);
                                    if (ImGui::BeginChild("##inspector-area", ImVec2(0.0f, 0.0f), true))
                                    {
                                        if (mat)
                                            DrawMaterialUI(ctx, mat);
                                        else
                                            ImGui::TextDisabled(ICON_MD_INFO " Select a mesh from the list.");
                                    }
                                    ImGui::EndChild();

                                    ImGui::EndTable();
                                }
                            }
                        }
                    }
                    ImGui::End(); // Always pair with Begin
                }

                ImGui::TreePop();
            }
        }

        ImGui::End();
    }



    
}