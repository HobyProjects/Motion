#include "CorePCH.hpp"
#include "EditorLayer.hpp"

namespace Motion::App
{
    static std::weak_ptr<Motion::Core::IWindow> s_Window;
    static std::weak_ptr<Motion::App::ImGuiLayer> s_ImGuiLayer;

    EditorLayer::EditorLayer(Motion::Core::WindowHandle handle, const std::shared_ptr<Motion::App::ImGuiLayer>& imguiLayer) : Motion::Core::Layer("EditorLayer") 
    {
        s_Window = Motion::Core::WindowManager::Get(handle);
        s_ImGuiLayer = imguiLayer;
    }

    void EditorLayer::OnAttach()
    {
        m_Viewport.FrameSpec.Width = static_cast<uint32_t>(m_ViewportWidth);
        m_Viewport.FrameSpec.Height = static_cast<uint32_t>(m_ViewportHeight);
        m_Viewport.Size = { m_ViewportWidth, m_ViewportHeight };

        if(!s_Window.expired())
        {
            auto window = s_Window.lock();
            Motion::Core::GraphicSettings& graphicSettings = window->GetGraphicSettings();
            Motion::Core::GraphicSettings::AntiAliasingLevel aaLevel = graphicSettings.AntiAliasing;
            m_Viewport.FrameSpec.Samples = static_cast<uint32_t>(aaLevel);
        }

        m_Framebuffer = Motion::Core::BufferFactory::CreateFrameBuffer(m_Viewport.FrameSpec);
        m_Scene = std::make_shared<Scene>(glm::vec2(m_ViewportWidth, m_ViewportHeight));
    }

    void EditorLayer::OnDetach()
    {
        m_Framebuffer.reset();
        m_Scene.reset();
    }

    void EditorLayer::OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime)
    {
        if(m_Viewport.SizeHasChanged(m_ViewportWidth, m_ViewportHeight))
        {
            m_Viewport.Update(glm::vec2(m_ViewportWidth, m_ViewportHeight));
            m_Framebuffer->ResizeFrame((uint32_t) m_ViewportWidth, (uint32_t) m_ViewportHeight);
            m_Scene->OnViewportSizeChanges(m_ViewportWidth, m_ViewportHeight);
        }

        m_Framebuffer->Bind();

        Motion::Core::Renderer::ClearColor({ 0.243, 0.243, 0.243, 1.0f });
        Motion::Core::Renderer::Clear();
        
        m_Scene->OnUpdate(handle, deltaTime);

        m_Framebuffer->Unbind();
    }

    void EditorLayer::OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e)
    {
        m_Scene->OnEvent(handle, e);
    }

    void EditorLayer::OnUIRender(Motion::Core::WindowHandle handle)
    {
        DrawDockspace();
        ImGui::ShowDemoWindow();
        m_Scene->OnUIRenders(handle);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Scene");
        if(!s_ImGuiLayer.expired())
        {
            auto imguiLayer = s_ImGuiLayer.lock();
            imguiLayer->AcceptEvents(ImGui::IsWindowFocused() || ImGui::IsWindowHovered());
            ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
            if( viewportPanelSize.x != m_ViewportWidth || viewportPanelSize.y != m_ViewportHeight )
            {
                m_ViewportWidth = viewportPanelSize.x;
                m_ViewportHeight = viewportPanelSize.y;
            }

            if( m_Framebuffer->IsMSAA() )
            {
                m_Framebuffer->Resolve();
                ImGui::Image((ImTextureID) m_Framebuffer->GetResolvedColorAttachment(), viewportPanelSize, { 0, 1 }, { 1, 0 });
            }
            else
            {
                ImGui::Image((ImTextureID) m_Framebuffer->GetColorAttachment(), viewportPanelSize, { 0, 1 }, { 1, 0 });
            }

        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorLayer::DrawDockspace()
    {
        static bool opt_fullscreen = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

        if( opt_fullscreen )
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

        if( dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode )
            window_flags |= ImGuiWindowFlags_NoBackground;

        if( !opt_padding )
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        static bool show_dockspace = true;
        ImGui::Begin("Dockspace", &show_dockspace, window_flags);

        if( !opt_padding )
            ImGui::PopStyleVar();

        if( opt_fullscreen )
            ImGui::PopStyleVar(2);

        ImGuiIO& io = ImGui::GetIO();
        ImGuiStyle& style = ImGui::GetStyle();
        float minWinSizeX = style.WindowMinSize.x;
        style.WindowMinSize.x = 370.0f;
        if( io.ConfigFlags & ImGuiConfigFlags_DockingEnable )
        {
            ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        style.WindowMinSize.x = minWinSizeX;

        ImGui::End();
    }
}