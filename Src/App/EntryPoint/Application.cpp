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

        m_LayersManager = std::make_shared<Motion::Core::LayersManager>();
    }

    Application::~Application()
    {
        Motion::Core::Renderer::Quit();
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

            m_Window->SwapBuffers();
        }
    }

    void Application::OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e)
    {
        Motion::Core::EventHandler handler(handle, e);
        handler.Dispatch<Motion::Core::EventWindowClose>(EVENT_CALLBACK(OnWindowClose));
        handler.Dispatch<Motion::Core::EventWindowResize<uint32_t>>(EVENT_CALLBACK(OnWindowResize));
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