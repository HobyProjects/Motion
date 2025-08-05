#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion
{
    Scene::Scene(SceneHandle handle, const std::string& name, const glm::vec2& viewportSize)
    {
        m_SceneID = handle;
        m_Name = name;
        m_SceneCamera = SceneCamera(viewportSize.x, viewportSize.y, false);
    }

    Scene::~Scene()
    {

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
            ImGuiTreeNodeFlags flags = ((m_SelectedEntity == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
            flags |= ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding;

            bool Opend = ImGui::TreeNodeEx((void*)tag.ID, flags, tag.Tag.c_str());
            if (ImGui::IsItemClicked())
            {
                m_SelectedEntity = entity;
            }

            if (Opend)
            {
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
            bool open = ImGui::TreeNodeEx((void*)component.ID, treeNodeFlags, name.c_str());

            if (open)
            {
                if (!enabled) ImGui::BeginDisabled();
                uiFunc(component);
                ImGui::TreePop();
                if (!enabled) ImGui::EndDisabled();
            }
        }
    }

    //-------------------------------------------------------------------
    // HELPER FUNCTIONS TO DISPLAY MATERIAL DETAILS
    //-------------------------------------------------------------------
    static void DrawInstanceTextureSlot(const std::string_view& texName, std::shared_ptr<ITexture>& texture, std::function<void(std::shared_ptr<ITexture>&)> onLoad)
    {
        ImGui::BeginGroup();

        // Card background (simulate a rounded box)
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
        ImGui::PushID(texName.data());

        // Texture thumbnail or placeholder
        ImVec2 imgSize(56, 56);
        if (texture)
        {
            ImGui::Image(texture->GetID(), imgSize);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Texture: %s", texName.data());
        }
        else
        {
            ImGui::Dummy(imgSize);
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Texture: %s (Not Loaded)", texName.data());
        }

        // Load button (below the name)
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (imgSize.x - 46.0f) * 0.5f); // center button
        if (ImGui::Button("Load", ImVec2(46, 0)))
        {
            if (onLoad)
                onLoad(texture);
        }
        ImGui::PopID();
        ImGui::PopStyleVar();

        ImGui::EndGroup();
    }

    static void DrawInstanceTexturesRow(std::unordered_map<std::string_view, std::shared_ptr<ITexture>>& textures, std::function<void(const std::string_view&, std::shared_ptr<ITexture>&)> onLoad)
    {
        // Show each texture as a card in a horizontal row
        int count = 0;
        for (auto& [name, tex] : textures)
        {
            if (count++ > 0)
                ImGui::SameLine(0.0f, 18.0f); // space between cards

            DrawInstanceTextureSlot(name, tex,
                [&](std::shared_ptr<ITexture>& t) { onLoad(name, t); });
        }
    }

    static void ShowBaseTexturesRow(const std::unordered_map<std::string_view, std::shared_ptr<ITexture>>& textures)
    {
        if (textures.empty())
            return;

        ImGui::BeginDisabled();
        int count = 0;
        for (const auto& [name, tex] : textures)
        {
            if (count++ > 0)
                ImGui::SameLine(0.0f, 18.0f);

            ImGui::BeginGroup();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

            ImVec2 imgSize(56, 56);
            if (tex)
            {
                ImGui::Image(tex->GetID(), imgSize);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Texture: %s", name.data());
            }
            else
            {
                ImGui::Dummy(imgSize);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Texture: %s (Not Loaded)", name.data());
            }

            ImGui::PopStyleVar();
            ImGui::EndGroup();
        }
        ImGui::EndDisabled();
    }

    static bool DrawMaterialAttributes(Motion::MaterialAttributes& attr)
    {
        bool changed = false;
        changed |= CustomUIControl::DrawColor3("MI-Base Color", attr.BaseColor);
        changed |= CustomUIControl::DrawFloat("MI-Metallic", attr.Metallic, 0.0f, 1.0f, 0.005f);
        changed |= CustomUIControl::DrawFloat("MI-Roughness", attr.Roughness, 0.0f, 1.0f, 0.005f);
        changed |= CustomUIControl::DrawFloat("MI-AO", attr.AmbientOcclusion, 0.0f, 1.0f, 0.005f);
        changed |= CustomUIControl::DrawFloat("MI-Opacity", attr.Opacity, 0.0f, 1.0f, 0.005f);
        changed |= CustomUIControl::DrawFloat("MI-Displacement", attr.DisplacementScale, 0.0f, 1.0f, 0.005f);
        return changed;
    }

    static void ShowMaterialAttributes(const Motion::MaterialAttributes& attr)
    {
        ImGui::BeginDisabled();
        glm::vec3 color = attr.BaseColor;
        CustomUIControl::DrawColor3("BM-Base Color", color); // const version just passes by value
        float metallic = attr.Metallic;
        CustomUIControl::DrawFloat("BM-Metallic", metallic, 0.0f, 1.0f);
        float roughness = attr.Roughness;
        CustomUIControl::DrawFloat("BM-Roughness", roughness, 0.0f, 1.0f);
        float ao = attr.AmbientOcclusion;
        CustomUIControl::DrawFloat("BM-AO", ao, 0.0f, 1.0f);
        float opacity = attr.Opacity;
        CustomUIControl::DrawFloat("BM-Opacity", opacity, 0.0f, 1.0f);
        float disp = attr.DisplacementScale;
        CustomUIControl::DrawFloat("BM-Displacement", disp, 0.0f, 1.0f);
        ImGui::EndDisabled();
    }

    static void DrawMaterialInstancePanel(std::shared_ptr<Motion::MaterialInstance>& matInstance, std::function<void(const std::string_view&, std::shared_ptr<ITexture>&)> onLoadTexture)
    {
        if (!matInstance) return;

        // ---- Editable: Instance attributes ----
        if (ImGui::CollapsingHeader("Material Instance Attributes", ImGuiTreeNodeFlags_DefaultOpen))
            DrawMaterialAttributes(matInstance->Attributes);

        // ---- Editable: Instance Textures ----
        if (!matInstance->Texture.empty() && ImGui::CollapsingHeader("Material Instance Textures", ImGuiTreeNodeFlags_DefaultOpen))
        {
            DrawInstanceTexturesRow(matInstance->Texture, onLoadTexture);
        }

        // ---- Read-only: Base Material ----
        if (matInstance->BaseMaterial)
        {
            if (ImGui::CollapsingHeader("Base Material Attributes", ImGuiTreeNodeFlags_DefaultOpen))
                ShowMaterialAttributes(matInstance->BaseMaterial->Attributes);

            if (!matInstance->BaseMaterial->Texture.empty() && ImGui::CollapsingHeader("Base Material Textures", ImGuiTreeNodeFlags_DefaultOpen))
                ShowBaseTexturesRow(matInstance->BaseMaterial->Texture);
        }
    }

    //-------------------------------------------------------------------


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
            [](auto& component)
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 10.0f, 0.0f });

                CustomUIControl::DrawFloat3("Translation", component.Translation, 0.0f);
                CustomUIControl::DrawFloat3("Rotation", component.Rotation, 0.0f);
                CustomUIControl::DrawFloat3("Scale", component.Scale, 1.0f);

                ImGui::PopStyleVar();
            }
        );

        DrawComponentControls<StaticMeshComponent>("Materials", entity,
            [](auto& component)
            {
                auto& model = component.Model;
                if (model)
                {
                    std::uint32_t meshIndex{ 0 };
                    for (auto meshSegment = model->begin(); meshSegment != model->end(); ++meshSegment, ++meshIndex)
                    {
                        if (meshSegment->Materials)
                        {
                            ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
                            std::string nodeHeader = fmt::format("Mesh [{}] - Material: {}", meshIndex, meshSegment->Materials->GetName());
                            if (ImGui::TreeNodeEx(nodeHeader.c_str(), nodeFlags))
                            {
                                DrawMaterialInstancePanel(meshSegment->Materials,
                                    [&](const std::string_view& name, std::shared_ptr<ITexture>& slotTex)
                                    {
                                        std::filesystem::path file = DialogBoxes::OpenFileDialog();
                                        if (!file.empty())
                                        {
                                            TextureType type;
                                            if (name == UniformCache::BaseColorTextures)
                                                type = TextureType::BaseColorTexture;
                                            else if (name == UniformCache::MetallicTextures)
                                                type = TextureType::MetallicTexture;
                                            else if (name == UniformCache::RoughnessTextures)
                                                type = TextureType::RoughnessTexture;
                                            else if (name == UniformCache::AmbientOcclusionTextures)
                                                type = TextureType::AmbientOcclusionTexture;
                                            else if (name == UniformCache::DisplacementTextures)
                                                type = TextureType::DisplacementTexture;
                                            else if (name == UniformCache::NormalTextures)
                                                type = TextureType::NormalTexture;
                                            else
                                                type = TextureType::UnknownTexture;


                                            slotTex = ITexture::Create(file, type, true);
                                            if (!slotTex)
                                            {
                                                MOTION_ERROR("Failed to load texture from file: {0}", file.string());
                                            }
                                        }
                                    });

                                ImGui::TreePop();
                            }
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