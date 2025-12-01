#pragma once

#include <string>
#include <vector>
#include <chrono>

#include <imgui/imgui.h>

namespace Motion
{
    enum class NotificationLevel
    {
        Info,
        Warn,
        Error,
        Critical
    };

    struct Notification
    {
        std::string Title;
        std::string Message;
        NotificationLevel Level;
        std::chrono::steady_clock::time_point CreationTime;
        float DisplayDuration; // in seconds
        bool IsVisible;
        float AnimationProgress; // 0.0 to 1.0 for slide-in/fade effects

        Notification(const std::string& title, const std::string& message, NotificationLevel level, float duration = 5.0f)
            : Title(title)
            , Message(message)
            , Level(level)
            , CreationTime(std::chrono::steady_clock::now())
            , DisplayDuration(duration)
            , IsVisible(true)
            , AnimationProgress(0.0f)
        {}
    };

    class Notific
    {
    public:
        static void PushInfo(const std::string& title, const std::string& message, float duration = 5.0f) noexcept;
        static void PushWarn(const std::string& title, const std::string& message, float duration = 5.0f) noexcept;
        static void PushError(const std::string& title, const std::string& message, float duration = 5.0f) noexcept;
        static void PushCritical(const std::string& title, const std::string& message, float duration = 8.0f) noexcept;
        
        static void Push(const std::string& title, const std::string& message, NotificationLevel level, float duration = 5.0f) noexcept;
        
        static void Render() noexcept;
        
        static void Clear() noexcept;
        static void ClearLevel(NotificationLevel level) noexcept;
        
        static size_t GetCount() noexcept;
        static size_t GetCount(NotificationLevel level) noexcept;
        
        static void SetMaxNotifications(size_t max) noexcept;
        static void SetDefaultDuration(float duration) noexcept;

    private:
        static void RenderNotification(Notification& notification, float yOffset) noexcept;
        static ImVec4 GetLevelColor(NotificationLevel level) noexcept;
        static const char* GetLevelIcon(NotificationLevel level) noexcept;
        static const char* GetLevelText(NotificationLevel level) noexcept;
        static void UpdateAnimations(float deltaTime) noexcept;

    private:
        static inline std::vector<Notification> s_Notifications;
        static inline size_t s_MaxNotifications = 5;
        static inline float s_DefaultDuration = 5.0f;
        static inline float s_NotificationWidth = 400.0f;
        static inline float s_NotificationSpacing = 10.0f;
        static inline float s_AnimationSpeed = 5.0f;
    };
}

// Convenience macros for notifications
#define MOTION_NOTIFY_INFO(title, message) Motion::Notific::PushInfo(title, message)
#define MOTION_NOTIFY_WARN(title, message) Motion::Notific::PushWarn(title, message)
#define MOTION_NOTIFY_ERROR(title, message) Motion::Notific::PushError(title, message)
#define MOTION_NOTIFY_CRITICAL(title, message) Motion::Notific::PushCritical(title, message)