#pragma once 

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/base_sink.h>

#include <deque>
#include <mutex>
#include <chrono>

#include "Base.hpp"

namespace Motion
{
    class Loggers
    {
    private:
        Loggers() = default;
        ~Loggers() = default;

        Loggers(const Loggers&) = delete;
        Loggers& operator=(const Loggers&) = delete;
        Loggers(Loggers&&) = delete;
        Loggers& operator=(Loggers&&) = delete;

    public:
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

#define MOTION_CORE_INFO(...) Motion::Loggers::GetInstance().CoreLogger()->info(__VA_ARGS__)
#define MOTION_CORE_WARN(...) Motion::Loggers::GetInstance().CoreLogger()->warn(__VA_ARGS__)
#define MOTION_CORE_ERROR(...) Motion::Loggers::GetInstance().CoreLogger()->error(__VA_ARGS__)
#define MOTION_CORE_TRACE(...) Motion::Loggers::GetInstance().CoreLogger()->trace(__VA_ARGS__)
#define MOTION_CORE_CRITICAL(...) Motion::Loggers::GetInstance().CoreLogger()->critical(__VA_ARGS__)

#define MOTION_INFO(...) Motion::Loggers::GetInstance().AppLogger()->info(__VA_ARGS__)
#define MOTION_WARN(...) Motion::Loggers::GetInstance().AppLogger()->warn(__VA_ARGS__)
#define MOTION_ERROR(...) Motion::Loggers::GetInstance().AppLogger()->error(__VA_ARGS__)
#define MOTION_TRACE(...) Motion::Loggers::GetInstance().AppLogger()->trace(__VA_ARGS__)
#define MOTION_CRITICAL(...) Motion::Loggers::GetInstance().AppLogger()->critical(__VA_ARGS__)