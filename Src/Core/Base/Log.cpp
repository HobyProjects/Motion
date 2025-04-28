#include "CorePCH.hpp"

namespace Motion::Core
{
    static std::shared_ptr<spdlog::logger> s_CoreLogger{nullptr};
    static std::shared_ptr<spdlog::logger> s_AppLogger{nullptr};
    static std::once_flag s_InitializeLoggers{};

    static void InitLoggers()
    {
        std::vector<spdlog::sink_ptr> log_skin{};
		log_skin.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
		log_skin.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("trimana.log", true));
		log_skin [0]->set_pattern("%^[%T] %n: %v%$");
		log_skin [1]->set_pattern("[%T][%l] %n: %v");

		s_CoreLogger = std::make_shared<spdlog::logger>("Trimana::core", std::begin(log_skin), std::end(log_skin));
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

		s_AppLogger = std::make_shared<spdlog::logger>("Trimana::app", std::begin(log_skin), std::end(log_skin));
		spdlog::register_logger(s_AppLogger);
		s_AppLogger->set_level(spdlog::level::trace);
		s_AppLogger->flush_on(spdlog::level::trace);
    }

    std::shared_ptr<spdlog::logger>& Loggers::CoreLogger()
    {
        std::call_once(s_InitializeLoggers, InitLoggers);
        return s_CoreLogger;
    }

    std::shared_ptr<spdlog::logger>& Loggers::AppLogger()
    {
        std::call_once(s_InitializeLoggers, InitLoggers);
        return s_AppLogger;
    }
}
