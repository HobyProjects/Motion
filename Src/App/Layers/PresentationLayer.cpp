#include "CorePCH.hpp"
#include "PresentationLayer.hpp"

namespace Motion
{
    PresentationLayer::PresentationLayer(WindowHandle handle) : m_WindowHandle(handle), Layer("ImGuiLayer") {}

    void PresentationLayer::OnAttach()
    {
        UserInterface::Init(m_WindowHandle);
        UserInterface::LoadDefaultFonts("Assets/Fonts/JetBrainsMono/JetBrainsMono-Regular.ttf", 17.5f);
        UserInterface::UseColor();

#ifdef MOTION_PLATFORM_WINDOWS
        UserInterface::EnableMicaEffect(m_WindowHandle);
#endif

    }

    void PresentationLayer::OnDetach()
    {
        UserInterface::Quit();
    }

    void PresentationLayer::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
;
    }

    void PresentationLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        if (m_AllowEvents)
        {
            ImGuiIO& io = ImGui::GetIO();
            if (e.Equals(EventCategory::Keyboard) & !io.WantCaptureKeyboard) { io.WantCaptureKeyboard = true; }
            if (e.Equals(EventCategory::Mouse) & !io.WantCaptureMouse) { io.WantCaptureMouse = true; }
        }
    }

    void PresentationLayer::Begin()
    {
        auto& coreAPI = CoreAPI::GetInstance();
        if (coreAPI.API() & PlatformBaseAPIs::GLFW && Renderer::GetAPI() & RenderingAPI::OpenGL)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();
        }
    }

    void PresentationLayer::End()
    {
        //RenderLogConsoleWindow();

        ImGuiIO& io = ImGui::GetIO();
        auto& coreAPI = CoreAPI::GetInstance();
        auto& windowManager = WindowManager::GetInstance();

        std::weak_ptr<IWindow> window = windowManager.GetWindow(m_WindowHandle);
        if (!window.expired())
        {
            auto windowPtr = window.lock();
            io.DisplaySize = ImVec2(static_cast<float>(windowPtr->GetProperties().Width), static_cast<float>(windowPtr->GetProperties().Height));
        }

        if (coreAPI.API() & PlatformBaseAPIs::GLFW && Renderer::GetAPI() & RenderingAPI::OpenGL)
        {
            ImGui::EndFrame();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                GLFWwindow* backup_current_context = glfwGetCurrentContext();
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
                glfwMakeContextCurrent(backup_current_context);
            }
        }
    }

    void PresentationLayer::RenderLogConsoleWindow()
    {
        ImGui::SetNextWindowSize(ImVec2(800, 400), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Console", &m_ShowLogConsole))
        {
            static bool show_trace = true;
            static bool show_debug = true;
            static bool show_info = true;
            static bool show_warn = true;
            static bool show_error = true;
            static bool show_critical = true;

            ImGui::Checkbox("Trace", &show_trace); ImGui::SameLine();
            ImGui::Checkbox("Debug", &show_debug); ImGui::SameLine();
            ImGui::Checkbox("Info", &show_info); ImGui::SameLine();
            ImGui::Checkbox("Warn", &show_warn); ImGui::SameLine();
            ImGui::Checkbox("Error", &show_error); ImGui::SameLine();
            ImGui::Checkbox("Critical", &show_critical);

            ImGui::Separator();

            if (ImGui::Button("Clear"))
            {
                Loggers::GetInstance().ClearLogQueue();
            }

            ImGui::Separator();
            ImGui::BeginChild("LogScrollRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
        
            auto logs = Loggers::GetInstance().GetRecentLogs(1000);
            
            for (const auto& log : logs)
            {
                bool should_show = false;
                switch (log.Level)
                {
                    case spdlog::level::trace:    should_show = show_trace; break;
                    case spdlog::level::debug:    should_show = show_debug; break;
                    case spdlog::level::info:     should_show = show_info; break;
                    case spdlog::level::warn:     should_show = show_warn; break;
                    case spdlog::level::err:      should_show = show_error; break;
                    case spdlog::level::critical: should_show = show_critical; break;
                }

                if (!should_show)
                    continue;

                ImVec4 color;
                const char* level_str;
                switch (log.Level)
                {
                    case spdlog::level::trace:
                        color = ImVec4(0.75f, 0.75f, 0.75f, 1.0f);
                        level_str = "[TRACE]";
                        break;
                    case spdlog::level::debug:
                        color = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
                        level_str = "[DEBUG]";
                        break;
                    case spdlog::level::info:
                        color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f);
                        level_str = "[INFO]";
                        break;
                    case spdlog::level::warn:
                        color = ImVec4(1.0f, 0.8f, 0.2f, 1.0f);
                        level_str = "[WARN]";
                        break;
                    case spdlog::level::err:
                        color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                        level_str = "[ERROR]";
                        break;
                    case spdlog::level::critical:
                        color = ImVec4(0.9f, 0.1f, 0.1f, 1.0f);
                        level_str = "[CRITICAL]";
                        break;
                    default:
                        color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                        level_str = "[UNKNOWN]";
                        break;
                }

                auto time = std::chrono::system_clock::to_time_t(log.Timestamp);
                char time_str[32];
                std::strftime(time_str, sizeof(time_str), "%H:%M:%S", std::localtime(&time));
                
                ImGui::TextDisabled("%s", time_str);
                ImGui::SameLine();
                
                ImGui::PushStyleColor(ImGuiCol_Text, color);
                ImGui::TextUnformatted(level_str);
                ImGui::PopStyleColor();
                ImGui::SameLine();
                
                ImGui::TextWrapped("%s", log.Message.c_str());
            }

            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);

            ImGui::EndChild();
        }
        ImGui::End();
    }
}