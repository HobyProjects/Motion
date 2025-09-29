#include "CorePCH.hpp"
#include "PropertiesPanel.hpp"

namespace Motion
{
    SceneEntityPropertiesPanel::SceneEntityPropertiesPanel()
    {
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Metal/Base.yaml"));
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Marble/Base.yaml"));
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Plastic/Base.yaml"));
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Rubber/Base.yaml"));
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Stone/Base.yaml"));
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

            std::ranges::transform(m_BaseMaterial, std::back_inserter(materialNames), [](const auto& matPtr) { return matPtr->Name; });

            std::int32_t index = 0;
            if (base)
            {
                auto it = std::ranges::find(materialNames, base->Name);
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
                    return matPtr->Name == selectedName;
                });

                if (it != m_BaseMaterial.end())
                    mat->SetBaseMaterial(*it);
            });

            EndPropertyGrid();
        }

        ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

        if(mat->Has<CoreMaterialComponents>())
        {
            auto& C = mat->Get<CoreMaterialComponents>();

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
        if(mat->Has<CoreMaterialComponents>())
        {
            auto& C = mat->Get<CoreMaterialComponents>();

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

        std::shared_ptr<Entity> selectedEntity  = ctx.ActiveScene ? ctx.ActiveScene->GetSelectedEntity() : nullptr;
        const bool hasSelection                 = selectedEntity && selectedEntity != Entity::Empty();

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
        if (selectedEntity->Has<TagComponent>())
        {
            BeginPropertyGrid("##tag-grid");
            auto& tag = selectedEntity->Get<TagComponent>();
            TextBox("Name Tag", tag.Tag, false, 256);

            isActive = tag.IsActive;
            ToggleSwitch("Is Active", tag.IsActive);
            EndPropertyGrid();
        }

        if(isActive && !selectedEntity->Get<NodeComponent>().IsRoot)
        {
            ImGui::TreeNodeEx(selectedEntity.get(), treeNodeFlags, ICON_FA_CUBES " Physics Properties");

            auto& TRC = selectedEntity->Get<TransformComponent>();
            auto& RBC = selectedEntity->Get<RigidBodyComponent>();
            auto& CC  = selectedEntity->Get<ColliderComponent>();
            auto& MC  = selectedEntity->Get<MeshComponent>();
            auto& MTC = selectedEntity->Get<MaterialComponent>();

            BeginPropertyGrid("##physics-grid");

            {
                glm::vec3 posM = TRC.Translation;                
                if (DragFloat3("Position (m)", posM, 0.01f))
                {
                    TRC.Translation = posM;
                }

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "World-space position in meters.\n"
                        "Units: 1 = 1 meter.\n"
                        "Used for rendering and (for static/kinematic bodies) also drives the physics pose."         
                    );
                }

            }

            {
                glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(TRC.Rotation));
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
                    if (glm::any(glm::epsilonNotEqual(q, TRC.Rotation, 1e-6f))) TRC.Rotation = q;
                }

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "World-space orientation in degrees (XYZ Euler shown for editing).\n"
                        "Internally stored as a unit quaternion.\n"
                        "Tip: avoid huge per-frame swings to reduce tunneling."             
                    );
                }
            }

            {
                glm::vec3 scale = TRC.Scale;
                if (DragFloat3("Scale (m)", scale, 0.1f))
                {
                    TRC.Scale = scale;
                }

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Visual scale only. Physics uses collider SHAPE size.\n"
                        "If you change visual scale, rebuild/rescale the collision shape to match."              
                    );
                }

            }

            {
                if (ImGui::Begin(ICON_MD_IMAGE " Material Editor"))
                {
                    if (ImGui::BeginChild("##inspector-area", ImVec2(0.0f, 0.0f), true))
                    {
                        if (MTC.MaterialPointer)
                            DrawMaterialUI(ctx, MTC.MaterialPointer);
                        else
                            ImGui::TextDisabled(ICON_MD_INFO " No material assigned");
                    }

                    ImGui::EndChild();
                }
                
                ImGui::End();
            }

            {
                std::int32_t selected{static_cast<std::int32_t>(RBC.Type)};

                if(CC.Type == ShapeType::Concave)
                {
                    selected = 0;
                    if(RBC.Type == BodyType::Dynamic) 
                    {
                        RBC.Type     = BodyType::Static;
                        auto* body   = RBC.PhysicsBody;
                        body->setType(rp3d::BodyType::STATIC);
                    }
                }
                
                ComboBox("Interaction",  { "Static", "Dynamic" }, selected, [&](std::int32_t selectedIndex, const std::string& selectedItem)
                {
                    if(selectedIndex == 0)
                    {
                        RBC.Type     = BodyType::Static;
                        auto* body   = RBC.PhysicsBody;
                        body->setType(rp3d::BodyType::STATIC);
                    };

                    if(selectedIndex == 1)
                    {
                        RBC.Type     = BodyType::Dynamic;
                        auto* body   = RBC.PhysicsBody;
                        body->setType(rp3d::BodyType::DYNAMIC);
                    };
                });

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Static: infinite mass, not affected by forces; used for level geometry.\n"
                        "Dynamic: simulated by forces/impulses; has finite mass & inertia."
                        
                    );
                }

                auto* body = RBC.PhysicsBody;
                
                float mass = (float)body->getMass();
                if(DragFloat("Compute Mass", &mass, 0.001f, 0.0000000001f, FLT_MAX))
                {
                    body->setMass(Units::ToKilograms(mass));
                }

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Mass in kilograms.\n"
                        "If you use collider density, prefer auto-compute:\n"
                        "  mass = Σ (density_i × volume_i)\n"
                        "and call RigidBody::updateMassPropertiesFromColliders().\n"
                        "Manual mass overrides only the scalar mass (not the inertia tensor)."
                    );
                }

                float linearDamping = (float)body->getLinearDamping();
                if(DragFloat("Linear Damping", &linearDamping, 0.001f, 0.0f, 1.0f))
                {
                    body->setLinearDamping(Units::ToMetersPerSecond(linearDamping));
                }

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Fractional drag on linear velocity (unitless per-step factor).\n"
                        "Continuous model: dv/dt = -β v  ⇒  v(t) = v0 · e^{-β t}\n"
                        "Discrete step Δt:   v_{k+1} ≈ v_k · (1 - β·Δt)  (small β·Δt)."
                    );
                }

                float angularDamping = (float)body->getAngularDamping();
                if(DragFloat("Angular Damping", &angularDamping, 0.001f, 0.0f, 1.0f))
                {
                    body->setAngularDamping(Units::ToMetersPerSecond(angularDamping));
                }

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Drag on angular velocity (spin). Same model as linear damping:\n"
                        " • dω/dt = -β ω  ⇒  ω(t) = ω0 · e^{-β t}.\n"
                        " • Use small values (≈0.05–0.3) to calm jitter without syrupy motion."
                    );
                }
            }

            {
                std::int32_t selected{static_cast<std::int32_t>(CC.Type)};
                ComboBox("Type",  { "Box", "Sphere", "Capsule", "Convex" }, selected, [&](std::int32_t selectedIndex, const std::string& selectedItem)
                {
                    if(CC.Type != static_cast<ShapeType>(selectedIndex))
                        KinetiX::GetInstance().ChangeCollider(selectedEntity, static_cast<ShapeType>(selectedIndex));
                });

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Collision shape used for contacts:\n"
                        " • Box: half-extents (hx,hy,hz)\n"
                        " • Sphere: radius r\n"
                        " • Capsule: radius r, height h (cylindrical section)\n"
                        " • Convex: mesh hull (can bake non-uniform scale)"
                    );
                }

                ImGui::BeginDisabled();
                
                if(CC.Type == ShapeType::Box)
                {
                    DragFloat3("Half-Extents", CC.BoxHalfExtents);
                }

                if(CC.Type == ShapeType::Sphere)
                {
                    DragFloat("Radius", &CC.SphereRadius);
                }

                if(CC.Type == ShapeType::Capsule)
                {
                    DragFloat("Radius", &CC.Capsule.Radius);
                    DragFloat("Height", &CC.Capsule.Height);
                }

                if(CC.Type == ShapeType::Convex)
                {
                    DragFloat3("Scale", TRC.Scale);
                }

                ImGui::EndDisabled();

                ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, 0.8f);

                float bounce = CC.Restitution;
                if(DragFloat("Bounce", &bounce, 0.001f, 0.0f, 1.0f))
                {
                    auto& material = CC.Collider->getMaterial();
                    material.setBounciness(bounce);
                    CC.Restitution = bounce;
                }

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Coefficient of restitution e (0–1): how bouncy contacts are.\n"
                        "Normal relative speed after impact:  v_out = -e · v_in.\n"
                        "e=0: perfectly inelastic (thud).  e=1: perfectly elastic."
                    );
                }

                float friction = CC.Friction;
                if(DragFloat("Friction", &friction, 0.001f, 0.0f, 1.0f))
                {
                    auto& material = CC.Collider->getMaterial();
                    material.setBounciness(friction);
                    CC.Friction = friction;
                }

                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Coulomb friction coefficient μ (unitless).\n"
                        "Tangential friction force capped by |F_t| ≤ μ · N (N = normal force).\n"
                        "Higher μ = grippier contact; typical 0.1–0.9."
                    );
                }

                float density = CC.MassDensity;
                if(DragFloat("Density", &density, 0.01f, 0.0f, FLT_MAX))
                {
                    auto& material = CC.Collider->getMaterial();
                    material.setMassDensity(density);
                    CC.MassDensity = density;
                } 
                
                if (ImGui::IsItemHovered()) 
                {
                    ImGui::SetItemTooltip(
                        "Mass density ρ in kg/m³ (material property).\n"
                        "Collider mass contribution: m_i = ρ_i · V_i.\n"
                        "Total body mass = Σ m_i; inertia tensor computed from the shapes.\n"
                        "Call body->updateMassPropertiesFromColliders() after changing density or collider size."
                    );
                }
            }


            EndPropertyGrid();
            ImGui::TreePop();

        }
        else
        {
            ImGui::TextDisabled(ICON_MD_INFO" Entity is not active.");
        }

        ImGui::EndDisabled();
        ImGui::End();
    }
}