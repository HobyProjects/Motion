#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <type_traits>

#include "Window.hpp"

namespace Motion::Core
{
    class Timer
    {
        public:
            Timer(float deltaTime = 0.0f) : m_DeltaTime(deltaTime) {}
            ~Timer() = default;

            float GetDeltaTime() const { return m_DeltaTime; }
            float GetDeltaTimeSeconds() const { return m_DeltaTime; }
            float GetDeltaTimeMilliseconds() const { return m_DeltaTime * 1000.0f; }

            operator float() const { return m_DeltaTime; }

        private:
            float m_DeltaTime{ 0.0f };
    };

    template <typename T>
    requires std::is_floating_point<T>::value
    class SystemTimer
    {
        private:
            SystemTimer() = default;
            ~SystemTimer() = default;

        public:
            static T GetSystemTicks()
            {
                switch(CoreAPI::GetBaseAPI()->API())
                {
                    case BaseAPIs::GLFW:
                        return static_cast<T>(glfwGetTime());
                    case BaseAPIs::Win32:
                        MOTION_ASSERT(false, "Win32 is not supported yet") return static_cast<T>(0);
                }
            }

            static T GetSystemTicksSeconds()
            {
                switch(CoreAPI::GetBaseAPI()->API())
                {
                    case BaseAPIs::GLFW:
                        return static_cast<T>(glfwGetTime());
                    case BaseAPIs::Win32:
                        MOTION_ASSERT(false, "Win32 is not supported yet") return static_cast<T>(0);
                }
            }

            static T GetSystemTicksMilliseconds()
            {
                switch(CoreAPI::GetBaseAPI()->API())
                {
                    case BaseAPIs::GLFW:
                        return static_cast<T>(glfwGetTime() * 1000.0f);
                    case BaseAPIs::Win32:
                        MOTION_ASSERT(false, "Win32 is not supported yet") return static_cast<T>(0);
                }
            }
    };
}