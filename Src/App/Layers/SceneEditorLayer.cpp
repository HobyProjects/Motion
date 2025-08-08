#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    static std::shared_ptr<IWindow> s_Window{ nullptr };
    static std::shared_ptr<ImGuiLayer> s_ImGuiLayer{ nullptr };
    static ImGuizmo::OPERATION s_CurrentOperation = ImGuizmo::TRANSLATE;

    SceneEditorLayer::SceneEditorLayer(WindowHandle handle, const std::shared_ptr<ImGuiLayer>& imguiLayer) : Layer("EditorLayer")
    {
        s_ImGuiLayer = imguiLayer;
    }

    void SceneEditorLayer::OnAttach()
    {
        //TEMP
        auto& assetManager = AssetManager::GetInstance();
        assetManager.Create<IShader>("ENV", "Assets/Shaders/Environment.glsl");
        assetManager.Create<IShader>("ENV_IRR", "Assets/Shaders/EnvironmentIrradiance.glsl");
        assetManager.Create<IShader>("ENV_PRE", "Assets/Shaders/EnvironmentPrefiltered.glsl");
        assetManager.Create<IShader>("ENV_CUB", "Assets/Shaders/EnvironmentCubeConverter.glsl");
        assetManager.Create<IShader>("ENV_BRD", "Assets/Shaders/EnvironmentBRDF.glsl");
        assetManager.Create<IShader>("PBR", "Assets/Shaders/PBR.glsl");
        assetManager.Create<IShader>("PHONG", "Assets/Shaders/Phong.glsl");

        PhysicalBasedMaterial::Import("Assets/Materials/Base/PBR/Base.yaml");
        StandardMaterial::Import("Assets/Materials/Base/STD/Base.yaml");

        m_Environment = IEnvironment::Create("Assets/HDRI/Scene4.hdr");



        m_Viewport.FrameSpec.Name = "SceneEditorFrame";
        m_Viewport.FrameSpec.Width = static_cast<uint32_t>(m_ViewportWidth);
        m_Viewport.FrameSpec.Height = static_cast<uint32_t>(m_ViewportHeight);
        m_Viewport.Size = { m_ViewportWidth, m_ViewportHeight };
        m_Framebuffer = IFrameBuffer::Create(m_Viewport.FrameSpec);

        if (m_Scenes.empty())
        {
            //[TODO] : When scene serialization is implemented, load the default scene from a file or create a new one.
            m_Scenes.push_back(std::make_shared<Scene>(UniqueIdentity::GetUniqueID(), "Default Scene", glm::vec2(m_ViewportWidth, m_ViewportHeight)));
        }

        m_ActiveScene = m_Scenes[0];
        m_ActiveScene->SetActive(true);
    }

    void SceneEditorLayer::OnDetach()
    {
        m_Framebuffer.reset();
        m_Scenes.clear();
    }

    void SceneEditorLayer::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        //--------------------------------------------------------------
        // UPDATING THE VIEWPORT
        //--------------------------------------------------------------
        if (m_Viewport.SizeHasChanged(m_ViewportWidth, m_ViewportHeight))
        {
            m_Viewport.Update(glm::vec2(m_ViewportWidth, m_ViewportHeight));
            m_Framebuffer->ResizeFrame((uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight);
            m_ActiveScene->OnViewportSizeChanges(m_ViewportWidth, m_ViewportHeight);
        }

        //--------------------------------------------------------------
        // UPDATING THE ACTIVE SCENE
        //--------------------------------------------------------------
        m_ActiveScene->OnUpdate(handle, deltaTime);


        //--------------------------------------------------------------
        // RENDERING THE SCENE
        //--------------------------------------------------------------
        m_Framebuffer->Bind();
        Renderer::ClearColor({ 0.243, 0.243, 0.243, 1.0f });
        Renderer::Clear();
        m_Environment->Render(m_ActiveScene->GetViewMatrix(), m_ActiveScene->GetProjectionMatrix());
        SceneRenderer::BeginScene();
        SceneRenderer::Submit(m_ActiveScene, m_Environment);
        SceneRenderer::EndScene();
        m_Framebuffer->Unbind();
        m_SceneTextures[m_ActiveScene] = m_Framebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
    }

    void SceneEditorLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        m_ActiveScene->OnEvent(handle, e);
    }

    static void DrawViewportAxisWidget(const glm::mat4& view, float size = 60.0f)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        // Find bottom-left of viewport
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportMin = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
        ImVec2 axisOrigin = ImVec2(viewportMin.x + size + 10.0f, viewportMin.y + ImGui::GetWindowSize().y - size - 10.0f);

        // Camera basis (extract from view matrix)
        glm::mat3 camBasis = glm::mat3(glm::transpose(view)); // Row-major, use transpose for camera orientation

        struct Axis {
            glm::vec3 dir;
            ImU32 color;
            const char* label;
        };

        Axis axes[3] = {
            { glm::vec3(1,0,0), IM_COL32(200,60,60,255), "X" },
            { glm::vec3(0,1,0), IM_COL32(60,200,60,255), "Y" },
            { glm::vec3(0,0,1), IM_COL32(80,150,255,255), "Z" }
        };

        for (int i = 0; i < 3; ++i)
        {
            glm::vec3 localDir = camBasis * axes[i].dir; // Camera space to world
            localDir = glm::normalize(localDir);

            float len = size;
            ImVec2 p0 = axisOrigin;
            ImVec2 p1 = ImVec2((axisOrigin.x + localDir.x * len), (axisOrigin.y - localDir.y * len)); // ImGui Y is downward

            drawList->AddLine(p0, p1, axes[i].color, 3.0f);

            // Draw label at the end
            ImVec2 labelPos = ImVec2(p1.x + 5.0f, p1.y - 5.0f); // Offset for better visibility
            drawList->AddText(labelPos, axes[i].color, axes[i].label);
        }

        // Optional: Draw circle at axis origin
        drawList->AddCircleFilled(axisOrigin, 5.0f, IM_COL32(120, 120, 120, 255));
    }

    void SceneEditorLayer::OnUIRender(WindowHandle handle)
    {
        // ------------------------------------------------------------
        // DRAWING THE DOCKSPACE AND SCENE UIs
        // ------------------------------------------------------------
        DrawDockspace();
        m_ActiveScene->OnUIRenders(handle);


        // ------------------------------------------------------------
        // DRAWING THE SCENE VIEWPORT
        // ------------------------------------------------------------
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(m_ActiveScene->GetSceneName().c_str());
        s_ImGuiLayer->AcceptEvents(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());
        ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        if (viewportPanelSize.x != m_ViewportWidth || viewportPanelSize.y != m_ViewportHeight)
        {
            m_ViewportWidth = viewportPanelSize.x;
            m_ViewportHeight = viewportPanelSize.y;
        }
        ImGui::Image((ImTextureID)m_SceneTextures[m_ActiveScene], viewportPanelSize, { 0, 1 }, { 1, 0 });

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
            // Convert mouse to viewport-local
            glm::vec2 mouseViewport = { mousePos.x - viewportMin.x, mousePos.y - viewportMin.y };
            // Flip Y if needed based on how your framebuffer is displayed
            mouseViewport.y = m_ViewportHeight - mouseViewport.y;

            // Call the scene picking
            auto picked = m_ActiveScene->PickEntity(mouseViewport, glm::vec2(m_ViewportWidth, m_ViewportHeight));
            if (picked)
                m_ActiveScene->SetSelectedEntity(picked);
        }

        // ------------------------------------------------------------
        // HANDLING IMGUIZMO MANIPULATION
        // ------------------------------------------------------------
        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImVec2 viewportPos = ImGui::GetWindowPos();
        ImGuizmo::SetRect(viewportPos.x, viewportPos.y, viewportPanelSize.x, viewportPanelSize.y);

        glm::mat4 view = m_ActiveScene->GetViewMatrix();
        glm::mat4 proj = m_ActiveScene->GetProjectionMatrix();

        // Handle CTRL+E to cycle operation
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_E))
        {
            if (s_CurrentOperation == ImGuizmo::TRANSLATE)
                s_CurrentOperation = ImGuizmo::ROTATE;
            else if (s_CurrentOperation == ImGuizmo::ROTATE)
                s_CurrentOperation = ImGuizmo::SCALE;
            else
                s_CurrentOperation = ImGuizmo::TRANSLATE;
        }

        // Manipulate selected entity
        auto selected = m_ActiveScene->GetSelectedEntity();
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

        auto& camera = m_ActiveScene->GetSceneCamera();
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


        // ------------------------------------------------------------
        // DRAWING SCENE ENVIRONMENT SETTINGS
        // ------------------------------------------------------------
        ImGui::Begin(std::format("{} Environment Settings", m_ActiveScene->GetSceneName()).c_str());
        auto& env = m_ActiveScene->GetEnvironment();
        static const ImGuiTreeNodeFlags treeNodeFlags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
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

    void SceneEditorLayer::DrawDockspace()
    {
        // ---- Host window flags
        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGuiDockNodeFlags dock_flags =
            ImGuiDockNodeFlags_PassthruCentralNode |   // central node is transparent
            ImGuiDockNodeFlags_AutoHideTabBar;         // cleaner tabs when single window

        // ---- Fullscreen host window over main viewport
        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        // If you want a menu bar in the host, add ImGuiWindowFlags_MenuBar to host_flags
        ImGui::Begin("##DockHost", nullptr, host_flags);

        ImGui::PopStyleVar(3); // padding, border, rounding

        // ---- Create dockspace
        ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            // Background for passthrough central node:
            // Make the host window bg clear so your viewport can draw under it.
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            ImGui::DockSpace(dockspace_id, ImVec2(0, 0), dock_flags);
            ImGui::PopStyleColor();
        }

        // ---- Optional top toolbar (thin strip)
        if (ImGui::BeginChild("TopToolbar", ImVec2(0, 36), false,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 6));

            // Place your icon buttons / toggles here
            // Example:
            // if (ImGui::Button(ICON_MD_PLAY_ARROW)) { ... }
            // ImGui::SameLine();
            // if (ImGui::Button(ICON_MD_STOP)) { ... }

            ImGui::PopStyleVar(2);
        }
        ImGui::EndChild();

        // ---- Build a sensible default layout once
        static bool built = false;
        if (!built && (io.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        {
            built = true;

            ImGui::DockBuilderRemoveNode(dockspace_id);                   // clear any previous
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, vp->WorkSize);

            // Split: main -> left/right, keep dockspace_id as center
            ImGuiID dock_main_id = dockspace_id;
            ImGuiID dock_right = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Right, 0.28f, nullptr, &dock_main_id);
            ImGuiID dock_bottom = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.28f, nullptr, &dock_main_id);
            ImGuiID dock_left = ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.22f, nullptr, &dock_main_id);

            // Optionally mark center node as passthrough/content
            // ImGui::DockBuilderGetNode(dock_main_id)->LocalFlags |= ImGuiDockNodeFlags_NoWindowMenuButton;

            // Dock your windows by name (must match ImGui::Begin() titles)
            ImGui::DockBuilderDockWindow("Scene", dock_main_id); // viewport
            ImGui::DockBuilderDockWindow("Outliner", dock_left);
            ImGui::DockBuilderDockWindow("Properties", dock_right);
            ImGui::DockBuilderDockWindow("Console", dock_bottom);
            ImGui::DockBuilderDockWindow("Assets", dock_bottom);

            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::End(); // host
    }
}