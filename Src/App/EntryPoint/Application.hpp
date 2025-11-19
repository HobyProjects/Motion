#pragma once

#include "Event.hpp"
#include "Window.hpp"
#include "Layer.hpp"
#include "LayersManager.hpp"
#include "Timer.hpp"

#include "PresentationLayer.hpp"
#include "SceneEditorLayer.hpp"

namespace Motion
{
    class Application
    {
        public:
            Application();
            ~Application();

            void Start();
            void OnEvent(WindowHandle handle, IEvent& e);

            void PushLayer(const std::shared_ptr<Layer>& layer);
            void PushOverlay(const std::shared_ptr<Layer>& layer);

        private:
            bool OnWindowClose(WindowHandle handle, EventWindowClose& e);
            bool OnWindowResize(WindowHandle handle, EventWindowResize& e);

        private:
            std::shared_ptr<IWindow> m_Window{ nullptr };
            std::shared_ptr<IContext> m_WindowContext{ nullptr };

            std::shared_ptr<PresentationLayer> m_PresentationLayer{ nullptr };
            std::shared_ptr<SceneEditorLayer> m_EditorLayer{ nullptr };
    };
}
