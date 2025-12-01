#include "CorePCH.hpp"

namespace Motion
{
    void Notific::PushInfo(const std::string& title, const std::string& message, float duration) noexcept
    {
        Push(title, message, NotificationLevel::Info, duration);
    }

    void Notific::PushWarn(const std::string& title, const std::string& message, float duration) noexcept
    {
        Push(title, message, NotificationLevel::Warn, duration);
    }

    void Notific::PushError(const std::string& title, const std::string& message, float duration) noexcept
    {
        Push(title, message, NotificationLevel::Error, duration);
    }

    void Notific::PushCritical(const std::string& title, const std::string& message, float duration) noexcept
    {
        Push(title, message, NotificationLevel::Critical, duration);
    }

    void Notific::Push(const std::string& title, const std::string& message, NotificationLevel level, float duration) noexcept
    {
        // Remove oldest notification if we've hit the limit
        if (s_Notifications.size() >= s_MaxNotifications)
        {
            s_Notifications.erase(s_Notifications.begin());
        }

        s_Notifications.emplace_back(title, message, level, duration);
    }

    void Notific::Render() noexcept
    {
        if (s_Notifications.empty())
            return;

        ImGuiIO& io = ImGui::GetIO();
        float deltaTime = io.DeltaTime;

        UpdateAnimations(deltaTime);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 viewportCenter = viewport->GetCenter();
        
        float startY = viewport->Pos.y + 20.0f; // 20px from top
        float currentY = startY;

        // Render notifications from oldest to newest (bottom to top visually)
        auto it = s_Notifications.begin();
        while (it != s_Notifications.end())
        {
            auto now = std::chrono::steady_clock::now();
            float elapsed = std::chrono::duration<float>(now - it->CreationTime).count();

            // Remove expired notifications
            if (elapsed > it->DisplayDuration)
            {
                it = s_Notifications.erase(it);
                continue;
            }

            if (it->IsVisible)
            {
                RenderNotification(*it, currentY);
                
                // Estimate notification height for spacing
                float notificationHeight = 80.0f; // Base height, will adjust based on content
                currentY += notificationHeight + s_NotificationSpacing;
            }

            ++it;
        }
    }

    void Notific::RenderNotification(Notification& notification, float yOffset) noexcept
    {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 viewportCenter = viewport->GetCenter();

        // Calculate position (centered horizontally)
        float posX = viewportCenter.x - (s_NotificationWidth * 0.5f);
        float posY = yOffset;

        // Apply slide-in animation
        float slideOffset = (1.0f - notification.AnimationProgress) * 50.0f;
        posY -= slideOffset;

        ImGui::SetNextWindowPos(ImVec2(posX, posY), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(s_NotificationWidth, 0), ImGuiCond_Always);

        // Style the notification window
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        
        // Set window background with alpha based on animation
        ImVec4 levelColor = GetLevelColor(notification.Level);
        ImVec4 bgColor = ImVec4(levelColor.x * 0.15f, levelColor.y * 0.15f, levelColor.z * 0.15f, 0.95f * notification.AnimationProgress);
        
        ImGui::PushStyleColor(ImGuiCol_WindowBg, bgColor);
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(levelColor.x, levelColor.y, levelColor.z, 0.3f * notification.AnimationProgress));

        // Create unique window name
        char windowName[256];
        snprintf(windowName, sizeof(windowName), "##Notification_%p", &notification);

        ImGui::Begin(windowName, nullptr, 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoSavedSettings | 
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNav |
            ImGuiWindowFlags_AlwaysAutoResize);

        // Calculate fade for expiring notifications
        auto now = std::chrono::steady_clock::now();
        float elapsed = std::chrono::duration<float>(now - notification.CreationTime).count();
        float remainingTime = notification.DisplayDuration - elapsed;
        float fadeAlpha = 1.0f;
        if (remainingTime < 0.5f)
        {
            fadeAlpha = remainingTime / 0.5f;
        }
        fadeAlpha *= notification.AnimationProgress;

