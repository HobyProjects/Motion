#pragma once

#include "Layer.hpp"
#include "Window.hpp"
#include "UI.hpp"
#include "Toast.hpp"

namespace Motion
{
    class ImGuiLayer final : public Layer
    {
        public:
            ImGuiLayer() : Layer("ImGuiLayer") {}
            ImGuiLayer(WindowHandle handle);
            virtual ~ImGuiLayer() = default;

            virtual void OnAttach() override;
            virtual void OnDetach() override;
            virtual void OnUpdate(WindowHandle handle, Timer deltaTime) override;
            virtual void OnEvent(WindowHandle handle, IEvent& e) override;

            void Begin();
            void End();
            void AcceptEvents(bool allowed) { m_AllowEvents = allowed; }

        private:
            void RenderLogConsoleWindow();

        private:
            WindowHandle m_WindowHandle{ 0 };
            std::unique_ptr<ToastManager> m_ToastManager;
            bool m_ShowLogConsole{ true };
            bool m_AllowEvents{ false };
    };

}