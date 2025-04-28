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
            static std::shared_ptr<spdlog::logger>& CoreLogger();
            static std::shared_ptr<spdlog::logger>& AppLogger();
    };
}

#define MOTION_CORE_INFO(...) ::Motion::Core::Loggers::CoreLogger()->info(__VA_ARGS__)
#define MOTION_CORE_WARN(...) ::Motion::Core::Loggers::CoreLogger()->warn(__VA_ARGS__)
#define MOTION_CORE_ERROR(...) ::Motion::Core::Loggers::CoreLogger()->error(__VA_ARGS__)
#define MOTION_CORE_CRITICAL(...) ::Motion::Core::Loggers::CoreLogger()->critical(__VA_ARGS__)

#define MOTION_INFO(...) ::Motion::Core::Loggers::AppLogger()->info(__VA_ARGS__)
#define MOTION_WARN(...) ::Motion::Core::Loggers::AppLogger()->warn(__VA_ARGS__)
#define MOTION_ERROR(...) ::Motion::Core::Loggers::AppLogger()->error(__VA_ARGS__)
#define MOTION_CRITICAL(...) ::Motion::Core::Loggers::AppLogger()->critical(__VA_ARGS__)

