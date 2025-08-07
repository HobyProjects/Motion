#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion
{
    //-------------------------------------------------------------------
    // HELPER FUNCTIONS 
    //-------------------------------------------------------------------

    static std::shared_ptr<ITexture> LoadTexture(TextureType type)
    {
        std::filesystem::path file = DialogBoxes::OpenFileDialog();
        if (!file.empty())
        {
            auto newTex = ITexture::Create(file, type, true);
            if (newTex)
            {
                return newTex; // Return the loaded texture
            }
            else
            {
                MOTION_ERROR("Failed to load texture from file: {0}", file.string());
                return nullptr;
            }
        }

        MOTION_ERROR("No file selected for texture loading");
        return nullptr; // Return null if no file was selected
    }

    //-------------------------------------------------------------------



    Scene::Scene(SceneHandle handle, const std::string& name, const glm::vec2& viewportSize)
    {
        m_SceneID = handle;
        m_Name = name;
        m_SceneCamera = SceneCamera(viewportSize.x, viewportSize.y, false);
    }

    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        m_SceneCamera.OnUpdate(handle, deltaTime);
    }

    void Scene::OnEvent(WindowHandle handle, IEvent& e)
    {
        m_SceneCamera.OnEvents(handle, e);
    }

    void Scene::OnUIRenders(WindowHandle handle)
    {
        RenderEntities(handle);
    }

    void Scene::OnViewportSizeChanges(float width, float height)
    {
        m_SceneCamera.SetAspectRatio(width, height);
    }

    std::shared_ptr<Entity> Scene::PickEntity(const glm::vec2& mousePos, const glm::vec2& viewportSize)
    {
        const glm::mat4& projection = GetProjectionMatrix();
        const glm::mat4& view = GetViewMatrix();

        float x = (2.0f * mousePos.x) / viewportSize.x - 1.0f;
        float y = 1.0f - (2.0f * mousePos.y) / viewportSize.y; // GL Y is inverted

        glm::vec4 rayStartNDC(x, y, -1.0f, 1.0f);
        glm::vec4 rayEndNDC(x, y, 1.0f, 1.0f);

        glm::mat4 invVP = glm::inverse(projection * view);
        glm::vec4 rayStartWorld = invVP * rayStartNDC; rayStartWorld /= rayStartWorld.w;
        glm::vec4 rayEndWorld = invVP * rayEndNDC;   rayEndWorld /= rayEndWorld.w;

        glm::vec3 rayOrigin = glm::vec3(rayStartWorld);
        glm::vec3 rayDir = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

        // 2. Find closest entity hit by ray
        float closestT = FLT_MAX;
        std::shared_ptr<Entity> pickedEntity = nullptr;

        for (const auto& entity : m_Entities)
        {
            if (!entity->HasComponent<StaticMeshComponent>() || !entity->HasComponent<TransformComponent>())
                continue;

            auto& meshComp = entity->GetComponent<StaticMeshComponent>();
            auto& transComp = entity->GetComponent<TransformComponent>();

            glm::vec3 meshMin = meshComp.Model->GetMinBounds(); // local-space min
            glm::vec3 meshMax = meshComp.Model->GetMaxBounds(); // local-space max

            // Translation only (no scale/rotation):
            glm::vec3 boxMin = meshMin + transComp.Translation;
            glm::vec3 boxMax = meshMax + transComp.Translation;

            float tmin, tmax;
            if (RayIntersectsAABB(rayOrigin, rayDir, boxMin, boxMax, tmin, tmax))
            {
                float hitDist = (tmin > 0.0f) ? tmin : tmax; // If tmin behind, try tmax
                if (hitDist < closestT && hitDist > 0.0f)
                {
                    closestT = hitDist;
                    pickedEntity = entity;
                }
            }

        }

        return pickedEntity;
    }

    void Scene::RenderEntities(WindowHandle handle)
    {
        ImGui::Begin("Scene Entities");
        if (ImGui::BeginPopupContextWindow(0, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Import StaticMesh"))
            {
                //[TODO]: This should happen on different thread
                std::filesystem::path filePath = DialogBoxes::OpenFileDialog();
                if (!filePath.empty())
                {
                    std::shared_ptr<StaticMesh> staticMesh = Importer::ImportModel(filePath);
                    if (staticMesh)
                    {
                        auto& entityFactory = EntityFactory::GetInstance();
                        std::shared_ptr<Entity> entity = entityFactory.CreateEntity(staticMesh->GetName());

                        entity->AddComponent<StaticMeshComponent>(staticMesh->GetName(), staticMesh);
                        entity->AddComponent<TransformComponent>();
                        m_Entities.emplace_back(entity);
                    }
                    else
                    {
                        MOTION_ERROR("Failed to load static Mesh from file: {0}", filePath.string());
                    }
                }
            }
            ImGui::EndPopup();
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowHovered())
        {
            m_SelectedEntity = EntityFactory::EMPTYENTITY;
        }

        for (uint32_t i = 0; i < m_Entities.size(); i++)
        {
            std::shared_ptr<Entity> entity = m_Entities[i];
            auto& tag = entity->GetComponent<TagComponent>();
            ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0)
                | ImGuiTreeNodeFlags_OpenOnArrow
                | ImGuiTreeNodeFlags_SpanAvailWidth
                | ImGuiTreeNodeFlags_Framed
                | ImGuiTreeNodeFlags_FramePadding;

            bool open = ImGui::TreeNodeEx((void*)tag.ID, flags, tag.Tag.c_str());
            if (ImGui::IsItemClicked())
                m_SelectedEntity = entity;

            if (open)
            {
                if (entity->HasComponent<StaticMeshComponent>() && open)
                {
                    auto& model = entity->GetComponent<StaticMeshComponent>().Model;
                    if (model)
                    {
                        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;
                        if (ImGui::CollapsingHeader("Mesh Details", nodeFlags))
                        {
                            if (ImGui::IsItemClicked())
                                m_SelectedEntity = entity;

                            std::string meshCount = std::to_string(model->GetMeshesCount());
                            std::string minBounds = glm::to_string(model->GetMinBounds());
                            std::string maxBounds = glm::to_string(model->GetMaxBounds());
                            std::string filePath = model->GetSource();

                            CustomUIControl::TextBox("Mesh Count", meshCount, true);
                            CustomUIControl::TextBox("Min Bounds", minBounds, true);
                            CustomUIControl::TextBox("Max Bounds", maxBounds, true);
                            CustomUIControl::TextBox("File Path", filePath, true);

                            static std::int32_t selected = 0;
                            CustomUIControl::ComboBox("Shading Method", selected, { "Standard", "Physical Based" });

                            if (selected == 0)
                                model->ModelShadingMethod = ShadingMethod::Standard;
                            if (selected == 1)
                                model->ModelShadingMethod = ShadingMethod::PhysicalBased;
                        }
                        if (ImGui::CollapsingHeader("Material Batch Assignment", nodeFlags))
                        {
                            if (ImGui::IsItemClicked())
                                m_SelectedEntity = entity;

                            if (model->ModelShadingMethod == ShadingMethod::PhysicalBased)
                            {
                                static PhysicalBasedMaterialAttribute batchAttri{};
                                static std::unordered_map<TextureType, std::shared_ptr<ITexture>> batchTextures;

                                bool attributesChanged = false;
                                attributesChanged |= CustomUIControl::DrawColor3("Base Color", batchAttri.BaseColor);
                                attributesChanged |= CustomUIControl::DrawFloat("Metallic", batchAttri.Metallic, 0.0f, 1.0f, 0.0005f);
                                attributesChanged |= CustomUIControl::DrawFloat("Roughness", batchAttri.Roughness, 0.0f, 1.0f, 0.0005f);
                                attributesChanged |= CustomUIControl::DrawFloat("Opacity", batchAttri.Opacity, 0.0f, 1.0f, 0.0005f);

                                for (auto texType : { TextureType::BaseColorTexture, TextureType::MetallicTexture, TextureType::RoughnessTexture, TextureType::AmbientOcclusionTexture, TextureType::DisplacementTexture, TextureType::NormalTexture })
                                {
                                    ImGui::SameLine(0.0f, 14.0f);
                                    ImGui::PushID(static_cast<std::int32_t>(texType));
                                    CustomUIControl::TextureSlotCard(GetTextureTypeString(texType), batchTextures[texType],
                                        [&]()
                                        {
                                            auto newTex = LoadTexture(texType);
                                            if (newTex) batchTextures[texType] = newTex;
                                        });

                                    ImGui::PopID();
                                }

                                if (ImGui::Button("Apply"))
                                {
                                    for (auto& mesh : *model)
                                    {
                                        if (mesh->PhysicalBasedMaterials)
                                            mesh->PhysicalBasedMaterials->Attributes = batchAttri;

                                        for (auto& [type, tex] : batchTextures)
                                        {
                                            if (mesh->PhysicalBasedMaterials)
                                                mesh->PhysicalBasedMaterials->Texture[type] = tex;
                                        }
                                    }
                                }
                            }
                            else
                            {
                                static StandardMaterialAttribute batchAttri{};
                                static std::unordered_map<TextureType, std::shared_ptr<ITexture>> batchTextures;

                                bool attributesChanged = false;
                                attributesChanged |= CustomUIControl::DrawColor3("Diffuse Color", batchAttri.DiffuseColor);
                                attributesChanged |= CustomUIControl::DrawColor3("Specular Color", batchAttri.SpecularColor);
                                attributesChanged |= CustomUIControl::DrawColor3("Ambient Color", batchAttri.AmbientColor);
                                attributesChanged |= CustomUIControl::DrawColor3("Emissive Color", batchAttri.EmissiveColor);
                                attributesChanged |= CustomUIControl::DrawFloat("Shininess", batchAttri.Shininess, 0.0f, 32.0f, 0.0005f);
                                attributesChanged |= CustomUIControl::DrawFloat("Opacity", batchAttri.Opacity, 0.0f, 1.0f, 0.0005f);

                                int texCount = 0;
                                for (auto texType : { TextureType::DiffuseTexture, TextureType::SpecularTexture, TextureType::EmissiveTexture, TextureType::OpacityTexture })
                                {
                                    if (texCount++ > 0)
                                        ImGui::SameLine(0.0f, 14.0f);

                                    ImGui::PushID(static_cast<std::int32_t>(texType));
                                    CustomUIControl::TextureSlotCard(GetTextureTypeString(texType), batchTextures[texType],
                                        [&]()
                                        {
                                            auto newTex = LoadTexture(texType);
                                            if (newTex) batchTextures[texType] = newTex;
                                        });

                                    ImGui::PopID();

                                }

                                if (ImGui::Button("Apply"))
                                {
                                    for (auto& mesh : *model)
                                    {
                                        if (mesh->StandardMaterials)
                                            mesh->StandardMaterials->Attributes = batchAttri;

                                        for (auto& [type, tex] : batchTextures)
                                        {
                                            if (mesh->StandardMaterials)
                                                mesh->StandardMaterials->Texture[type] = tex;
                                        }
                                    }
                                }

                            }
                        }
                    }
                }

                ImGui::TreePop();
            }

        }
        ImGui::End();

        ImGui::Begin("Properties");
        if (m_SelectedEntity && m_SelectedEntity != EntityFactory::EMPTYENTITY)
        {
            RenderComponents(handle, m_SelectedEntity);
        }
        ImGui::End();
    }

    template<typename T, typename UIFunc>
    static void DrawComponentControls(const std::string& name, const std::shared_ptr<Entity>& entity, UIFunc uiFunc, bool enabled = true)
    {
        static const ImGuiTreeNodeFlags treeNodeFlags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;

        if (entity->HasComponent<T>())
        {
            auto& component = entity->GetComponent<T>();
            if (ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, name.c_str()))
            {
                if (!enabled) ImGui::BeginDisabled();

                uiFunc(component);

                if (!enabled) ImGui::EndDisabled();
                ImGui::TreePop();
            }
        }
    }

    void Scene::RenderComponents(WindowHandle handle, const std::shared_ptr<Entity>& entity)
    {
        if (entity->HasComponent<TagComponent>())
        {
            auto& tag = entity->GetComponent<TagComponent>();

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy_s(buffer, sizeof(buffer), tag.Tag.c_str());

            if (ImGui::InputText("Tag", buffer, sizeof(buffer)))
            {
                tag.Tag = std::string(buffer);
            }
        }

        DrawComponentControls<TransformComponent>("Transform", entity,
            [](TransformComponent& component)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 0.0f });

                CustomUIControl::DrawFloat3("Translation", component.Translation, 0.0f);
                CustomUIControl::DrawFloat3("Rotation", component.Rotation, 0.0f);
                CustomUIControl::DrawFloat3("Scale", component.Scale, 1.0f);

                ImGui::PopStyleVar();
            }
        );

        DrawComponentControls<StaticMeshComponent>("Materials", entity,
            [](StaticMeshComponent& component)
            {
                std::shared_ptr<StaticMesh>& model = component.Model;
                if (model)
                {
                    for (std::shared_ptr<Mesh>& mesh : *model)
                    {
                        std::string nodeHeader = fmt::format("Mesh: {} [{}]", mesh->Name, mesh->Index);
                        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
                        if (ImGui::TreeNodeEx(nodeHeader.c_str(), nodeFlags))
                        {
                            if (model->ModelShadingMethod == ShadingMethod::PhysicalBased)
                            {
                                auto& material = mesh->PhysicalBasedMaterials;
                                if (ImGui::CollapsingHeader("Material Attributes", nodeFlags))
                                {
                                    CustomUIControl::DrawColor3("Base Color", material->Attributes.BaseColor);
                                    CustomUIControl::DrawFloat("Metallic", material->Attributes.Metallic, 0.0f, 1.0f, 0.0005f);
                                    CustomUIControl::DrawFloat("Roughness", material->Attributes.Roughness, 0.0f, 1.0f, 0.0005f);
                                    CustomUIControl::DrawFloat("Opacity", material->Attributes.Opacity, 0.0f, 1.0f, 0.0005f);
                                }
                                if (ImGui::CollapsingHeader("Material Textures", nodeFlags))
                                {
                                    std::int32_t count = 0;
                                    for (auto& [type, tex] : material->Texture)
                                    {
                                        if (count++ > 0)
                                            ImGui::SameLine(0.0f, 14.0f);
                                        ImGui::PushID(static_cast<std::int32_t>(type));
                                        CustomUIControl::TextureSlotCard(GetTextureTypeString(type), tex,
                                            [&]()
                                            {
                                                auto newTex = LoadTexture(type);
                                                if (newTex)
                                                    material->Texture[type] = std::move(newTex);
                                            });

                                        ImGui::PopID();
                                    }
                                }
                                if (ImGui::CollapsingHeader("Base Material Attributes", nodeFlags))
                                {
                                    ImGui::BeginDisabled();
                                    CustomUIControl::DrawColor3("Base Color", material->BaseMaterial->Attributes.BaseColor);
                                    CustomUIControl::DrawFloat("Metallic", material->BaseMaterial->Attributes.Metallic, 0.0f, 1.0f, 0.0005f);
                                    CustomUIControl::DrawFloat("Roughness", material->BaseMaterial->Attributes.Roughness, 0.0f, 1.0f, 0.0005f);
                                    CustomUIControl::DrawFloat("Opacity", material->BaseMaterial->Attributes.Opacity, 0.0f, 1.0f, 0.0005f);
                                    ImGui::EndDisabled();
                                }
                                if (ImGui::CollapsingHeader("Base Material Textures", nodeFlags))
                                {
                                    std::int32_t count = 0;
                                    ImGui::BeginDisabled();
                                    for (const auto& [type, tex] : material->BaseMaterial->Texture)
                                    {
                                        if (count++ > 0)
                                            ImGui::SameLine(0.0f, 14.0f);

                                        ImGui::PushID(static_cast<std::int32_t>(type));
                                        ImGui::BeginDisabled();
                                        CustomUIControl::TextureSlotCard(GetTextureTypeString(type), tex);
                                        ImGui::EndDisabled();
                                        ImGui::PopID();
                                    }
                                    ImGui::EndDisabled();
                                }
                            }
                            else
                            {
                                auto& material = mesh->StandardMaterials;
                                if (ImGui::CollapsingHeader("Material Attributes", nodeFlags))
                                {
                                    CustomUIControl::DrawColor3("Diffuse Color", material->Attributes.DiffuseColor);
                                    CustomUIControl::DrawColor3("Specular Color", material->Attributes.SpecularColor);
                                    CustomUIControl::DrawColor3("Ambient Color", material->Attributes.AmbientColor);
                                    CustomUIControl::DrawColor3("Emissive Color", material->Attributes.EmissiveColor);
                                    CustomUIControl::DrawFloat("Shininess", material->Attributes.Shininess, 1.0f, 32.0f, 0.0005f);
                                    CustomUIControl::DrawFloat("Opacity", material->Attributes.Opacity, 0.0f, 1.0f, 0.0005f);
                                }
                                if (ImGui::CollapsingHeader("Material Textures", nodeFlags))
                                {
                                    std::int32_t count = 0;
                                    for (auto& [type, tex] : material->Texture)
                                    {
                                        if (count++ > 0)
                                            ImGui::SameLine(0.0f, 14.0f);

                                        ImGui::PushID(static_cast<std::int32_t>(type));
                                        CustomUIControl::TextureSlotCard(GetTextureTypeString(type), tex,
                                            [&]()
                                            {
                                                auto newTex = LoadTexture(type);
                                                if (newTex)
                                                    material->Texture[type] = std::move(newTex);
                                            });

                                        ImGui::PopID();
                                    }
                                }
                                if (ImGui::CollapsingHeader("Base Material Attributes", nodeFlags))
                                {
                                    ImGui::BeginDisabled();
                                    CustomUIControl::DrawColor3("Diffuse Color", material->BaseMaterial->Attributes.DiffuseColor);
                                    CustomUIControl::DrawColor3("Specular Color", material->BaseMaterial->Attributes.SpecularColor);
                                    CustomUIControl::DrawColor3("Ambient Color", material->BaseMaterial->Attributes.AmbientColor);
                                    CustomUIControl::DrawColor3("Emissive Color", material->BaseMaterial->Attributes.EmissiveColor);
                                    CustomUIControl::DrawFloat("Shininess", material->BaseMaterial->Attributes.Shininess, 1.0f, 32.0f, 0.0005f);
                                    CustomUIControl::DrawFloat("Opacity", material->BaseMaterial->Attributes.Opacity, 0.0f, 1.0f, 0.0005f);
                                    ImGui::EndDisabled();
                                }
                                if (ImGui::CollapsingHeader("Base Material Textures", nodeFlags))
                                {
                                    std::int32_t count = 0;
                                    ImGui::BeginDisabled();
                                    for (const auto& [type, tex] : material->BaseMaterial->Texture)
                                    {
                                        if (count++ > 0)
                                            ImGui::SameLine(0.0f, 14.0f);

                                        ImGui::PushID(static_cast<std::int32_t>(type));
                                        ImGui::BeginDisabled();
                                        CustomUIControl::TextureSlotCard(GetTextureTypeString(type), tex);
                                        ImGui::EndDisabled();
                                        ImGui::PopID();
                                    }
                                    ImGui::EndDisabled();
                                }
                            }

                            ImGui::TreePop();
                        }
                    }
                }
            }
        );
    }

    void SceneViewport::Update(const FrameBufferSpecification& spec)
    {
        FrameSpec = spec;
        Size = { (float)spec.Width, (float)spec.Height };
    }

    void SceneViewport::Update(const glm::vec2& size)
    {
        Size = size;
        FrameSpec.Width = (uint32_t)size.x;
        FrameSpec.Height = (uint32_t)size.y;
    }

    bool SceneViewport::SizeHasChanged(float width, float height)
    {
        return Size.x != width || Size.y != height || FrameSpec.Width != width || FrameSpec.Height != height;
    }
}