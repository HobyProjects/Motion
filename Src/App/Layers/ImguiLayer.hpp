#pragma once

#include "Layer.hpp"
#include "Window.hpp"
#include "UI.hpp"

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
            virtual void OnEvent(WindowHandle handle, IEvent& e) override;

            void Begin();
            void End();
            void AcceptEvents(bool allowed) { m_AllowEvents = allowed; }

        private:
            WindowHandle m_WindowHandle{ 0 };
            bool m_AllowEvents{ false };
    };

}