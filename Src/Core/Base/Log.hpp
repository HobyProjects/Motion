#pragma once 

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/base_sink.h>

#include <deque>
#include <mutex>
#include <chrono>

#include "Base.hpp"
#include "Notific.hpp"

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
        
        // Control whether logs also create notifications
        static void EnableNotifications(bool enable) noexcept { s_NotificationsEnabled = enable; }
        static bool AreNotificationsEnabled() noexcept { return s_NotificationsEnabled; }
        
    private:
        std::shared_ptr<spdlog::logger> m_CoreLogger{ nullptr };
        std::shared_ptr<spdlog::logger> m_AppLogger{ nullptr };
        std::once_flag m_InitializeLoggers{};
        
        static inline bool s_NotificationsEnabled = true;
    };
}

// Helper macro for creating notifications from log messages
#define MOTION_LOG_WITH_NOTIFICATION(logger_call, notify_call, title, ...) \
    do { \
        logger_call(__VA_ARGS__); \
        if (Motion::Loggers::AreNotificationsEnabled()) { \
            notify_call(title, fmt::format(__VA_ARGS__)); \
        } \
    } while(0)

// Core logging macros
#define MOTION_CORE_INFO(...) Motion::Loggers::GetInstance().CoreLogger()->info(__VA_ARGS__)
#define MOTION_CORE_TRACE(...) Motion::Loggers::GetInstance().CoreLogger()->trace(__VA_ARGS__)

#define MOTION_CORE_WARN(...) \
    MOTION_LOG_WITH_NOTIFICATION( \
        Motion::Loggers::GetInstance().CoreLogger()->warn, \
        Motion::Notific::PushWarn, \
        "Core Warning", \
        __VA_ARGS__)

#define MOTION_CORE_ERROR(...) \
    MOTION_LOG_WITH_NOTIFICATION( \
        Motion::Loggers::GetInstance().CoreLogger()->error, \
        Motion::Notific::PushError, \
        "Core Error", \
        __VA_ARGS__)

#define MOTION_CORE_CRITICAL(...) \
    MOTION_LOG_WITH_NOTIFICATION( \
        Motion::Loggers::GetInstance().CoreLogger()->critical, \
        Motion::Notific::PushCritical, \
        "Core Critical", \
        __VA_ARGS__)

// App logging macros
#define MOTION_INFO(...) Motion::Loggers::GetInstance().AppLogger()->info(__VA_ARGS__)
#define MOTION_TRACE(...) Motion::Loggers::GetInstance().AppLogger()->trace(__VA_ARGS__)

#define MOTION_WARN(...) \
    MOTION_LOG_WITH_NOTIFICATION( \
        Motion::Loggers::GetInstance().AppLogger()->warn, \
        Motion::Notific::PushWarn, \
        "Warning", \
        __VA_ARGS__)

#define MOTION_ERROR(...) \
    MOTION_LOG_WITH_NOTIFICATION( \
        Motion::Loggers::GetInstance().AppLogger()->error, \
        Motion::Notific::PushError, \
        "Error", \
        __VA_ARGS__)

#define MOTION_CRITICAL(...) \
    MOTION_LOG_WITH_NOTIFICATION( \
        Motion::Loggers::GetInstance().AppLogger()->critical, \
        Motion::Notific::PushCritical, \
        "Critical", \
        __VA_ARGS__)