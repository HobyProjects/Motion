#pragma once

#include "Log.hpp"

#include <imgui/imgui.h>
#include <vector>
#include <chrono>

namespace Motion
{
    class ToastManager
    {
    public:
        struct Toast
        {
            LogMessage Message;
            std::chrono::system_clock::time_point ShowTime;
            float Alpha = 1.0f;
            bool FadingOut = false;

            Toast(const LogMessage& msg)
                : Message(msg), ShowTime(std::chrono::system_clock::now())
            {}
        };

        ToastManager(float displayDuration = 5.0f, float fadeOutDuration = 0.5f)
            : m_DisplayDuration(displayDuration)
            , m_FadeOutDuration(fadeOutDuration)
        {}

        void Update()
        {
            auto unreadLogs = Loggers::GetInstance().GetUnreadLogs();
            for (const auto& log : unreadLogs)
            {
                if (log.Level >= spdlog::level::warn)
                {
                    m_ActiveToasts.emplace_back(log);
                }
            }

            if (!unreadLogs.empty())
            {
                Loggers::GetInstance().MarkLogsAsRead();
            }

            auto now = std::chrono::system_clock::now();
            for (auto it = m_ActiveToasts.begin(); it != m_ActiveToasts.end();)
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - it->ShowTime).count() / 1000.0f;

                if (elapsed > m_DisplayDuration && !it->FadingOut)
                {
                    it->FadingOut = true;
                }

                if (it->FadingOut)
                {
                    float fadeElapsed = elapsed - m_DisplayDuration;
                    it->Alpha = 1.0f - (fadeElapsed / m_FadeOutDuration);

                    if (it->Alpha <= 0.0f)
                    {
                        it = m_ActiveToasts.erase(it);
                        continue;
                    }
                }

                ++it;
            }
        }

        void Render()
        {
            if (m_ActiveToasts.empty()) return;

            ImGuiIO& io         = ImGui::GetIO();
            ImGuiStyle& style   = ImGui::GetStyle();

            float padding       = 10.0f;
            float toast_width   = 300.0f;
            float toast_height  = 60.0f;
            float spacing       = 10.0f;

            ImVec2 work_pos = ImGui::GetMainViewport()->WorkPos;
            ImVec2 work_size = ImGui::GetMainViewport()->WorkSize;

            for (size_t i = 0; i < m_ActiveToasts.size(); ++i)
            {
                auto& toast = m_ActiveToasts[i];

                ImVec2 toast_pos;
                toast_pos.x = work_pos.x + work_size.x - toast_width - padding;
                toast_pos.y = work_pos.y + work_size.y - (toast_height + spacing) * (i + 1);

                ImGui::SetNextWindowPos(toast_pos);
                ImGui::SetNextWindowSize(ImVec2(toast_width, toast_height));
                ImGui::SetNextWindowBgAlpha(0.9f * toast.Alpha);

                ImGuiWindowFlags window_flags = 
                    ImGuiWindowFlags_NoDecoration | 
                    ImGuiWindowFlags_NoMove | 
                    ImGuiWindowFlags_NoSavedSettings |
                    ImGuiWindowFlags_NoFocusOnAppearing |
                    ImGuiWindowFlags_NoNav;

                char window_name[32];
                snprintf(window_name, sizeof(window_name), "Toast##%zu", i);

                if (ImGui::Begin(window_name, nullptr, window_flags))
                {
                    ImVec4 color = GetColorForLogLevel(toast.Message.Level);
                    
                    ImDrawList* draw_list = ImGui::GetWindowDrawList();
                    ImVec2 p_min = ImGui::GetWindowPos();
                    ImVec2 p_max = ImVec2(p_min.x + 5.0f, p_min.y + toast_height);
                    draw_list->AddRectFilled(p_min, p_max, ImGui::ColorConvertFloat4ToU32(color));

                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                    ImGui::PushStyleColor(ImGuiCol_Text, color);
                    ImGui::TextUnformatted(GetLevelString(toast.Message.Level));
                    ImGui::PopStyleColor();

                    ImGui::SameLine();
                    
                    auto time = std::chrono::system_clock::to_time_t(toast.Message.Timestamp);
                    char time_str[32];
                    std::strftime(time_str, sizeof(time_str), "%H:%M:%S", std::localtime(&time));
                    ImGui::TextDisabled("%s", time_str);

                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 10.0f);
                    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + toast_width - 20.0f);
                    ImGui::TextWrapped("%s", toast.Message.Message.c_str());
                    ImGui::PopTextWrapPos();
                }
                ImGui::End();
            }
        }

        void ClearAll()
        {
            m_ActiveToasts.clear();
        }

    private:
        ImVec4 GetColorForLogLevel(spdlog::level::level_enum level)
        {
            switch (level)
            {
                case spdlog::level::trace:      return ImVec4(0.75f, 0.75f, 0.75f, 1.0f);   // Light gray
                case spdlog::level::debug:      return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);      // Light blue
                case spdlog::level::info:       return ImVec4(0.4f, 0.8f, 0.4f, 1.0f);      // Green
                case spdlog::level::warn:       return ImVec4(1.0f, 0.8f, 0.2f, 1.0f);      // Yellow/Orange
                case spdlog::level::err:        return ImVec4(1.0f, 0.4f, 0.4f, 1.0f);      // Red
                case spdlog::level::critical:   return ImVec4(0.9f, 0.1f, 0.1f, 1.0f);      // Dark red
                default:                        return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);      // White
            }
        }

        const char* GetLevelString(spdlog::level::level_enum level)
        {
            switch (level)
            {
                case spdlog::level::trace:    return "TRACE";
                case spdlog::level::debug:    return "DEBUG";
                case spdlog::level::info:     return "INFO";
                case spdlog::level::warn:     return "WARN";
                case spdlog::level::err:      return "ERROR";
                case spdlog::level::critical: return "CRITICAL";
                default:                      return "UNKNOWN";
            }
        }

    private:
        std::vector<Toast> m_ActiveToasts;
        float m_DisplayDuration;
        float m_FadeOutDuration;
    };
}