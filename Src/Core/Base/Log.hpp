#pragma once 

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/base_sink.h>

#include <deque>
#include <mutex>
#include <chrono>

#include "Base.hpp"
#include "Controls.hpp"

namespace Motion
{
    struct LogMessage
    {
        spdlog::level::level_enum Level;
        std::string Message;
        std::chrono::system_clock::time_point Timestamp;
        bool Displayed = false; // For toast notification tracking

        LogMessage(spdlog::level::level_enum level, std::string msg)
            : Level(level), Message(std::move(msg)), Timestamp(std::chrono::system_clock::now())
        {}
    };

    template<typename Mutex>
    class ImGuiSink : public spdlog::sinks::base_sink<Mutex>
    {
        public:
            explicit ImGuiSink(size_t max_size = 1000)
                : m_MaxSize(max_size)
            {}

            std::deque<LogMessage> GetMessages()
            {
                std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
                return m_Messages;
            }

            std::vector<LogMessage> GetRecentMessages(size_t count)
            {
                std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
                std::vector<LogMessage> recent;
                
                size_t start = m_Messages.size() > count ? m_Messages.size() - count : 0;
                for (size_t i = start; i < m_Messages.size(); ++i)
                {
                    recent.push_back(m_Messages[i]);
                }
                
                return recent;
            }

            std::vector<LogMessage> GetUnreadMessages()
            {
                std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
                std::vector<LogMessage> unread;
                
                for (auto& msg : m_Messages)
                {
                    if (!msg.Displayed)
                    {
                        unread.push_back(msg);
                    }
                }
                
                return unread;
            }

            void MarkAllAsDisplayed()
            {
                std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
                for (auto& msg : m_Messages)
                {
                    msg.Displayed = true;
                }
            }

            void ClearMessages()
            {
                std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
                m_Messages.clear();
            }

            size_t GetMessageCount() const
            {
                std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
                return m_Messages.size();
            }

        protected:
            void sink_it_(const spdlog::details::log_msg& msg) override
            {
                spdlog::memory_buf_t formatted;
                spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
                
                std::string message = fmt::to_string(formatted);
                
                // Add to queue
                m_Messages.emplace_back(msg.level, message);
                
                // Maintain max size
                if (m_Messages.size() > m_MaxSize)
                {
                    m_Messages.pop_front();
                }
            }

            void flush_() override {}

        private:
            std::deque<LogMessage> m_Messages;
            size_t m_MaxSize;
    };

    using ImGuiSink_mt = ImGuiSink<std::mutex>;
    using ImGuiSink_st = ImGuiSink<spdlog::details::null_mutex>;

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
        [[nodiscard]] std::shared_ptr<ImGuiSink_mt> GetImGuiSink() const { return m_ImGuiSink; }
        
        std::vector<LogMessage> GetRecentLogs(size_t count = 100) const
        {
            if (m_ImGuiSink)
                return m_ImGuiSink->GetRecentMessages(count);
            return {};
        }

        std::vector<LogMessage> GetUnreadLogs() const
        {
            if (m_ImGuiSink)
                return m_ImGuiSink->GetUnreadMessages();
            return {};
        }

        void MarkLogsAsRead() const
        {
            if (m_ImGuiSink)
                m_ImGuiSink->MarkAllAsDisplayed();
        }

        void ClearLogQueue() const
        {
            if (m_ImGuiSink)
                m_ImGuiSink->ClearMessages();
        }

    private:
        std::shared_ptr<spdlog::logger> m_CoreLogger{ nullptr };
        std::shared_ptr<spdlog::logger> m_AppLogger{ nullptr };
        std::shared_ptr<ImGuiSink_mt> m_ImGuiSink{ nullptr };
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