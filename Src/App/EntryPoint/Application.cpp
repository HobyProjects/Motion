#include "Application.hpp"
#include "Renderer.hpp"

namespace Motion::App
{
    Application::Application()
    {
        Motion::Core::CoreAPI::Init();
        m_Window = Motion::Core::WindowManager::Create("Motion Engine");
        m_Window->SetEventsCallbackFunc(EVENT_CALLBACK(OnEvent));
        Motion::Core::Renderer::Init();
        Motion::Core::UI::Init(m_Window->GetHandle());

        m_LayersManager = std::make_shared<Motion::Core::LayersManager>();
        m_ImGuiLayer = std::make_shared<ImGuiLayer>(m_Window->GetHandle(), ImGuiColorScheme::Dark);

        PushOverlay(m_ImGuiLayer);
    }

    Application::~Application()
    {
        Motion::Core::Renderer::Quit();
        Motion::Core::UI::Quit();
        Motion::Core::WindowManager::Destroy(m_Window);
        Motion::Core::CoreAPI::Quit();
    }

    void Application::Start()
    {
        while(m_Window->IsActive())
        {
            m_Window->PollEvents();
            Motion::Core::Renderer::Clear();
            Motion::Core::Renderer::ClearColor({ 255.0f, 0.0f, 0.0f, 255.0f });

            if(m_Window->GetProperties().State != Motion::Core::WindowState::Minimized)
            {
                float currentTime{0.0f};
                currentTime = Motion::Core::SystemTimer<float>::GetSystemTicks();

                Motion::Core::Timer deltaTime = currentTime - m_LastFrameTime;
                m_LastFrameTime = currentTime;

                for(auto& layer : *m_LayersManager)
                {
                    layer->OnUpdate(m_Window->GetHandle(), deltaTime);
                }
            }

            m_ImGuiLayer->Begin();

            for(auto& layer : *m_LayersManager)
            {
                layer->OnUIRender(m_Window->GetHandle());
            }

            m_ImGuiLayer->End();
            m_Window->SwapBuffers();
        }
    }

    void Application::PushLayer(const std::shared_ptr<Motion::Core::Layer>& layer)
    {
        layer->OnAttach();
        m_LayersManager->PushLayer(layer);
    }

    void Application::PushOverlay(const std::shared_ptr<Motion::Core::Layer>& layer)
    {
        layer->OnAttach();
        m_LayersManager->PushOverlay(layer);
    }

    void Application::OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e)
    {
        Motion::Core::EventHandler handler(handle, e);
        handler.Dispatch<Motion::Core::EventWindowClose>(EVENT_CALLBACK(OnWindowClose));
        handler.Dispatch<Motion::Core::EventWindowResize<uint32_t>>(EVENT_CALLBACK(OnWindowResize));

        for( std::vector<std::shared_ptr<Motion::Core::Layer>>::reverse_iterator it = m_LayersManager->rbegin(); it != m_LayersManager->rend(); ++it )
		{
			if( handler.IsHandled() )
				break;

			( *it )->OnEvent(handle, e);
		}
    }

    bool Application::OnWindowClose(Motion::Core::WindowHandle handle, Motion::Core::EventWindowClose& e)
    {
        if(m_Window->IsActive())
            m_Window->GetProperties().IsActive = false;

        return false;

    }

    bool Application::OnWindowResize(Motion::Core::WindowHandle handle, Motion::Core::EventWindowResize<uint32_t>& e)
    {
        if(m_Window->GetProperties().State != Motion::Core::WindowState::Minimized)
            Motion::Core::Renderer::SetViewport(0, 0, e.Width(), e.Height());

        return false;
    }
}