        // Title with icon and level text
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(levelColor.x, levelColor.y, levelColor.z, fadeAlpha));
        
        ImGui::Text("%s %s", GetLevelIcon(notification.Level), GetLevelText(notification.Level));
        ImGui::SameLine();
        
        ImGui::PopStyleColor();

        // Title
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, fadeAlpha));
        ImGui::TextWrapped("%s", notification.Title.c_str());
        ImGui::PopStyleColor();

        ImGui::Spacing();

        // Message with wrapping
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, fadeAlpha));
        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + s_NotificationWidth - 40.0f);
        ImGui::TextWrapped("%s", notification.Message.c_str());
        ImGui::PopTextWrapPos();
        ImGui::PopStyleColor();

        // Progress bar showing remaining time
        float progress = 1.0f - (elapsed / notification.DisplayDuration);
        ImGui::Spacing();
        
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(levelColor.x, levelColor.y, levelColor.z, 0.6f * fadeAlpha));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.2f, 0.2f, 0.3f * fadeAlpha));
        ImGui::ProgressBar(progress, ImVec2(-1, 2.0f), "");
        ImGui::PopStyleColor(2);

        ImGui::End();

        ImGui::PopStyleColor(2); // WindowBg, Border
        ImGui::PopStyleVar(3);   // Rounding, Padding, BorderSize
    }

    void Notific::UpdateAnimations(float deltaTime) noexcept
    {
        for (auto& notification : s_Notifications)
        {
            if (notification.AnimationProgress < 1.0f)
            {
                notification.AnimationProgress += deltaTime * s_AnimationSpeed;
                if (notification.AnimationProgress > 1.0f)
                    notification.AnimationProgress = 1.0f;
            }
        }
    }

    ImVec4 Notific::GetLevelColor(NotificationLevel level) noexcept
    {
        switch (level)
        {
            case NotificationLevel::Info:
                return ImVec4(0.3f, 0.7f, 1.0f, 1.0f);  // Blue
            case NotificationLevel::Warn:
                return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);  // Yellow/Orange
            case NotificationLevel::Error:
                return ImVec4(1.0f, 0.3f, 0.3f, 1.0f);  // Red
            case NotificationLevel::Critical:
                return ImVec4(0.9f, 0.1f, 0.1f, 1.0f);  // Dark Red
            default:
                return ImVec4(0.7f, 0.7f, 0.7f, 1.0f);  // Gray
        }
    }

    const char* Notific::GetLevelIcon(NotificationLevel level) noexcept
    {
        switch (level)
        {
            case NotificationLevel::Info:
                return "ℹ";  // Info icon
            case NotificationLevel::Warn:
                return "⚠";  // Warning icon
            case NotificationLevel::Error:
                return "✖";  // Error icon
            case NotificationLevel::Critical:
                return "🔥"; // Critical icon
            default:
                return "•";
        }
    }

    const char* Notific::GetLevelText(NotificationLevel level) noexcept
    {
        switch (level)
        {
            case NotificationLevel::Info:
                return "INFO";
            case NotificationLevel::Warn:
                return "WARNING";
            case NotificationLevel::Error:
                return "ERROR";
            case NotificationLevel::Critical:
                return "CRITICAL";
            default:
                return "NOTIFICATION";
        }
    }

    void Notific::Clear() noexcept
    {
        s_Notifications.clear();
    }

    void Notific::ClearLevel(NotificationLevel level) noexcept
    {
        s_Notifications.erase(
            std::remove_if(s_Notifications.begin(), s_Notifications.end(),
                [level](const Notification& n) { return n.Level == level; }),
            s_Notifications.end()
        );
    }

    size_t Notific::GetCount() noexcept
    {
        return s_Notifications.size();
    }

    size_t Notific::GetCount(NotificationLevel level) noexcept
    {
        return std::count_if(s_Notifications.begin(), s_Notifications.end(),
            [level](const Notification& n) { return n.Level == level; });
    }

    void Notific::SetMaxNotifications(size_t max) noexcept
    {
        s_MaxNotifications = max;
    }

    void Notific::SetDefaultDuration(float duration) noexcept
    {
        s_DefaultDuration = duration;
    }
}