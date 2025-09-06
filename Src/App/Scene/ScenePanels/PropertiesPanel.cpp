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
        if(BeginPropertyGrid("##base-material"))
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

            ComboBox("Base Material", materialNames, index, [&](std::int32_t selectedIndex, const std::string& selectedName)
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

            EndPropertyGrid();
        }

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

        if(mat->HasTexture<CorePBR>())
        {
            auto& C = mat->GetTexture<CorePBR>();

            if(BeginPropertyGrid("##core-pbr"))
            {
                ColorEdit4("Base Color",            C.BaseColorFactor);
                SliderFloat("Metallic Factor",      &C.MetallicFactor, 0.0f, 1.0f, "%.3f");
                SliderFloat("Roughness Factor",     &C.RoughnessFactor, 0.0f, 1.0f, "%.3f");
                SliderFloat("Normal Scaling",       &C.NormalScale,     0.0f, 1.0f, "%.3f");
                SliderFloat("Occlusion Strength",   &C.OcclusionStrength, 0.0f, 1.0f, "%.3f");
                ColorEdit3("Emissive Factor",       C.EmissiveFactor);
                SliderFloat("Emissive Strength",    &C.EmissiveStrength, 0.0f, 1.0f, "%.3f");
                SliderFloat("Opacity Factor",       &C.OpacityFactor, 0.0f, 1.0f, "%.3f");
                EndPropertyGrid();
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
                TextureSlot(textures[i].Label, textures[i].Tex, textures[i].Type);
            }

            ImGui::EndTable();
        }
        else
        {
            ImGui::TextDisabled(ICON_FA_INFO " No textures assigned");
        }
    }

    void SceneEntityPropertiesPanel::RenderUI(ScenePanelContext& ctx)
    {
        ImGui::Begin(ICON_MD_SETTINGS " Properties");

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
            BeginPropertyGrid("##tag-grid");
            auto& tag = selectedEntity->GetComponent<TagComponent>();

            TextBox(ICON_FA_SHAPES" Name Tag", tag.Tag, false, 256);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Friendly name for this mesh. Useful for organization");

            ToggleSwitch(ICON_FA_EYE " Is Active", tag.IsActive);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("This entity should display or not");

            EndPropertyGrid();
        }
  
        if (selectedEntity->HasComponent<TransformComponent>())
        {
            auto& component = selectedEntity->GetComponent<TransformComponent>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, ICON_FA_CUBES " Transform"))
            {
                BeginPropertyGrid("##transform-grid");
                DragFloat3(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT " Translation", component.Translation, 0.1f);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Position of the object in world space (X, Y, Z)");

                glm::vec3 rotationDgree = glm::degrees(glm::eulerAngles(component.Rotation));
                if(DragFloat3(ICON_FA_ARROWS_ROTATE " Rotation", rotationDgree, 0.1f))
                {
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("Orientation of the object. Adjust as angles (degrees) around X, Y, Z axes");
                    glm::vec3 RdRad       = glm::radians(rotationDgree);
                    component.Rotation    = glm::quat(RdRad);
                }
                
                DragFloat3(ICON_MD_ZOOM_OUT_MAP " Scale", component.Scale, 0.1f);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Size of the object along each axis. Example: (2,1,1) doubles the width only.");
                
                EndPropertyGrid();
                ImGui::TreePop();
            }
        }

        if (selectedEntity->HasComponent<StaticMeshComponent>())
        {
            auto& component = selectedEntity->GetComponent<StaticMeshComponent>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, ICON_MD_IMAGE " Materials"))
            {
                if(selectedEntity->HasComponent<ColliderComponent>())
                {
                    auto& phyMat = selectedEntity->GetComponent<ColliderComponent>().MaterialBase;
                    BeginPropertyGrid("##phycalmat-grid");

                    DragFloat(ICON_FA_CIRCLE_ARROW_UP" Restitution", &phyMat.Restitution, 0.001f, 0.0f, 1.0f);
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("Bounciness. 0 = no bounce, 1 = perfectly elastic (like a super ball).");

                    DragFloat(ICON_FA_HAND" Friction Static", &phyMat.FrictionStatic, 0.001f, 0.0f, 2.0f);
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("How hard it is to start moving when at rest (grip)");

                    DragFloat(ICON_FA_ARROW_RIGHT_LONG" Friction Dynamic", &phyMat.FrictionDynamic, 0.001f, 0.0f, 2.0f);
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("How much resistance occurs while sliding.");

                    static std::int32_t rCombine{0};
                    ComboBox(ICON_MD_FUNCTIONS" Restitution Combine", { "Average", "Minimum", "Maximum", "Multiply" }, rCombine, [&](std::int32_t selectedIndex, const std::string& selectedItem)
                    {
                        if(selectedIndex == 0) phyMat.RestitutionCombine = CombineMode::Average;
                        if(selectedIndex == 1) phyMat.RestitutionCombine = CombineMode::Minimum;
                        if(selectedIndex == 2) phyMat.RestitutionCombine = CombineMode::Maximum;
                        if(selectedIndex == 3) phyMat.RestitutionCombine = CombineMode::Multiply;
                    });
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("How bounciness is calculated between two colliding objects");

                    static std::int32_t fCombine{2};
                    ComboBox(ICON_MD_INTEGRATION_INSTRUCTIONS" Friction Combine", { "Average", "Minimum", "Maximum", "Multiply" }, fCombine, [&](std::int32_t selectedIndex, const std::string& selectedItem)
                    {
                        if(selectedIndex == 0) phyMat.RestitutionCombine = CombineMode::Average;
                        if(selectedIndex == 1) phyMat.RestitutionCombine = CombineMode::Minimum;
                        if(selectedIndex == 2) phyMat.RestitutionCombine = CombineMode::Maximum;
                        if(selectedIndex == 3) phyMat.RestitutionCombine = CombineMode::Multiply;
                    });
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("How friction is calculated when two objects touch (average, min, max, multiply).");

                    EndPropertyGrid();
                }

                static bool s_ShowMaterialEditor = false;
                if (ImGui::Button(ICON_MD_EDIT " Open Advanced Material Editor"))
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
                                                if (ImGui::Selectable(fmt::format("[{}] - {}", model[i]->Index, model[i]->Name).c_str(), isSelected))
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
                    ImGui::End();
                }

                ImGui::TreePop();
            }
        }

        if (selectedEntity->HasComponent<RigidBodyComponent>())
        {
            auto& component = selectedEntity->GetComponent<RigidBodyComponent>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, ICON_MD_3D_ROTATION" RigidBody"))
            {
                BeginPropertyGrid("##rigid-body-grid");

                DragFloat(ICON_FA_DUMBBELL" Mass", &component.Mass, 0.01f, 0.01f, 1000.0f);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("How heavy the object is. 0 makes it immovable (static). Higher values make it harder to accelerate.");
                
                DragFloat(ICON_FA_WIND" Linear Damping", &component.LinearDamping, 0.001f, 0.0f, 1.0f);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Air resistance. Higher values slow down linear motion. 0 = no drag, 1 = very strong drag.");
                
                DragFloat(ICON_FA_CIRCLE_NOTCH" Angular Damping", &component.LinearDamping, 0.001f, 0.0f, 1.0f);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Rotational resistance. Higher values slow down spinning. 0 = spins forever, 1 = stops quickly");

                ToggleSwitch(ICON_FA_EYE " Is Sleeping", component.Sleeping);
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("If enabled, the body can stop simulating when inactive to save performance.");

                EndPropertyGrid();
                ImGui::TreePop();
            }
        }

        if (selectedEntity->HasComponent<ColliderComponent>())
        {
            auto& component = selectedEntity->GetComponent<ColliderComponent>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, ICON_MD_ADJUST" Collider"))
            {
                BeginPropertyGrid("##collider-grid");
                static std::int32_t cType{0};
                ComboBox(ICON_MD_FUNCTIONS" Collider Type", { "None", "Sphere", "Box", "Capsule" }, cType, [&](std::int32_t selectedIndex, const std::string& selectedItem)
                {
                    if(selectedIndex == 0) component.Type = ColliderType::None;
                    if(selectedIndex == 1) component.Type = ColliderType::Sphere;
                    if(selectedIndex == 2) component.Type = ColliderType::Box;
                    if(selectedIndex == 3) component.Type = ColliderType::Capsule;
                });
                if(ImGui::IsItemHovered()) ImGui::SetTooltip("Shape used for collisions. Defines how the object interacts physically with the world");

                if(component.Type == ColliderType::Box)
                {
                    DragFloat3(ICON_MD_CROP_SQUARE" Half Extents", component.Box.HalfExtents, 0.01f, 0.01f, 100.0f);     
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("Half the size of the box in each axis. Example: (1,1,1) makes a full box size of (2,2,2).");
                }

                if(component.Type == ColliderType::Sphere)
                {
                    DragFloat(ICON_FA_GLOBE" Radius", &component.Sphere.Radius, 0.01f, 0.01f, 100.0f);
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("Radius of the spherical collider. Controls how large the sphere collision area is.");
                }

                if(component.Type == ColliderType::Capsule)
                {
                    DragFloat3(ICON_MD_CROP_SQUARE" Half Extents", component.Box.HalfExtents, 0.01f, 0.01f, 100.0f);     
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("Half the height of the straight middle section of the capsule.");
                    DragFloat(ICON_FA_GLOBE" Radius", &component.Sphere.Radius, 0.01f, 0.01f, 100.0f);
                    if(ImGui::IsItemHovered()) ImGui::SetTooltip("Radius of the capsule’s rounded ends.");
                }
                EndPropertyGrid();

                if(component.Type == ColliderType::None)
                {
                    ImGui::TextUnformatted(ICON_MD_INFO " This entity has no collision shape. It will not interact physically with other objects or participate in the physics simulation. Use this for purely visual objects or markers that don’t need physics.");
                }

                ImGui::TreePop();
            }

        }


        ImGui::End();
    }



    
}