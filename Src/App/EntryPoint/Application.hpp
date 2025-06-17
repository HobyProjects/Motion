#pragma once

#include "Event.hpp"
#include "Window.hpp"
#include "Layer.hpp"
#include "LayersManager.hpp"

namespace Motion::App
{
    class Application
    {
        public:
            Application();
            ~Application();

            void Start();
            void OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e);
            
            void PushLayer(const std::shared_ptr<Motion::Core::Layer>& layer);
            void PushOverlay(const std::shared_ptr<Motion::Core::Layer>& layer);

        private:
            bool OnWindowClose(Motion::Core::WindowHandle handle, Motion::Core::EventWindowClose& e);
            bool OnWindowResize(Motion::Core::WindowHandle handle, Motion::Core::EventWindowResize<uint32_t>& e);

        private:
            std::shared_ptr<Motion::Core::IWindow> m_Window{nullptr};
            std::shared_ptr<Motion::Core::LayersManager> m_LayersManager{nullptr};
    };
}
