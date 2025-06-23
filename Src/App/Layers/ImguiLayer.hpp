#pragma once

#include "Layer.hpp"
#include "Window.hpp"
#include "UI.hpp"

namespace Motion::App
{
    enum class ImGuiColorScheme
    {
        Light,
        Dark,
    };

    class ImGuiLayer final : public Motion::Core::Layer
    {
        public:
            ImGuiLayer() : Motion::Core::Layer("ImGuiLayer") {}
            ImGuiLayer(Motion::Core::WindowHandle handle, ImGuiColorScheme colorScheme = ImGuiColorScheme::Dark);
            virtual ~ImGuiLayer() = default;

            virtual void OnAttach() override;
            virtual void OnDetach() override;
            virtual void OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e) override;

            void Begin();
            void End();
            void AcceptEvents(bool allowed) { m_AllowEvents = allowed; }
            void UseColorScheme(ImGuiColorScheme colorScheme);

        private:
            Motion::Core::WindowHandle m_WindowHandle{0};
            ImGuiColorScheme m_ColorScheme{ ImGuiColorScheme::Dark };
            bool m_AllowEvents{ false };
    };

}