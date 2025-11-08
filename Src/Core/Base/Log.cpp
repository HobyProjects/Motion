#include "CorePCH.hpp"
#include "Log.hpp"

namespace Motion
{
    void Loggers::Initialize()
    {
        std::vector<spdlog::sink_ptr> log_skin{};
        
        log_skin.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        log_skin.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("MotionLogging.log", true));
        
        m_ImGuiSink = std::make_shared<ImGuiSink_mt>(1000); 
        log_skin.emplace_back(m_ImGuiSink);
        
        log_skin[0]->set_pattern("%^[%T] %n: %v%$");
        log_skin[1]->set_pattern("[%T][%l] %n: %v");
        log_skin[2]->set_pattern("%v"); 
        
        m_CoreLogger = std::make_shared<spdlog::logger>("[CORE]", std::begin(log_skin), std::end(log_skin));
        spdlog::register_logger(m_CoreLogger);
        m_CoreLogger->set_level(spdlog::level::trace);
        m_CoreLogger->flush_on(spdlog::level::trace);

        m_AppLogger = std::make_shared<spdlog::logger>("[APP]", std::begin(log_skin), std::end(log_skin));
        spdlog::register_logger(m_AppLogger);
        m_AppLogger->set_level(spdlog::level::trace);
        m_AppLogger->flush_on(spdlog::level::trace);
    }

    std::shared_ptr<spdlog::logger> Loggers::CoreLogger() const
    {
        return m_CoreLogger;
    }

    std::shared_ptr<spdlog::logger> Loggers::AppLogger() const
    {
        return m_AppLogger;
    }
}