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

        // Context menu on empty window space
        if (ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
        {
            if (ImGui::MenuItem("Import StaticMesh"))
            {
                // [TODO]: This should happen on a different thread
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

        // Only clear selection when clicking truly empty space in this window
        if (ImGui::IsWindowHovered()
            && ImGui::IsMouseDown(ImGuiMouseButton_Left)
            && !ImGui::IsAnyItemHovered())
        {
            m_SelectedEntity = EntityFactory::EMPTYENTITY;
        }

        for (uint32_t i = 0; i < m_Entities.size(); i++)
        {
            std::shared_ptr<Entity> entity = m_Entities[i];
            auto& tag = entity->GetComponent<TagComponent>();

            ImGuiTreeNodeFlags flags =
                ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0)
                | ImGuiTreeNodeFlags_OpenOnArrow
                | ImGuiTreeNodeFlags_OpenOnDoubleClick
                | ImGuiTreeNodeFlags_SpanFullWidth
                | ImGuiTreeNodeFlags_Framed
                | ImGuiTreeNodeFlags_FramePadding;

            ImGui::PushID((void*)entity.get());
            bool nodeOpen = ImGui::TreeNodeEx("##node", flags, "%s", tag.Tag.c_str());

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                m_SelectedEntity = entity;

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                m_SelectedEntity = entity;

            if (nodeOpen)
            {
                if (entity->HasComponent<StaticMeshComponent>())
                {
                    auto& model = entity->GetComponent<StaticMeshComponent>().Model;
                    if (model)
                    {
                        ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth
                            | ImGuiTreeNodeFlags_Framed
                            | ImGuiTreeNodeFlags_FramePadding;

                        if (ImGui::CollapsingHeader("Mesh Details", nodeFlags))
                        {
                            std::string meshCount = std::to_string(model->GetMeshesCount());
                            std::string minBounds = glm::to_string(model->GetMinBounds());
                            std::string maxBounds = glm::to_string(model->GetMaxBounds());
                            std::string filePath = model->GetSource();

                            CustomUIControl::TextBox("Mesh Count", meshCount, true);
                            CustomUIControl::TextBox("Min Bounds", minBounds, true);
                            CustomUIControl::TextBox("Max Bounds", maxBounds, true);
                            CustomUIControl::TextBox("File Path", filePath, true);
                        }

                        if (ImGui::CollapsingHeader("Material Batch Assignment", nodeFlags))
                        {
                            // Clicking inside this header should also select the entity (nice UX)
                            if (ImGui::IsItemActivated())
                                m_SelectedEntity = entity;

                            static PhysicalBasedMaterialAttribute batchAttri{};
                            static std::unordered_map<TextureType, std::shared_ptr<ITexture>> batchTextures{};

                            bool attributesChanged = false;
                            attributesChanged |= CustomUIControl::ColorEdit3("Base Color", batchAttri.BaseColor);
                            attributesChanged |= CustomUIControl::DrawFloat("Metallic", batchAttri.Metallic, 0.0f, 1.0f, 0.005f);
                            attributesChanged |= CustomUIControl::DrawFloat("Roughness", batchAttri.Roughness, 0.0f, 1.0f, 0.005f);
                            attributesChanged |= CustomUIControl::DrawFloat("Opacity", batchAttri.Opacity, 0.0f, 1.0f, 0.005f);

                            for (auto texType : { TextureType::BaseColorTexture, TextureType::MetallicTexture, TextureType::RoughnessTexture, TextureType::AmbientOcclusionTexture, TextureType::DisplacementTexture, TextureType::NormalTexture })
                            {
                                CustomUIControl::TextureSlotCard(GetTextureTypeString(texType).c_str(), batchTextures[texType],
                                    [&]()
                                    {
                                        auto newTex = LoadTexture(texType);
                                        if (newTex) batchTextures[texType] = newTex;
                                    }
                                );
                            }

                            if (ImGui::Button("Apply"))
                            {
                                for (auto& mesh : *model)
                                {
                                    if (mesh->PhysicalBasedMaterials)
                                    {
                                        mesh->PhysicalBasedMaterials->Attributes = batchAttri;
                                        for (auto& [type, tex] : batchTextures)
                                            mesh->PhysicalBasedMaterials->Texture[type] = tex;
                                    }
                                }
                            }
                        }
                    }
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        ImGui::End();

        // Properties panel
        ImGui::Begin("Properties");
        if (m_SelectedEntity && m_SelectedEntity != EntityFactory::EMPTYENTITY)
        {
            RenderComponents(handle, m_SelectedEntity);
        }
        ImGui::End();
    }


    void Scene::RenderComponents(WindowHandle handle, const std::shared_ptr<Entity>& entity)
    {
        if (entity->HasComponent<TagComponent>())
        {
            auto& tag = entity->GetComponent<TagComponent>();
            CustomUIControl::TextBox("Tag", tag.Tag, false, 256, 150.0f);
        }

        DrawComponentControls<TransformComponent>("Transform", entity,
            [](TransformComponent& component)
            {
                CustomUIControl::DrawFloat3("Translation", component.Translation, 0.0f);
                CustomUIControl::DrawQuatEuler("Rotation", component.Rotation, 0.0f);
                CustomUIControl::DrawFloat3("Scale", component.Scale, 10.0f);
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
                                    CustomUIControl::ColorEdit3("Base Color", material->Attributes.BaseColor);
                                    CustomUIControl::DrawFloat("Metallic", material->Attributes.Metallic, 0.0f, 1.0f, 0.005f);
                                    CustomUIControl::DrawFloat("Roughness", material->Attributes.Roughness, 0.0f, 1.0f, 0.005f);
                                    CustomUIControl::DrawFloat("Opacity", material->Attributes.Opacity, 0.0f, 1.0f, 0.005f);
                                }
                                if (ImGui::CollapsingHeader("Material Textures", nodeFlags))
                                {
                                    for (auto& [type, tex] : material->Texture)
                                    {
                                        CustomUIControl::TextureSlotCard(GetTextureTypeString(type).c_str(), tex,
                                            [&]()
                                            {
                                                auto newTex = LoadTexture(type);
                                                if (newTex)
                                                    material->Texture[type] = std::move(newTex);
                                            });
                                    }
                                }
                                if (ImGui::CollapsingHeader("Base Material Attributes", nodeFlags))
                                {
                                    ImGui::BeginDisabled();
                                    CustomUIControl::ColorEdit3("Base Color", material->BaseMaterial->Attributes.BaseColor);
                                    CustomUIControl::DrawFloat("Metallic", material->BaseMaterial->Attributes.Metallic, 0.0f, 1.0f, 0.005f);
                                    CustomUIControl::DrawFloat("Roughness", material->BaseMaterial->Attributes.Roughness, 0.0f, 1.0f, 0.005f);
                                    CustomUIControl::DrawFloat("Opacity", material->BaseMaterial->Attributes.Opacity, 0.0f, 1.0f, 0.005f);
                                    ImGui::EndDisabled();
                                }
                                if (ImGui::CollapsingHeader("Base Material Textures", nodeFlags))
                                {
                                    ImGui::BeginDisabled();
                                    for (auto& [type, tex] : material->BaseMaterial->Texture)
                                    {
                                        CustomUIControl::TextureSlotCard(GetTextureTypeString(type).c_str(), tex);
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