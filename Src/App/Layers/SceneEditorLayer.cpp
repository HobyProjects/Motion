#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    static std::shared_ptr<ImGuiLayer> s_ImGuiLayer{ nullptr };

    SceneEditorLayer::SceneEditorLayer(WindowHandle handle, const std::shared_ptr<ImGuiLayer>& imguiLayer) : Layer("EditorLayer")
    {
        s_ImGuiLayer = imguiLayer;
        m_Panels = std::make_shared<ScenePanelManager>();
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
        m_Environment = IEnvironment::Create("Assets/HDRI/Scene4.hdr");

        m_Viewport.FrameSpec.Name = "SceneEditorFrame";
        m_Viewport.FrameSpec.Width = static_cast<uint32_t>(m_CurrentViewportSize.x);
        m_Viewport.FrameSpec.Height = static_cast<uint32_t>(m_CurrentViewportSize.y);
        m_Viewport.Size = { m_CurrentViewportSize.x, m_CurrentViewportSize.y };
        m_Framebuffer = IFrameBuffer::Create(m_Viewport.FrameSpec);

        SceneSpecification spec;
        spec.Name = "Default Scene";
        spec.IsActive = true;
        spec.Viewport = m_Viewport;
        spec.Environment = SceneEnvironment();

        if (m_Scenes.empty())
        {
            //[TODO] : When scene serialization is implemented, load the default scene from a file or create a new one.
            m_Scenes.push_back(std::make_shared<Scene>(spec));
        }

        m_ActiveScene = m_Scenes[0];
        m_ActiveScene->Activate(true);

        m_Panels->Emplace<SceneViewportPanel>();
        m_Panels->Emplace<SceneEntityInspectPanel>();
        m_Panels->Emplace<SceneEntityPropertiesPanel>();
        m_Panels->Emplace<SceneSettingsPanel>();
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
        if (m_ActiveScene->GetSpecification().Viewport.Size != m_CurrentViewportSize)
        {
            m_CurrentViewportSize = m_ActiveScene->GetSpecification().Viewport.Size;
            m_Framebuffer->ResizeFrame((uint32_t)m_CurrentViewportSize.x, (uint32_t)m_CurrentViewportSize.y);
            m_ActiveScene->OnViewportSizeChanges(m_CurrentViewportSize);
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
        m_Environment->Render(m_ActiveScene->GetCameraView(), m_ActiveScene->GetCameraProjection());
        SceneRenderer::BeginScene();
        SceneRenderer::Submit(m_ActiveScene.get(), m_Environment.get());
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
        BuildDockspace();

        ScenePanelContext panelContext;
        panelContext.ActiveScene = m_ActiveScene;
        panelContext.ActiveCamera = m_ActiveScene->GetCamera();
        panelContext.ActiveSceneSpecification = m_ActiveScene->GetSpecification();
        panelContext.ActiveViewportTexture = m_SceneTextures[m_ActiveScene];
        panelContext.UILayerInstance = s_ImGuiLayer.get();

        for (const auto& panel : *m_Panels)
            panel->RenderUI(panelContext);
    }

    void SceneEditorLayer::BuildDockspace()
    {
        // Host window
        ImGuiWindowFlags host_flags =
            ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
            ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoScrollWithMouse;

        ImGuiDockNodeFlags dock_flags =
            ImGuiDockNodeFlags_PassthruCentralNode |
            ImGuiDockNodeFlags_AutoHideTabBar;

        const ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->WorkPos);
        ImGui::SetNextWindowSize(vp->WorkSize);
        ImGui::SetNextWindowViewport(vp->ID);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("##DockHost", nullptr, host_flags);
        ImGui::PopStyleVar(3);

        ImGuiID dockspace_id = ImGui::GetID("MainDockspace");
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
            ImGui::DockSpace(dockspace_id, ImVec2(0, 0), dock_flags);
            ImGui::PopStyleColor();
        }

        static bool built = false;
        if (!built && (io.ConfigFlags & ImGuiConfigFlags_DockingEnable))
        {
            built = true;
            ImGui::DockBuilderRemoveNode(dockspace_id);
            ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspace_id, vp->WorkSize);

            ImGuiID CENTER_NODE = dockspace_id;
            ImGuiID RIGHT = ImGui::DockBuilderSplitNode(CENTER_NODE, ImGuiDir_Right, 0.28f, nullptr, &CENTER_NODE);
            ImGuiID BOTTOM = ImGui::DockBuilderSplitNode(CENTER_NODE, ImGuiDir_Down, 0.28f, nullptr, &CENTER_NODE);
            ImGuiID LEFT = ImGui::DockBuilderSplitNode(CENTER_NODE, ImGuiDir_Left, 0.22f, nullptr, &CENTER_NODE);

            for (const auto& p : *m_Panels)
            {
                std::string title = p->GetTitle();

                switch (p->GetCategory())
                {
                case PanelCategory::ScenePanel:      ImGui::DockBuilderDockWindow(title.c_str(), CENTER_NODE); break;
                case PanelCategory::PropertiesPanel:  ImGui::DockBuilderDockWindow(title.c_str(), LEFT); break;
                case PanelCategory::InspectorPanel:  ImGui::DockBuilderDockWindow(title.c_str(), RIGHT); break;
                case PanelCategory::AssetsPanel:     ImGui::DockBuilderDockWindow(title.c_str(), BOTTOM); break;
                case PanelCategory::ConsolePanel:     ImGui::DockBuilderDockWindow(title.c_str(), BOTTOM); break;
                }
            }

            ImGui::DockBuilderFinish(dockspace_id);
        }

        ImGui::End(); // host
    }
}