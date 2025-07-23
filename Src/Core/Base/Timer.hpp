#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <type_traits>

#include "Window.hpp"

namespace Motion
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

        SystemTimer(const SystemTimer&) = delete;
        SystemTimer& operator=(const SystemTimer&) = delete;
        SystemTimer(SystemTimer&&) = delete;
        SystemTimer& operator=(SystemTimer&&) = delete;

    public:

        /**
         * @brief Retrieves the current system ticks.
         *
         * This method returns the current system ticks based on the platform API used.
         * It can return the time in seconds or milliseconds depending on the method called.
         *
         * @return T The current system ticks.
         */
        static T GetSystemTicks()
        {
            auto& coreAPI = CoreAPI::GetInstance();
            switch (coreAPI.API())
            {
            case PlatformBaseAPIs::GLFW: return static_cast<T>(glfwGetTime());
            case PlatformBaseAPIs::Win32: MOTION_ASSERT(false, "Win32 is not supported yet") return static_cast<T>(0);
            }

            return static_cast<T>(0);
        }


        /**
         * @brief Retrieves the current system ticks in seconds or milliseconds.
         *
         * This method returns the current system ticks in seconds or milliseconds based on the platform API used.
         *
         * @return T The current system ticks in seconds or milliseconds.
         */
        static T GetSystemTicksSeconds()
        {
            switch (CoreAPI::GetBaseAPI()->API())
            {
            case PlatformBaseAPIs::GLFW: return static_cast<T>(glfwGetTime());
            case PlatformBaseAPIs::Win32: MOTION_ASSERT(false, "Win32 is not supported yet") return static_cast<T>(0);
            }

            return static_cast<T>(0);
        }


        /**
         * @brief Retrieves the current system ticks in milliseconds.
         *
         * This method returns the current system ticks in milliseconds based on the platform API used.
         *
         * @return T The current system ticks in milliseconds.
         */
        static T GetSystemTicksMilliseconds()
        {
            switch (CoreAPI::GetBaseAPI()->API())
            {
            case PlatformBaseAPIs::GLFW: return static_cast<T>(glfwGetTime() * 1000.0f);
            case PlatformBaseAPIs::Win32: MOTION_ASSERT(false, "Win32 is not supported yet") return static_cast<T>(0);
            }

            return static_cast<T>(0);
        }
    };
}