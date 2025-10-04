#include "CorePCH.hpp"

#include "ProjectSceneViewPanel.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    SceneViewPanel::SceneViewPanel()
    {
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Metal/Base.yaml"));
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Marble/Base.yaml"));
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Plastic/Base.yaml"));
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Rubber/Base.yaml"));
        m_BaseMaterial.push_back(Material::CreateBase("Assets/Materials/Stone/Base.yaml"));
    }

    void SceneViewPanel::DrawMaterialUI(ScenePanelContext&, std::shared_ptr<Material>& mat)
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

    void SceneViewPanel::DrawAttributes(std::shared_ptr<Material>& mat)
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

    void SceneViewPanel::DrawTexturesSlots(std::shared_ptr<Material>& mat)
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

    void SceneViewPanel::RenderUI(ScenePanelContext& context)
    {
        ImGui::Begin("Project Scenes");

        static char s_SearchBuf[128] = {};
        {
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 38.0f);
            ImGui::InputTextWithHint("##SearchScenes", ICON_MD_SEARCH " Search scenes...", s_SearchBuf, sizeof(s_SearchBuf));
            ImGui::PopItemWidth();

            ImGui::SameLine();
            if (ImGui::Button(ICON_MD_ADD "##AddScene"))
            {
                ImGui::OpenPopup("New Scene");
            }
            ImGui::Separator();
        }

        if (ImGui::BeginPopupModal("New Scene", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char s_NewSceneName[128] = {};
            static bool s_SetActive = true;
            static bool s_Init = true;
            static std::string s_Error;

            if (ImGui::IsWindowAppearing() || s_Init)
            {
                s_Init = false;
                s_Error.clear();
                static int s_Counter = 1;
                std::snprintf(s_NewSceneName, sizeof(s_NewSceneName), "New Scene %d", s_Counter++);
                s_SetActive = true;
                ImGui::SetKeyboardFocusHere();
            }

            ImGui::TextUnformatted("Scene name:");
            ImGui::SetNextItemWidth(320.0f);
            bool enterPressed = ImGui::InputText("##scene_name", s_NewSceneName, sizeof(s_NewSceneName), ImGuiInputTextFlags_EnterReturnsTrue);

            ImGui::Checkbox("Set active after creating", &s_SetActive);
            auto isBlank = [](const char* s)
            {
                for (const char* p = s; *p; ++p) if (!std::isspace((unsigned char)*p)) return false;
                return true;
            };

            auto nameExists = [&](const std::string& n)
            {
                for (const std::shared_ptr<Scene>& sc : *context.EditorInstance)
                    if (sc->GetSpecification().Name == n) return true;
                
                return false;
            };

            if (!s_Error.empty())
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(1, 0.35f, 0.35f, 1), "%s", s_Error.c_str());
            }

            ImGui::Separator();

            auto tryCreate = [&]() 
            {
                std::string name = s_NewSceneName;
                if (isBlank(name.c_str()))
                {
                    s_Error = "Name cannot be empty.";
                    return false;
                }
                if (nameExists(name))
                {
                    s_Error = "A scene with this name already exists.";
                    return false;
                }
                context.EditorInstance->AddNewScene(name, s_SetActive);
                s_Error.clear();
                s_Init = true;
                ImGui::CloseCurrentPopup();
                return true;
            };

            bool createClicked = ImGui::Button("Create", ImVec2(100, 0));
            if (createClicked || enterPressed)
                tryCreate();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 0)))
            {
                s_Error.clear();
                s_Init = true;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }

        ImGuiTreeNodeFlags nodeFlags = 
            ImGuiTreeNodeFlags_DefaultOpen      | 
            ImGuiTreeNodeFlags_Framed           | 
            ImGuiTreeNodeFlags_SpanAvailWidth   | 
            ImGuiTreeNodeFlags_AllowItemOverlap | 
            ImGuiTreeNodeFlags_FramePadding;

        auto ci_contains = [](std::string hay, std::string needle)
        {
            std::transform(hay.begin(), hay.end(), hay.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c) { return (char)std::tolower(c); });
            return needle.empty() || (hay.find(needle) != std::string::npos);
        };


        static std::future<std::shared_ptr<ImportedResults>> s_ImportedResults;
        static bool s_RequestEntityImport   = false;
        static bool s_RequestSceneRename    = false;
        static bool s_RequestSceneDelete    = false;

        for (auto& scene : *context.EditorInstance)
        {
            ImGui::PushID(scene.get());
        
            const bool isActive = scene->IsActive();
            std::string label = std::format("{}  {}[{:X}]{}", ICON_MD_DASHBOARD, scene->GetName(), scene->GetID(), isActive ? std::string("  ") + ICON_MD_STAR : "");

            if (!ci_contains(label, std::string(s_SearchBuf)))
            {
                ImGui::PopID();
                continue;
            }

            static const ImGuiTreeNodeFlags treeNodeFlags = 
                ImGuiTreeNodeFlags_Framed | 
                ImGuiTreeNodeFlags_SpanAvailWidth | 
                ImGuiTreeNodeFlags_AllowItemOverlap | 
                ImGuiTreeNodeFlags_FramePadding;

            bool open = ImGui::TreeNodeEx((void*)scene->GetID(), treeNodeFlags, label.c_str());
            if (ImGui::IsItemClicked())
            {
                scene->SelectEntityIf();
                context.ActiveScene = scene;                       
                context.EditorInstance->SetActiveScene(scene); 
            }

            if(ImGui::IsItemClicked(ImGuiMouseButton_Right))
            {
                ImGui::OpenPopup("SceneContext");
            }

            if (ImGui::BeginPopup("SceneContext"))
            {
                if (ImGui::MenuItem(ICON_MD_FILE_UPLOAD "  Import Model"))
                {
                    ImGui::CloseCurrentPopup();
                    DialogBoxes::InitializeCOM();

                    OpenDialogOptions options{};
                    options.Title = L"Import Model";
                    options.DefaultExtension = L"obj";
                    options.AllowMultiSelect = false;
                    options.InitialDirectory = std::filesystem::current_path();
                    options.Filters = 
                    {
                        {L"Mesh Files", L"*.fbx;*.obj;*.gltf;*.glb"},
                        {L"All Files",  L"*.*"}
                    };

                    if (auto path = DialogBoxes::OpenFileDialog(options); !path.empty()) 
                    {
                        s_ImportedResults = std::async(std::launch::async, [path]
                        {
                            return Importer::ImportModelAsync(path, false, "default");
                        });

                        s_RequestEntityImport = true;
                    }

                    DialogBoxes::UninitializeCOM();
                }
                if (ImGui::MenuItem(ICON_MD_EDIT "  Rename"))
                {
                    ImGui::CloseCurrentPopup();
                    s_RequestSceneRename = true;
                }
                if (ImGui::MenuItem(ICON_MD_DELETE "  Delete"))
                {
                    ImGui::CloseCurrentPopup();
                    s_RequestSceneDelete = true;
                }
                ImGui::EndPopup();
            }
            {
                if (s_RequestSceneRename)
                {
                    ImGui::OpenPopup("RenameScene##Popup");
                    s_RequestSceneRename = false;
                }
                if (s_RequestSceneDelete)
                {
                    ImGui::OpenPopup("DeleteScene##Popup");
                    s_RequestSceneDelete = false;
                }
                if (s_RequestEntityImport)
                {
                    ImGui::OpenPopup("ImportEntity##Popup");
                    s_RequestEntityImport = false;
                }
    
                if (ImGui::BeginPopupModal("RenameScene##Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    static char s_RenameBuf[128] = {};
                    if (ImGui::IsWindowAppearing())
                    {
                        memset(s_RenameBuf, 0, sizeof(s_RenameBuf));
                        const std::string& n = scene->GetSpecification().Name;
                        strncpy(s_RenameBuf, n.c_str(), sizeof(s_RenameBuf) - 1);
                        ImGui::SetKeyboardFocusHere();
                    }
    
                    ImGui::TextUnformatted("New scene name:");
                    ImGui::InputText("##rename", s_RenameBuf, sizeof(s_RenameBuf));
    
                    ImGui::Separator();
                    if (ImGui::Button("OK", { 80,0 }))
                    {
                        scene->GetSpecification().Name = std::string(s_RenameBuf);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", { 80,0 }))
                    {
                        ImGui::CloseCurrentPopup();
                    }
    
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("DeleteScene##Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::TextWrapped("%s  Delete scene \"%s\"?\nThis cannot be undone.", ICON_MD_WARNING, scene->GetName().c_str());
                    ImGui::Separator();
    
                    if (ImGui::Button("Delete", { 80,0 }))
                    {
                        auto id = scene->GetID();
                        ImGui::CloseCurrentPopup();
                        context.EditorInstance->DeleteScene(id);
                        ImGui::EndPopup(); 
                        ImGui::PopID();
                        break; 
                    }
    
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", { 80,0 }))
                    {
                        ImGui::CloseCurrentPopup();
                    }
    
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopupModal("ImportEntity##Popup", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
                {
                    float t = float(fmod(ImGui::GetTime(), 1.0));
                    float p = 0.25f + 0.5f * (0.5f - std::abs(t - 0.5f)) * 2.0f;
    
                    ImGui::TextUnformatted("Crunching triangles and untangling meshes…");
                    ImGui::Dummy(ImVec2(0, 8));
                    ImGui::ProgressBar(p, ImVec2(320, 0), "Working");
    
                    using namespace std::chrono_literals;
                    if (s_ImportedResults.valid() && s_ImportedResults.wait_for(0ms) == std::future_status::ready)
                    {
                        auto result = s_ImportedResults.get(); 
                        if (result) 
                        {
                            auto& KX = KinetiX::GetInstance();
                            std::shared_ptr<Entity> root = Entity::Create(result->Name);
    
                            const BufferLayout layout
                            {
                                { "a_Position",   BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Position)    },
                                { "a_TexCoords",  BufferComponents::UV,   BufferStride::F2, false, offsetof(Vertex, TexCoord)    },
                                { "a_Normals",    BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Normal)      },
                                { "a_Tangents",   BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)     },
                                { "a_Bitangents", BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Bitangent)   },
                            };
    
                            auto& node  = root->Get<NodeComponent>();
                            node.IsRoot = true;
                            
                            bool nextNodeSet{false};
                            std::shared_ptr<Entity> lastEntt{nullptr};
    
                            for (const auto& [meshID, mesh] : result->Meshes)
                            {
                                auto entt = Entity::Create(std::format("{}", mesh.Name));
                                if (!nextNodeSet)
                                {
                                    node.EnTTNext = entt;
                                    nextNodeSet = true;
                                }
    
                                if(lastEntt) 
                                {
                                    lastEntt->Get<NodeComponent>().EnTTNext     = entt;
                                    lastEntt->Get<NodeComponent>().IsRoot       = false;
                                }
                                
                                auto& meshComponent = entt->Emplace<MeshComponent>();
    
                                auto meshPtr = Mesh::Create(
                                    mesh.Vertices.data(), static_cast<std::uint32_t>(mesh.Vertices.size()), 
                                    mesh.Indices.data(),  static_cast<std::uint32_t>(mesh.Indices.size()), 
                                    layout
                                );
    
                                auto bounds = ModelBounds(mesh.Vertices);
                                meshPtr->MIN = bounds.first;
                                meshPtr->MAX = bounds.second;
    
                                std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), std::back_inserter(meshPtr->Positions), [](const Vertex& v) { return v.Position; });
                                meshPtr->Faces.insert(meshPtr->Faces.end(), mesh.Indices.begin(), mesh.Indices.end());
                                meshComponent.MeshPointer = std::move(meshPtr);
    
                                entt->Emplace<TransformComponent>();
                                entt->Emplace<RigidBodyComponent>();
                                entt->Emplace<ColliderComponent>();
                                
                                auto& material              = entt->Emplace<MaterialComponent>();
                                material.MaterialPointer    = Material::Create();
    
                                KX.CreateRigidBody(entt);
                                KX.CreateConvexCollider(entt, meshComponent.MeshPointer->Positions, meshComponent.MeshPointer->Faces);
                                lastEntt = entt;
                            }
    
                            scene->EmplaceEntity(root);
                            scene->SelectedEntity(root);
                        } 
                        else
                        {
                            MOTION_ERROR("Failed to import model.");
                        }
    
                        ImGui::CloseCurrentPopup();
                        s_ImportedResults = std::future<std::shared_ptr<ImportedResults>>(); 
                    }
    
                    ImGui::EndPopup();
                }
            }

            if (open)
            {
                std::shared_ptr<Entity> selected = scene->GetSelectedEntity();
                for(auto& entt : scene->GetEntities())
                {
                    std::shared_ptr<Entity> current = entt;
                    while(current)
                    {
                        std::shared_ptr<Entity> next = nullptr;

                        bool openedHeader = ImGui::TreeNodeEx(current.get(), treeNodeFlags, current->Get<TagComponent>().Tag.c_str());
                        if (ImGui::IsItemClicked())
                        {
                            selected = current;
                            context.ActiveScene->SelectedEntity(selected);
                        }

                        if(openedHeader)
                        {
                            if(current->Has<NodeComponent>())
                            {
                                next = current->Get<NodeComponent>().EnTTNext;
                                if(current->Get<NodeComponent>().IsRoot)
                                {
                                    current = next;
                                    continue;
                                }
                            }
                        
                            bool isActive{false};
                            BeginPropertyGrid("##tag-grid");

                            auto& tag = entt->Get<TagComponent>();
                            TextBox("Name Tag", tag.Tag, false);
                            isActive = tag.IsActive;
                            ToggleSwitch("Is Active", tag.IsActive);

                            EndPropertyGrid();

                            if(isActive)
                            {
                                auto& TRC = current->Get<TransformComponent>();
                                auto& RBC = current->Get<RigidBodyComponent>();
                                auto& CC  = current->Get<ColliderComponent>();
                                auto& MC  = current->Get<MeshComponent>();
                                auto& MTC = current->Get<MaterialComponent>();

                                BeginPropertyGrid("##physics-grid");

                                {
                                    glm::vec3 posM = TRC.Translation;                
                                    if (DragFloat3("Position (m)", posM, 0.01f))
                                    {
                                        TRC.Translation = posM;
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
                                }
                                {
                                    glm::vec3 scale = TRC.Scale;
                                    if (DragFloat3("Scale (m)", scale, 0.1f))
                                    {
                                        TRC.Scale = scale;
                                    }
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

                                    auto* body = RBC.PhysicsBody;
                                    
                                    float mass = (float)body->getMass();
                                    if(DragFloat("Compute Mass", &mass, 0.001f, 0.0000000001f, FLT_MAX))
                                    {
                                        body->setMass(Units::ToKilograms(mass));
                                    }

                                    float linearDamping = (float)body->getLinearDamping();
                                    if(DragFloat("Linear Damping", &linearDamping, 0.001f, 0.0f, 1.0f))
                                    {
                                        body->setLinearDamping(Units::ToMetersPerSecond(linearDamping));
                                    }

                                    float angularDamping = (float)body->getAngularDamping();
                                    if(DragFloat("Angular Damping", &angularDamping, 0.001f, 0.0f, 1.0f))
                                    {
                                        body->setAngularDamping(Units::ToMetersPerSecond(angularDamping));
                                    }
                                }
                                {
                                    float bounce = CC.Restitution;
                                    if(DragFloat("Bounce", &bounce, 0.001f, 0.0f, 1.0f))
                                    {
                                        auto& material = CC.Collider->getMaterial();
                                        material.setBounciness(bounce);
                                        CC.Restitution = bounce;
                                    }

                                    float friction = CC.Friction;
                                    if(DragFloat("Friction", &friction, 0.001f, 0.0f, 1.0f))
                                    {
                                        auto& material = CC.Collider->getMaterial();
                                        material.setBounciness(friction);
                                        CC.Friction = friction;
                                    }

                                    float density = CC.MassDensity;
                                    if(DragFloat("Density", &density, 0.01f, 0.0f, FLT_MAX))
                                    {
                                        auto& material = CC.Collider->getMaterial();
                                        material.setMassDensity(density);
                                        CC.MassDensity = density;
                                    } 
                                }
                                {
                                    static bool isEditorOpen = false;
                                    if (ImGui::Button(ICON_MD_IMAGE " Material Editor"))
                                        isEditorOpen = !isEditorOpen;

                                    if (isEditorOpen)
                                    {
                                        if (ImGui::Begin(ICON_MD_IMAGE " Material Editor", &isEditorOpen, ImGuiWindowFlags_NoDocking))
                                        {
                                            if (ImGui::BeginChild("##inspector-area", ImVec2(0.0f, 0.0f)))
                                            {
                                                if (MTC.MaterialPointer)
                                                    DrawMaterialUI(context, MTC.MaterialPointer);
                                                else
                                                    ImGui::TextDisabled(ICON_MD_INFO " No material assigned");

                                                ImGui::EndChild(); 
                                            }

                                            ImGui::End();
                                        }
                                    }
                                }

                                EndPropertyGrid();
                            }

                            ImGui::TreePop();
                        }

                        current = next;
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::End();
    }
}