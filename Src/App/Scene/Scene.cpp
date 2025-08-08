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



    Scene::Scene(const SceneSpecification& spec)
    {
        m_Specification = spec;
        m_Camera = SceneCamera(spec.Viewport.Size.x, spec.Viewport.Size.y, false);
    }

    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime) noexcept
    {
        m_Camera.OnUpdate(handle, deltaTime);
    }

    void Scene::OnEvent(WindowHandle handle, IEvent& e) noexcept
    {
        m_Camera.OnEvents(handle, e);
    }

    void Scene::OnViewportSizeChanges(const glm::vec2& size) noexcept
    {
        m_Camera.SetAspectRatio(size.x, size.y);
    }

    std::shared_ptr<Entity> Scene::PickEntity(const glm::vec2& mousePos, const glm::vec2& viewportSize)
    {
        const glm::mat4& projection = m_Camera.Camera.Projection;
        const glm::mat4& view = m_Camera.Camera.View;

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

    static void DrawViewportAxisWidget(const glm::mat4& view, int corner = 2, float baseSize = 64.0f, ImVec2 basePadding = ImVec2(12, 12))
    {
        ImDrawList* dl = ImGui::GetForegroundDrawList(); // above content
        ImGuiIO& io = ImGui::GetIO();

        // Scale for HiDPI
        const float scale = io.FontGlobalScale > 0.0f ? io.FontGlobalScale : 1.0f;
        const float size = baseSize * scale;
        const ImVec2 pad = ImVec2(basePadding.x * scale, basePadding.y * scale);
        const float thick = 3.0f * scale;
        const float ahLen = 8.0f * scale;   // arrowhead length
        const float ahHalf = 4.0f * scale;   // arrowhead half-width
        const float radius = 7.0f * scale;   // origin dot
        const float cardR = 8.0f * scale;   // card rounding
        const float cardPad = 6.0f * scale;

        // Anchor inside current window's content rect
        const ImVec2 winPos = ImGui::GetWindowPos();
        const ImVec2 crMin = ImGui::GetWindowContentRegionMin();
        const ImVec2 crMax = ImGui::GetWindowContentRegionMax();
        ImRect content(ImVec2(winPos.x + crMin.x, winPos.y + crMin.y), ImVec2(winPos.x + crMax.x, winPos.y + crMax.y));

        // Card rectangle
        ImVec2 cardSize(size + cardPad * 2, size + cardPad * 2);
        ImVec2 cardMin, cardMax;
        switch (corner)
        {
        case 0:     cardMin = ImVec2(content.Min.x + pad.x, content.Min.y + pad.y); break; // Top-left
        case 1:     cardMin = ImVec2(content.Max.x - pad.x - cardSize.x, content.Min.y + pad.y); break; // Top-right
        case 2:     cardMin = ImVec2(content.Min.x + pad.x, content.Max.y - pad.y - cardSize.y); break; // Bottom-left
        default:    cardMin = ImVec2(content.Max.x - pad.x - cardSize.x, content.Max.y - pad.y - cardSize.y); break; // Bottom-right
        }
        cardMax = ImVec2(cardMin.x + cardSize.x, cardMin.y + cardSize.y);

        // Background "card" with subtle shadow
        dl->AddRectFilled(ImVec2(cardMin.x, cardMin.y + 2 * scale), ImVec2(cardMax.x, cardMax.y + 2 * scale), IM_COL32(0, 0, 0, 40), cardR);
        dl->AddRectFilled(cardMin, cardMax, IM_COL32(28, 28, 32, 180), cardR);
        dl->AddRect(cardMin, cardMax, IM_COL32(255, 255, 255, 20), cardR);

        // Axis origin
        ImVec2 origin = ImVec2(cardMin.x + cardPad + size * 0.5f, cardMin.y + cardPad + size * 0.5f);

        // Extract camera rotation (upper-left 3x3 of inverse(view))
        // view = R^T * T^-1 for right-handed OpenGL-like conventions.
        glm::mat3 R = glm::mat3(glm::transpose(view)); // matches your original approach (camera basis rows)

        struct Axis { glm::vec3 dir; ImU32 col; const char* lbl; };
        Axis axes[] = {
            { {1,0,0}, IM_COL32(220, 70, 70, 255), "X" },
            { {0,1,0}, IM_COL32(70,220, 70, 255), "Y" },
            { {0,0,1}, IM_COL32(90,150,255,255), "Z" },
        };

        auto draw_axis = [&](const Axis& a)
            {
                // Local axis in camera space (so "toward screen" fades)
                glm::vec3 v = glm::normalize(R * a.dir);

                // Map to 2D inside the square: X to +x, Y to -y to match screen down
                ImVec2 tip = ImVec2(origin.x + v.x * (size * 0.45f),
                    origin.y - v.y * (size * 0.45f));

                // Fade based on Z (positive Z away from camera in view space)—tweak to taste
                float z = v.z; // if axis points out of screen (z<0), brighten, else dim
                float alpha = (z < 0.0f) ? 1.00f : 0.40f;
                ImU32 lineCol = IM_COL32(
                    (int)((a.col >> 0) & 0xFF),
                    (int)((a.col >> 8) & 0xFF),
                    (int)((a.col >> 16) & 0xFF),
                    (int)(255 * alpha)
                );

                // Line
                dl->AddLine(origin, tip, lineCol, thick);

                // Arrowhead (simple isosceles)
                glm::vec2 d = glm::normalize(glm::vec2(tip.x - origin.x, tip.y - origin.y));
                glm::vec2 n = glm::vec2(-d.y, d.x);
                ImVec2 a0 = ImVec2(tip.x - d.x * ahLen + n.x * ahHalf, tip.y - d.y * ahLen + n.y * ahHalf);
                ImVec2 a1 = ImVec2(tip.x - d.x * ahLen - n.x * ahHalf, tip.y - d.y * ahLen - n.y * ahHalf);
                dl->AddTriangleFilled(tip, a0, a1, lineCol);

                // Label near the tip
                ImVec2 labelPos = ImVec2(tip.x + 6.0f * scale, tip.y - 6.0f * scale);
                dl->AddText(labelPos, lineCol, a.lbl);
            };

        for (const Axis& a : axes)
            draw_axis(a);

        // Origin dot
        dl->AddCircleFilled(origin, radius, IM_COL32(180, 180, 190, 220));
        dl->AddCircle(origin, radius, IM_COL32(255, 255, 255, 40), 0, 1.5f * scale);
    }

    void SceneViewportPanel::RenderUI(ScenePanelContext& context)
    {
        // ------------------------------------------------------------
        // DRAWING THE SCENE VIEWPORT
        // ------------------------------------------------------------
        m_Title = context.ActiveScene->GetName();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(m_Title.c_str());
        context.UILayerInstance->AcceptEvents(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());
        ImVec2 sceneViewport = ImGui::GetContentRegionAvail();
        if (sceneViewport.x != context.ActiveSceneSpecification.Viewport.Size.x || sceneViewport.y != context.ActiveSceneSpecification.Viewport.Size.y)
        {
            context.ActiveSceneSpecification.Viewport.Size.x = sceneViewport.x;
            context.ActiveSceneSpecification.Viewport.Size.y = sceneViewport.y;
        }

        ImGui::Image((ImTextureID)context.ActiveViewportTexture, sceneViewport, { 0, 1 }, { 1, 0 });

        //----------------------------------------------
        // HANDLING MOUSE PICKING
        //----------------------------------------------
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 contentMax = ImGui::GetWindowContentRegionMax();

        ImVec2 viewportMin = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
        ImVec2 viewportMax = ImVec2(windowPos.x + contentMax.x, windowPos.y + contentMax.y);
        ImVec2 mousePos = ImGui::GetMousePos();

        bool isWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bool isWindowFocused = ImGui::IsWindowFocused();

        bool isClick = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        bool isUsingGizmo = ImGuizmo::IsUsing();

        if (isWindowHovered && isWindowFocused && isClick && !isUsingGizmo)
        {
            glm::vec2 mouseViewport = { mousePos.x - viewportMin.x, mousePos.y - viewportMin.y };
            mouseViewport.y = sceneViewport.y - mouseViewport.y;
            auto picked = context.ActiveScene->PickEntity(mouseViewport, glm::vec2(sceneViewport.x, sceneViewport.y));
            if (picked)
                context.ActiveScene->SelectedEntity(picked);
        }

        // ------------------------------------------------------------
        // HANDLING IMGUIZMO MANIPULATION
        // ------------------------------------------------------------
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImVec2 viewportPos = ImGui::GetWindowPos();
        ImGuizmo::SetRect(viewportPos.x, viewportPos.y, sceneViewport.x, sceneViewport.y);

        glm::mat4 view = context.ActiveScene->GetCameraView();
        glm::mat4 proj = context.ActiveScene->GetCameraProjection();

        // Handle CTRL+E to cycle operation
        // [TODO]: Need to check operation and control when scene count increases
        static ImGuizmo::OPERATION s_CurrentOperation = ImGuizmo::TRANSLATE;
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_E))
        {
            if (s_CurrentOperation == ImGuizmo::TRANSLATE)
                s_CurrentOperation = ImGuizmo::ROTATE;
            else if (s_CurrentOperation == ImGuizmo::ROTATE)
                s_CurrentOperation = ImGuizmo::SCALE;
            else
                s_CurrentOperation = ImGuizmo::TRANSLATE;
        }

        auto selected = context.ActiveScene->GetSelectedEntity();
        if (selected && selected != EntityFactory::EMPTYENTITY && selected->HasComponent<TransformComponent>())
        {
            auto& tc = selected->GetComponent<TransformComponent>();
            glm::mat4 model = tc.GetTransform();

            float matrix[16];
            memcpy(matrix, glm::value_ptr(model), sizeof(float) * 16);

            if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), s_CurrentOperation, ImGuizmo::LOCAL, matrix))
            {
                glm::vec3 translation, scale;
                glm::quat rotation;

                glm::vec3 eulerRotation = glm::degrees(glm::eulerAngles(rotation)); // Convert to degrees for user editing
                ImGuizmo::DecomposeMatrixToComponents(matrix, &translation.x, &eulerRotation.x, &scale.x);
                rotation = glm::quat(glm::radians(eulerRotation)); // Convert back to quaternion

                tc.Translation = translation;
                tc.Rotation = rotation;
                tc.Scale = scale;
            }
        }

        //-----------------------------------------------
        // DRAWING THE VIEW MANIPULATION GIZMO
        //-----------------------------------------------
        glm::vec3 selectedPosition;
        if (selected && selected->HasComponent<TransformComponent>())
            selectedPosition = selected->GetComponent<TransformComponent>().Translation;
        else
            selectedPosition = glm::vec3(0.0f);

        auto& camera = context.ActiveScene->GetCamera().Camera;
        glm::vec3 target = selectedPosition;
        float cameraDistance = glm::length(camera.Position - target);
        glm::mat4 oldView = camera.View;

        ImVec2 viewGizmoSize(200, 200);
        ImVec2 gizmoPos = ImVec2(viewportMax.x - viewGizmoSize.x, viewportMin.y);
        ImGuizmo::ViewManipulate(glm::value_ptr(view), 16.0f, gizmoPos, viewGizmoSize, IM_COL32(0x22, 0x22, 0x22, 0x88));

        bool viewChanged = false;
        for (int i = 0; i < 16; ++i)
            if (fabs(glm::value_ptr(view)[i] - glm::value_ptr(oldView)[i]) > 1e-5f)
                viewChanged = true;

        if (viewChanged)
        {
            glm::vec3 newForward = -glm::vec3(view[2]); // Negative Z (OpenGL)
            glm::vec3 newPos = target - newForward * cameraDistance;
            camera.Position = newPos;
            camera.LookAt(target);
        }

        // Draw the viewport axis widget
        DrawViewportAxisWidget(view);

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void SceneEntityInspectPanel::RenderUI(ScenePanelContext& context)
    {
        m_Title = std::format("{} Entities", context.ActiveScene->GetName());
        ImGui::Begin(m_Title.c_str());

        //------------------------------------------------------------
        // POPUP MENU FOR IMPORT MODEL QUICKLY
        //------------------------------------------------------------
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
                        context.ActiveScene->EmplaceEntity(entity);
                    }
                    else
                    {
                        MOTION_ERROR("Failed to load static Mesh from file: {0}", filePath.string());
                    }
                }
            }
            ImGui::EndPopup();
        }

        //------------------------------------------------------------
        // CLEAR SELECTION ON CLICKING EMPTY SPACE
        //------------------------------------------------------------
        if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsAnyItemHovered())
        {
            context.ActiveScene->SelectedEntity(EntityFactory::EMPTYENTITY);
        }

        //------------------------------------------------------------
        // RENDERING ENTITIES IN THE OUTLINER AND IT'S DETAILS
        //------------------------------------------------------------
        for (auto& entity : *context.ActiveScene)
        {
            auto& tag = entity->GetComponent<TagComponent>();

            ImGuiTreeNodeFlags flags =
                ((context.ActiveScene->GetSelectedEntity() == entity) ? ImGuiTreeNodeFlags_Selected : 0)
                | ImGuiTreeNodeFlags_OpenOnArrow
                | ImGuiTreeNodeFlags_OpenOnDoubleClick
                | ImGuiTreeNodeFlags_SpanFullWidth
                | ImGuiTreeNodeFlags_Framed
                | ImGuiTreeNodeFlags_FramePadding;

            ImGui::PushID((void*)entity.get());
            bool nodeOpen = ImGui::TreeNodeEx("##node", flags, "%s", tag.Tag.c_str());

            if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                context.ActiveScene->SelectedEntity(entity);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                context.ActiveScene->SelectedEntity(entity);

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
                                context.ActiveScene->SelectedEntity(entity);

                            static PhysicalBasedMaterialAttribute batchAttri{};
                            static std::unordered_map<TextureType, std::shared_ptr<ITexture>> batchTextures{};

                            bool attributesChanged = false;
                            attributesChanged |= CustomUIControl::ColorEdit3("Base Color", batchAttri.BaseColor);
                            attributesChanged |= CustomUIControl::DrawFloat("Metallic", batchAttri.Metallic, 0.0f, 1.0f, 0.005f);
                            attributesChanged |= CustomUIControl::DrawFloat("Roughness", batchAttri.Roughness, 0.0f, 1.0f, 0.005f);
                            attributesChanged |= CustomUIControl::DrawFloat("Opacity", batchAttri.Opacity, 0.0f, 1.0f, 0.005f);


                            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

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

                            ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal);

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
    }

    void SceneEntityPropertiesPanel::RenderUI(ScenePanelContext& context)
    {
        std::shared_ptr<Entity> selectedEntity = context.ActiveScene->GetSelectedEntity();
        if (selectedEntity && selectedEntity != EntityFactory::EMPTYENTITY)
        {
            std::string entityName = selectedEntity->GetComponent<TagComponent>().Tag;
            std::string title = std::format("{} Properties", entityName);
            ImGui::Begin(title.c_str());

            if (selectedEntity->HasComponent<TagComponent>())
            {
                auto& tag = selectedEntity->GetComponent<TagComponent>();
                CustomUIControl::TextBox("Tag", tag.Tag, false, 256, 150.0f);
            }

            DrawComponentControls<TransformComponent>("Transform", selectedEntity,
                [](TransformComponent& component)
                {
                    CustomUIControl::DrawFloat3("Translation", component.Translation, 0.0f);
                    CustomUIControl::DrawQuatEuler("Rotation", component.Rotation, 0.0f);
                    CustomUIControl::DrawFloat3("Scale", component.Scale, 10.0f);
                }
            );

            DrawComponentControls<StaticMeshComponent>("Materials", selectedEntity,
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

            ImGui::End();
        }
    }

    void SceneSettingsPanel::RenderUI(ScenePanelContext& context)
    {
        // ------------------------------------------------------------
        // DRAWING SCENE ENVIRONMENT SETTINGS
        // ------------------------------------------------------------
        ImGui::Begin(std::format("{} Environment Settings", context.ActiveScene->GetName()).c_str());
        auto& env = context.ActiveScene->GetEnvironment();
        static const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen
            | ImGuiTreeNodeFlags_Framed
            | ImGuiTreeNodeFlags_SpanAvailWidth
            | ImGuiTreeNodeFlags_AllowItemOverlap
            | ImGuiTreeNodeFlags_FramePadding;

        if (ImGui::TreeNodeEx((void*)env.DirectionalLight.LightID, treeNodeFlags, "Environment Lighting"))
        {
            for (int i = 0; i < DirectionalLight::LIGHT_COUNT; ++i)
            {
                if (ImGui::CollapsingHeader(std::format("Light {}", i).c_str(), ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed))
                {
                    CustomUIControl::DrawFloat3("Position", env.DirectionalLight.LightPosition[i], 0.0f);
                    CustomUIControl::ColorEdit3("Color", env.DirectionalLight.LightColor[i]);
                    CustomUIControl::DrawFloat("Intensity", env.DirectionalLight.LightIntensity[i], 0.0f, 1.0f, 0.005f);
                }
            }

            ImGui::TreePop();
        }
        ImGui::End();
    }
}