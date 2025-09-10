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
        TaskManager::Instance().Shutdown();
        UserInterfaceInitializer::Quit();
        Renderer::Quit();

        auto& windowManager = WindowManager::GetInstance();
        windowManager.Destroy(m_Window->GetHandle());
    }

    void Application::Start()
    {
        using clock     = std::chrono::steady_clock;
        using secondsf  = std::chrono::duration<float>;
        auto lastFrame  = clock::now();

        auto& LM = LayersManager::GetInstance();

        while (m_Window->IsActive() && m_Window->IsFocused() && m_Window->GetProperties().State != WindowState::Minimized)
        {
            m_Window->PollEvents();

            auto now    = clock::now();
            float dt    = std::chrono::duration_cast<secondsf>(now - lastFrame).count();
            lastFrame   = now;

            for (auto& layer : LM)
            {
                layer->OnUpdate(m_Window->GetHandle(), dt);
            }

            MainThreadDispatcher::Instance().Dispatch();

            m_ImGuiLayer->Begin();

            for (auto& layer : LM)
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
        handler.Dispatch<EventWindowResize>(EVENT_CALLBACK(OnWindowResize));

        auto& LM = LayersManager::GetInstance();
        for (std::vector<std::shared_ptr<Layer>>::reverse_iterator it = LM.rbegin(); it != LM.rend(); ++it)
        {
            (*it)->OnEvent(handle, e);
        }
    }

    bool Application::OnWindowClose(WindowHandle handle, EventWindowClose& e)
    {
        if (m_Window->IsActive())
            m_Window->GetProperties().IsActive = false;

        return false;
    }

    bool Application::OnWindowResize(WindowHandle handle, EventWindowResize& e)
    {
        if (m_Window->GetProperties().State != WindowState::Minimized)
            Renderer::SetViewport(0, 0, e.Width(), e.Height());

        return false;
    }
}