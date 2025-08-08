#include "CorePCH.hpp"
#include "ImguiLayer.hpp"

namespace Motion
{
    ImGuiLayer::ImGuiLayer(WindowHandle handle, ImGuiColorScheme colorScheme)
        : m_WindowHandle(handle), m_ColorScheme(colorScheme), Layer("ImGuiLayer") {
    }

    void ImGuiLayer::OnAttach()
    {
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        float defaultFontSize = 16.0f;
        std::filesystem::path defaultFontFile(std::filesystem::current_path() / "Assets/Fonts/JetBrainsMono/JetBrainsMono-Regular.ttf");
        io.Fonts->AddFontDefault();
        ImFont* defaultFont = io.Fonts->AddFontFromFileTTF(defaultFontFile.string().c_str(), 16.0f);
        io.FontDefault = defaultFont;

        float iconFontSize = defaultFontSize * 0.9f; // adjust icon scaling
        static const ImWchar icons_ranges[] = { (ImWchar)ICON_MIN_MD, (ImWchar)ICON_MAX_MD, 0 };
        ImFontConfig config;
        config.MergeMode = true;
        std::filesystem::path iconFontFile(std::filesystem::current_path() / "Assets/Fonts/MaterialIconFonts/MaterialIcons-Regular.ttf");
        io.Fonts->AddFontFromFileTTF(iconFontFile.string().c_str(), iconFontSize, &config, icons_ranges);


        (m_ColorScheme == ImGuiColorScheme::Dark) ? UserInterfaceInitializer::UseColorDark() : UserInterfaceInitializer::UseColorLight();
    }

    void ImGuiLayer::OnDetach()
    {

    }

    void ImGuiLayer::OnEvent(WindowHandle handle, IEvent& e)
    {
        if (m_AllowEvents)
        {
            ImGuiIO& io = ImGui::GetIO();
            if (e.Equals(EventCategory::Keyboard) && !io.WantCaptureKeyboard)
            {
                io.WantCaptureKeyboard = true;
            }
            if (e.Equals(EventCategory::Mouse) && !io.WantCaptureMouse)
            {
                io.WantCaptureMouse = true;
            }
        }
    }

    void ImGuiLayer::Begin()
    {
        auto& coreAPI = CoreAPI::GetInstance();
        if (coreAPI.API() & PlatformBaseAPIs::GLFW && Renderer::GetAPI() & RenderingAPI::OpenGL)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();
        }

        //[TODO] : Add support for Win32
    }

    void ImGuiLayer::End()
    {
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

    void ImGuiLayer::UseColorScheme(ImGuiColorScheme colorScheme)
    {
        (colorScheme == ImGuiColorScheme::Dark) ? UserInterfaceInitializer::UseColorDark() : UserInterfaceInitializer::UseColorLight();
        m_ColorScheme = colorScheme;
    }
}