#include "CorePCH.hpp"
#include "Application.hpp"

namespace Motion
{
    Application::Application()
    {
        auto& windowManager = WindowManager::GetInstance();

        m_Window = windowManager.Create("Motion Engine", true);
        m_Window->SetEventsCallbackFunc(EventCallbackFn(&Application::OnEvent, this));

        m_WindowContext = IContext::GetContext();
        m_WindowContext->MakeCurrent(m_Window->GetNativeWindow());
        m_WindowContext->Create();

        Renderer::Init();
        UserInterface::Init(m_Window->GetHandle());

        LOADER::Create(m_Window->GetNativeWindow());
        LOADER::Start();

        m_ImGuiLayer    = std::make_shared<ImGuiLayer>(m_Window->GetHandle());
        m_EditorLayer   = std::make_shared<SceneEditorLayer>();

        PushOverlay(m_ImGuiLayer);
        PushLayer(m_EditorLayer);
    }

    Application::~Application()
    {
        UserInterface::Quit();
        Renderer::Quit();
        LOADER::Stop();

        auto& windowManager = WindowManager::GetInstance();
        windowManager.Destroy(m_Window->GetHandle());
    }

    void Application::Start()
    {
        using clock = std::chrono::steady_clock;
        using secondsf = std::chrono::duration<float>;
        auto lastFrame = clock::now();

        auto& lm = LayersManager::GetInstance();

        m_WindowContext->MakeCurrent(m_Window->GetNativeWindow());

        while (m_Window->IsActive())
        {
            m_Window->PollEvents();
            LOADER::FeedBack();

            if (!m_Window->IsFocused() || m_Window->GetProperties().State == WindowState::Minimized)
                continue;

            auto now = clock::now();
            float dt = std::chrono::duration_cast<secondsf>(now - lastFrame).count();
            lastFrame = now;

            for (auto& layer : lm) layer->OnUpdate(m_Window->GetHandle(), dt);

            m_ImGuiLayer->Begin();
            for (auto& layer : lm) layer->OnUIRender(m_Window->GetHandle());
            m_ImGuiLayer->End();

            m_WindowContext->SwapBuffers(m_Window->GetNativeWindow());
        }
        
        m_WindowContext->ClearCurrent();
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

        auto& lm = LayersManager::GetInstance();
        for (std::vector<std::shared_ptr<Layer>>::reverse_iterator it = lm.rbegin(); it != lm.rend(); ++it)
        {
            (*it)->OnEvent(handle, e);
        }
    }

    bool Application::OnWindowClose(WindowHandle handle, EventWindowClose& e)
    {
        if (m_Window->IsActive())
        {
            LOADER::Stop();
            m_Window->GetProperties().IsActive = false;
        }

        return false;
    }

    bool Application::OnWindowResize(WindowHandle handle, EventWindowResize& e)
    {
        if (m_Window->GetProperties().State != WindowState::Minimized)
            Renderer::SetViewport(0, 0, e.Width(), e.Height());

        return false;
    }
}