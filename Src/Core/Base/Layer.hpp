#pragma once

#include <string>

#include "Event.hpp"
#include "Timer.hpp"

namespace Motion::Core
{
    class Layer
    {
        public:
            Layer(const std::string& name): m_LayerName(name){}
            virtual ~Layer() = default;

            virtual void OnAttach(){}
            virtual void OnDetach(){}
            virtual void OnUpdate(Timer deltaTime){}
            virtual void OnEvent(IEvent& event){}

            const std::string& GetName() const { return m_LayerName; }
        
        protected:
            std::string m_LayerName{};
    };
}