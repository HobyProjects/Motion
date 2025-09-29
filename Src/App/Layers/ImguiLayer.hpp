#pragma once

#include "Layer.hpp"
#include "Window.hpp"
#include "UI.hpp"

namespace Motion
{
    enum class ImGuiColorScheme
    {
        Light,
        Dark,
    };

    class ImGuiLayer final : public Layer
    {
        public:
            ImGuiLayer() : Layer("ImGuiLayer") {}
            ImGuiLayer(WindowHandle handle, ImGuiColorScheme colorScheme = ImGuiColorScheme::Dark);
            virtual ~ImGuiLayer() = default;

            virtual void OnAttach() override;
            virtual void OnDetach() override;
            virtual void OnEvent(WindowHandle handle, IEvent& e) override;

            void Begin();
            void End();
            void AcceptEvents(bool allowed) { m_AllowEvents = allowed; }
            void UseColorScheme(ImGuiColorScheme colorScheme);

        private:
            WindowHandle m_WindowHandle{ 0 };
            ImGuiColorScheme m_ColorScheme{ ImGuiColorScheme::Dark };
            bool m_AllowEvents{ false };
    };

}