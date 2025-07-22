#include "CorePCH.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion::App
{
    static std::weak_ptr<Motion::Core::IWindow> s_Window;
    static std::weak_ptr<Motion::App::ImGuiLayer> s_ImGuiLayer;

    SceneEditorLayer::SceneEditorLayer(Motion::Core::WindowHandle handle, const std::shared_ptr<Motion::App::ImGuiLayer>& imguiLayer)
        : Motion::Core::Layer("EditorLayer")
    {
        s_ImGuiLayer = imguiLayer;
    }

    void SceneEditorLayer::OnAttach()
    {
        //TEMP
        auto& assetManager = Motion::Core::AssetManager::GetInstance();
        assetManager.Create<Motion::Core::IShader>("PostProcessingShader", "Assets/Shaders/PostProcessing.glsl");
        assetManager.Create<Motion::Core::IShader>("SkyBoxShader", "Assets/Shaders/SkyBox.glsl");
        assetManager.Create<Motion::Core::IShader>("PBRShader", "Assets/Shaders/PBRShader.glsl");
        assetManager.Create<Motion::Core::IShader>("PhongShader", "Assets/Shaders/PhongShader.glsl");
        assetManager.Create<Motion::Core::IShader>("UnlitShader", "Assets/Shaders/UnlitShader.glsl");
        Motion::Core::MaterialFallbackTextures::Initialize();

        m_Viewport.FrameSpec.Width = static_cast<uint32_t>(m_ViewportWidth);
        m_Viewport.FrameSpec.Height = static_cast<uint32_t>(m_ViewportHeight);
        m_Viewport.Size = { m_ViewportWidth, m_ViewportHeight };

        m_Framebuffer = Motion::Core::BufferFactory::CreateFrameBuffer(m_Viewport.FrameSpec);
        m_PostProcessor = std::make_unique<Motion::Core::PostProcessor>(m_Framebuffer->GetFrameSpecification());
        m_SkyBox = std::make_unique<Motion::Core::SkyBox>();

        if (m_Scenes.empty())
        {
            //[TODO] : When scene serialization is implemented, load the default scene from a file or create a new one.
            m_Scenes.push_back(std::make_shared<Scene>(Motion::Core::UniqueIdentity::GetUniqueID(), "Default Scene", glm::vec2(m_ViewportWidth, m_ViewportHeight)));
        }

        m_ActiveScene = m_Scenes[0];
        m_ActiveScene->SetActive(true);
    }

    void SceneEditorLayer::OnDetach()
    {
        m_Framebuffer.reset();
        m_PostProcessor.reset();
        m_Scenes.clear();
    }

    void SceneEditorLayer::OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime)
    {
        if (m_Viewport.SizeHasChanged(m_ViewportWidth, m_ViewportHeight))
        {
            m_Viewport.Update(glm::vec2(m_ViewportWidth, m_ViewportHeight));
            m_Framebuffer->ResizeFrame((uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight);
            m_PostProcessor->OnResize((uint32_t)m_ViewportWidth, (uint32_t)m_ViewportHeight);
            m_ActiveScene->OnViewportSizeChanges(m_ViewportWidth, m_ViewportHeight);
        }

        m_ActiveScene->OnUpdate(handle, deltaTime);


        m_Framebuffer->Bind();

        Motion::Core::Renderer::ClearColor({ 0.243, 0.243, 0.243, 1.0f });
        Motion::Core::Renderer::Clear();


        auto& sceneRenderer = SceneRenderer::GetInstance();
        sceneRenderer.BeginScene();
        m_SkyBox->Render(m_ActiveScene->GetViewMatrix(), m_ActiveScene->GetProjectionMatrix());
        sceneRenderer.Submit(m_ActiveScene.get());
        sceneRenderer.EndScene(m_SkyBox->GetTextureID());

        m_Framebuffer->Unbind();
        m_PostProcessor->Process(m_Framebuffer->GetAttachment(Motion::Core::FrameBufferColorAttachmentStandards::Standard).TextureID);
        m_SceneTextures[m_ActiveScene] = m_PostProcessor->GetOutputTextureID();
    }

    void SceneEditorLayer::OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e)
    {
        m_ActiveScene->OnEvent(handle, e);
    }

    void SceneEditorLayer::OnUIRender(Motion::Core::WindowHandle handle)
    {
        DrawDockspace();
        ImGui::ShowDemoWindow();
        m_ActiveScene->OnUIRenders(handle);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin(m_ActiveScene->GetSceneName().c_str());
        if (!s_ImGuiLayer.expired())
        {
            auto imguiLayer = s_ImGuiLayer.lock();
            imguiLayer->AcceptEvents(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            if (viewportPanelSize.x != m_ViewportWidth || viewportPanelSize.y != m_ViewportHeight)
            {
                m_ViewportWidth = viewportPanelSize.x;
                m_ViewportHeight = viewportPanelSize.y;
            }

            ImGui::Image((ImTextureID)m_SceneTextures[m_ActiveScene], viewportPanelSize, { 0, 1 }, { 1, 0 });
        }
        ImGui::End();
        ImGui::PopStyleVar();
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