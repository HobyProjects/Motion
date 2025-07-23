#include "CorePCH.hpp"
#include "Application.hpp"

namespace Motion
{
    Application::Application()
    {
        auto& windowManager = WindowManager::GetInstance();
        m_Window = windowManager.Create("Motion Engine");
        m_Window->SetEventsCallbackFunc(EVENT_CALLBACK(OnEvent));

        Renderer::Init();
        UserInterfaceInitializer::Init(m_Window->GetHandle());

        m_ImGuiLayer = std::make_shared<ImGuiLayer>(m_Window->GetHandle(), ImGuiColorScheme::Dark);
        m_EditorLayer = std::make_shared<SceneEditorLayer>(m_Window->GetHandle(), m_ImGuiLayer);

        PushOverlay(m_ImGuiLayer);
        PushLayer(m_EditorLayer);
    }

    Application::~Application()
    {
        UserInterfaceInitializer::Quit();
        Renderer::Quit();

        auto& windowManager = WindowManager::GetInstance();
        windowManager.Destroy(m_Window->GetHandle());
    }

    void Application::Start()
    {
        while (m_Window->IsActive())
        {
            m_Window->PollEvents();
            auto& layersManager = LayersManager::GetInstance();

            if (m_Window->GetProperties().State != WindowState::Minimized)
            {
                float currentTime{ 0.0f };
                currentTime = SystemTimer<float>::GetSystemTicks();

                Timer deltaTime = currentTime - m_LastFrameTime;
                m_LastFrameTime = currentTime;

                for (auto& layer : layersManager)
                {
                    layer->OnUpdate(m_Window->GetHandle(), deltaTime);
                }
            }

            m_ImGuiLayer->Begin();

            for (auto& layer : layersManager)
            {
                layer->OnUIRender(m_Window->GetHandle());
            }

            m_ImGuiLayer->End();
            m_Window->SwapBuffers();
        }
    }

    void Application::PushLayer(const std::shared_ptr<Layer>& layer)
    {
        layer->OnAttach();
        LayersManager::GetInstance().PushLayer(layer);
    }

    void Application::PushOverlay(const std::shared_ptr<Layer>& layer)
    {
        layer->OnAttach();
        LayersManager::GetInstance().PushOverlay(layer);
    }

    void Application::OnEvent(WindowHandle handle, IEvent& e)
    {
        EventHandler handler(handle, e);
        handler.Dispatch<EventWindowClose>(EVENT_CALLBACK(OnWindowClose));
        handler.Dispatch<EventWindowResize<uint32_t>>(EVENT_CALLBACK(OnWindowResize));

        auto& layersManager = LayersManager::GetInstance();
        for (std::vector<std::shared_ptr<Layer>>::reverse_iterator it = layersManager.rbegin(); it != layersManager.rend(); ++it)
        {
            if (handler.IsHandled())
                break;

            (*it)->OnEvent(handle, e);
        }
    }

    bool Application::OnWindowClose(WindowHandle handle, EventWindowClose& e)
    {
        if (m_Window->IsActive())
            m_Window->GetProperties().IsActive = false;

        return false;

    }

    bool Application::OnWindowResize(WindowHandle handle, EventWindowResize<uint32_t>& e)
    {
        if (m_Window->GetProperties().State != WindowState::Minimized)
            Core::Renderer::SetViewport(0, 0, e.Width(), e.Height());

        return false;
    }
}