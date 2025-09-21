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
        ImGui::BeginDisabled(ctx.ActiveScene->GetSimualtionState() == SimulationState::Running);

        std::shared_ptr<Entity> selectedEntity = ctx.ActiveScene ? ctx.ActiveScene->GetSelectedEntity() : nullptr;
        const bool hasSelection = selectedEntity && selectedEntity != EntityFactory::EMPTYENTITY;
        static const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | 
            ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | 
            ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;


        if (!hasSelection)
        {
            ImGui::EndDisabled();
            ImGui::TextDisabled(ICON_MD_INFO" No entity selected");
            ImGui::End();
            return;
        }

        bool isActive{false};
        if (selectedEntity->HasComponent<TagComponent>())
        {
            BeginPropertyGrid("##tag-grid");
            auto& tag = selectedEntity->GetComponent<TagComponent>();

            TextBox("Name Tag", tag.Tag, false, 256);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("Friendly name for this mesh. Useful for organization");

            isActive = tag.IsActive;
            ToggleSwitch("Is Active", tag.IsActive);
            if(ImGui::IsItemHovered()) ImGui::SetTooltip("This entity should display or not");

            EndPropertyGrid();
        }

        if(isActive)
        {
            if (selectedEntity->HasComponent<TransformComponent>())
            {
                auto& tr = selectedEntity->GetComponent<TransformComponent>();
                if (ImGui::TreeNodeEx((void*)tr.ID, treeNodeFlags, ICON_FA_CUBES " Transform"))
                {
                    BeginPropertyGrid("##transform-grid");

                    {
                        glm::vec3 posM = tr.Translation;                
                        if (DragFloat3("Position (m)", posM, 0.01f))
                            tr.Translation = posM;

                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("World position in meters (X, Y, Z).");
                    }

                    {
                        glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(tr.Rotation));
                        auto wrap180 = [](float a)
                        {
                            a = std::fmod(a + 180.0f, 360.0f);
                            if (a < 0) a += 360.0f;
                            return a - 180.0f;
                        };
                        eulerDeg.x = wrap180(eulerDeg.x);
                        eulerDeg.y = wrap180(eulerDeg.y);
                        eulerDeg.z = wrap180(eulerDeg.z);

                        glm::vec3 edited = eulerDeg;
                        if (DragFloat3("Rotation (deg)", edited, 0.1f))
                        {
                            const glm::vec3 rad = glm::radians(edited);
                            glm::quat q = glm::normalize(glm::quat(rad));
                            if (glm::any(glm::epsilonNotEqual(q, tr.Rotation, 1e-6f)))
                                tr.Rotation = q;
                        }
                        if (ImGui::IsItemHovered())
                            ImGui::SetTooltip("Orientation (degrees about X, Y, Z). Stored internally as a normalized quaternion.");
                    }

                    {
                        glm::vec3 scale = tr.Scale;
                        if (DragFloat3("Scale (m)", scale, 0.1f))
                            tr.Scale = scale;
                    }

                    EndPropertyGrid();
                    ImGui::TreePop();
                }
            }


            if (selectedEntity->HasComponent<MeshComponent>())
            {
                if (ImGui::Begin(ICON_MD_IMAGE " Material Editor"))
                {
                    if (!selectedEntity || selectedEntity == EntityFactory::EMPTYENTITY)
                    {
                        ImGui::TextDisabled(ICON_MD_INFO " No entity selected.");
                    }
                    else if (!selectedEntity->HasComponent<MeshComponent>())
                    {
                        ImGui::TextDisabled(ICON_MD_INFO " Selected entity has no Static Mesh Component.");
                    }
                    else
                    {
                        auto& smc = selectedEntity->GetComponent<MeshComponent>();
                        if (!smc.Model)
                        {
                            ImGui::TextDisabled(ICON_MD_INFO " Entity has no model.");
                        }
                        else
                        {
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

                            if (ImGui::BeginTable("##content-splitted", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoBordersInBody))
                            {
                                ImGui::TableSetupColumn("##side-bar", ImGuiTableColumnFlags_WidthFixed, 260.0f);
                                ImGui::TableSetupColumn("##inspector-panel", ImGuiTableColumnFlags_WidthStretch);
                                ImGui::TableNextRow();
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

            if (selectedEntity->HasComponent<RigidBodyComponent>())
            {
                auto& rb = selectedEntity->GetComponent<RigidBodyComponent>();
                if (ImGui::TreeNodeEx((void*)rb.ID, treeNodeFlags, ICON_MD_3D_ROTATION " Rigid Body"))
                {
                    BeginPropertyGrid("rigid-body");
                    auto& cc = selectedEntity->GetComponent<ColliderComponent>();

                    std::int32_t selected{static_cast<std::int32_t>(rb.Type)};
                    ImGui::BeginDisabled(cc.Type == ShapeType::Concave);

                    if(cc.Type == ShapeType::Concave)
                    {
                        selected = 0;
                        if(rb.Type == BodyType::Dynamic) 
                        {
                            rb.Type     = BodyType::Static;
                            auto* body  = rb.PhysicsBody;
                            body->setType(rp3d::BodyType::STATIC);
                        }
                    }
                    
                    ComboBox("Interaction",  { "Static", "Dynamic" }, selected, [&](std::int32_t selectedIndex, const std::string& selectedItem)
                    {
                        if(selectedIndex == 0)
                        {
                            rb.Type     = BodyType::Static;
                            auto* body  = rb.PhysicsBody;
                            body->setType(rp3d::BodyType::STATIC);
                        };

                        if(selectedIndex == 1)
                        {
                            rb.Type     = BodyType::Dynamic;
                            auto* body  = rb.PhysicsBody;
                            body->setType(rp3d::BodyType::DYNAMIC);
                        };
                    });

                    auto* body = rb.PhysicsBody;
                    
                    float mass = (float)body->getMass();
                    if(SliderFloat("Mass", &mass, 0.0001f, 1000000.0f))
                    {
                        body->setMass(mass);
                    }

                    float linearDamping = (float)body->getLinearDamping();
                    if(SliderFloat("Linear Damping", &linearDamping, 0.0f, 1.0f))
                    {
                        body->setLinearDamping(linearDamping);
                    }

                    float angularDamping = (float)body->getAngularDamping();
                    if(SliderFloat("Angular Damping", &angularDamping, 0.0f, 1.0f))
                    {
                        body->setAngularDamping(angularDamping);
                    }

                    ImGui::EndDisabled();
                    EndPropertyGrid();
                    ImGui::TreePop();
                }
            }

            if(selectedEntity->HasComponent<ColliderComponent>())
            {
                auto& cc = selectedEntity->GetComponent<ColliderComponent>();
                if (ImGui::TreeNodeEx((void*)cc.ID, treeNodeFlags, ICON_FA_BOX " Collision"))
                {
                    BeginPropertyGrid("collider");

                    std::int32_t selected{static_cast<std::int32_t>(cc.Type)};
                    ComboBox("Type",  { "Box", "Sphere", "Capsule", "Convex", "Concave" }, selected, [&](std::int32_t selectedIndex, const std::string& selectedItem)
                    {
                        if(cc.Type != static_cast<ShapeType>(selectedIndex))
                            KinetiX::GetInstance().ChangeCollider(selectedEntity, static_cast<ShapeType>(selectedIndex));
                    });

                    float bounce = (float)cc.Attributes->getBounciness();
                    if(SliderFloat("Bounce", &bounce, 0.0f, 1.0f))
                    {
                        cc.Attributes->setBounciness(bounce);
                    }

                    float friction = (float)cc.Attributes->getFrictionCoefficient();
                    if(SliderFloat("Friction", &friction, 0.0f, 1.0f))
                    {
                        cc.Attributes->setFrictionCoefficient(friction);
                    }

                    float density = (float)cc.Attributes->getMassDensity();
                    if(SliderFloat("Density", &density, 0.0f, 1.0f))
                    {
                        cc.Attributes->setMassDensity(density);
                    }

                    EndPropertyGrid();
                    ImGui::TreePop();
                }
            }

        }
        else
        {
            ImGui::TextDisabled(ICON_MD_INFO" Entity is not active.");
        }

        ImGui::EndDisabled();
        ImGui::End();
    }
}