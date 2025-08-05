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

        Material::ImportMaterial("Assets/Materials/Base/Base.yaml");
        m_Environment = IEnvironment::Create("Assets/HDRI/Scene.hdr");

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

        bool isHovered = ImGui::IsItemHovered();
        bool isClicked = ImGui::IsItemClicked() && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (isHovered && isClicked && !ImGuizmo::IsUsing())
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

        ImGui::End();
        ImGui::PopStyleVar();


        // ------------------------------------------------------------
        // DRAWING SCENE ENVIRONMENT SETTINGS
        // ------------------------------------------------------------
        ImGui::Begin(std::format("{} Environment Settings", m_ActiveScene->GetSceneName()).c_str());
        auto& env = m_ActiveScene->GetEnvironment();
        static const ImGuiTreeNodeFlags treeNodeFlags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        bool open = ImGui::TreeNodeEx((void*)env.DirectionalLight.LightID, treeNodeFlags, "Environment Lighting");
        if (open)
        {
            CustomUIControl::DrawFloat3("Direction", env.DirectionalLight.Direction, 0.0f);
            CustomUIControl::DrawColor3("Color", env.DirectionalLight.Color);
            CustomUIControl::DrawFloat("Intensity", env.DirectionalLight.AmbientIntensity, 0.0f, 1.0f, 0.005f);
            ImGui::TreePop();
        }
        ImGui::End();
    }

    void SceneEditorLayer::DrawDockspace()
    {
        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        if (opt_fullscreen)
        {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        }
        else
        {
            dockspace_flags &= -ImGuiDockNodeFlags_PassthruCentralNode;
        }

        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;

        if (!opt_padding)
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        static bool show_dockspace = true;
        ImGui::Begin("Dockspace", &show_dockspace, window_flags);

        if (!opt_padding)
            ImGui::PopStyleVar();

        if (opt_fullscreen)
            ImGui::PopStyleVar(2);

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        style.WindowMinSize.x = minWinSizeX;

        ImGui::End();
    }
}