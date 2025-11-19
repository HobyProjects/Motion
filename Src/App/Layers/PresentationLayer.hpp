#pragma once

#include "Layer.hpp"
#include "Window.hpp"
#include "UI.hpp"
#include "Toast.hpp"

namespace Motion
{
    class PresentationLayer final : public Layer
    {
    public:
        PresentationLayer() : Layer("PresentationLayer") {}
        PresentationLayer(WindowHandle handle);
        virtual ~PresentationLayer() = default;

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
        bool m_ShowLogConsole{ true };
        bool m_AllowEvents{ false };
    };

}