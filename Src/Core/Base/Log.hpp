#pragma once 

#include "Base.hpp"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace Motion::Core
{
    class Loggers
    {
    private:
        Loggers() = default;
        ~Loggers() = default;

        Loggers(const Loggers&) = delete;
        Loggers& operator=(const Loggers&) = delete;
        Loggers(const Loggers&&) = delete;
        Loggers&& operator=(const Loggers&&) = delete;

    public:
        /**
         * @brief Returns the singleton instance of Loggers.
         *
         * This method ensures that only one instance of Loggers exists throughout the application.
         * It initializes the instance if it does not already exist.
         *
         * @return Reference to the singleton Loggers instance.
         */
        static Loggers& GetInstance() noexcept
        {
            static Loggers instance;
            return instance;
        }

    public:
        void Initialize();

        [[nodiscard]] std::shared_ptr<spdlog::logger> CoreLogger() const;
        [[nodiscard]] std::shared_ptr<spdlog::logger> AppLogger() const;

    private:
        std::shared_ptr<spdlog::logger> m_CoreLogger{ nullptr };
        std::shared_ptr<spdlog::logger> m_AppLogger{ nullptr };
        std::once_flag m_InitializeLoggers{};
    };
}

#define MOTION_CORE_INFO(...) ::Motion::Core::Loggers::GetInstance().CoreLogger()->info(__VA_ARGS__)
#define MOTION_CORE_WARN(...) ::Motion::Core::Loggers::GetInstance().CoreLogger()->warn(__VA_ARGS__)
#define MOTION_CORE_ERROR(...) ::Motion::Core::Loggers::GetInstance().CoreLogger()->error(__VA_ARGS__)
#define MOTION_CORE_CRITICAL(...) ::Motion::Core::Loggers::GetInstance().CoreLogger()->critical(__VA_ARGS__)

#define MOTION_INFO(...) ::Motion::Core::Loggers::GetInstance().AppLogger()->info(__VA_ARGS__)
#define MOTION_WARN(...) ::Motion::Core::Loggers::GetInstance().AppLogger()->warn(__VA_ARGS__)
#define MOTION_ERROR(...) ::Motion::Core::Loggers::GetInstance().AppLogger()->error(__VA_ARGS__)
#define MOTION_CRITICAL(...) ::Motion::Core::Loggers::GetInstance().AppLogger()->critical(__VA_ARGS__)

