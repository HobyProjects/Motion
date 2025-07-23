#pragma once

#include <string>

#include "Window.hpp"
#include "Event.hpp"
#include "Timer.hpp"

namespace Motion
{
    class Layer
    {
    public:
        Layer(const std::string& name) : m_LayerName(name) {}
        virtual ~Layer() = default;

        virtual void OnAttach() {}
        virtual void OnDetach() {}
        virtual void OnUpdate(WindowHandle handle, Timer deltaTime) {}
        virtual void OnEvent(WindowHandle handle, IEvent& e) {}
        virtual void OnUIRender(WindowHandle handle) {}

        const std::string& GetName() const { return m_LayerName; }

    protected:
        std::string m_LayerName{};
    };
}