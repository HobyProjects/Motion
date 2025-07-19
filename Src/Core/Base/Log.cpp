#include "CorePCH.hpp"
#include "Log.hpp"

namespace Motion::Core
{
    /**
     * @brief Initializes the logging system for the Motion Core and Application.
     *
     * This function sets up two loggers using the spdlog library:
     * - Core Logger: Used for internal core logging.
     * - Application Logger: Used for application-level logging.
     *
     * Both loggers output to the console (with color) and to a file named "MotionLogging.log".
     * The loggers are configured with specific output patterns and set to the trace log level,
     * ensuring that all log messages (including the most detailed) are captured and flushed immediately.
     *
     * The loggers are registered globally with spdlog for later retrieval and use.
     */
    void Motion::Core::Loggers::Initialize()
    {
        std::vector<spdlog::sink_ptr> log_skin{};
        log_skin.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        log_skin.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("MotionLogging.log", true));
        log_skin[0]->set_pattern("%^[%T] %n: %v%$");
        log_skin[1]->set_pattern("[%T][%l] %n: %v");

        m_CoreLogger = std::make_shared<spdlog::logger>("[MOTION::CORE]", std::begin(log_skin), std::end(log_skin));
        spdlog::register_logger(m_CoreLogger);
        m_CoreLogger->set_level(spdlog::level::trace);
        m_CoreLogger->flush_on(spdlog::level::trace);

        m_AppLogger = std::make_shared<spdlog::logger>("[MOTION::APP]", std::begin(log_skin), std::end(log_skin));
        spdlog::register_logger(m_AppLogger);
        m_AppLogger->set_level(spdlog::level::trace);
        m_AppLogger->flush_on(spdlog::level::trace);
    }

    /**
     * @brief Retrieves the core logger instance.
     *
     * This method returns a shared pointer to the core logger used by the application.
     * The core logger is typically responsible for logging system-wide or core messages.
     *
     * @return std::shared_ptr<spdlog::logger> Shared pointer to the core logger.
     */
    std::shared_ptr<spdlog::logger> Loggers::CoreLogger() const
    {
        return m_CoreLogger;
    }

    /**
     * @brief Retrieves the application logger instance.
     *
     * This method returns a shared pointer to the application's main logger,
     * which can be used for logging messages throughout the application.
     *
     * @return std::shared_ptr<spdlog::logger> Shared pointer to the application logger.
     */
    std::shared_ptr<spdlog::logger> Loggers::AppLogger() const
    {
        return m_AppLogger;
    }
}


