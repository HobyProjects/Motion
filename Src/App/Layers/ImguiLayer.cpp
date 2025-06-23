#include "CorePCH.hpp"
#include "ImguiLayer.hpp"

namespace Motion::App
{
    ImGuiLayer::ImGuiLayer(Motion::Core::WindowHandle handle, ImGuiColorScheme colorScheme) 
        : m_WindowHandle(handle), m_ColorScheme(colorScheme), Motion::Core::Layer("ImGuiLayer") {}

    void ImGuiLayer::OnAttach()
    {
        //[TODO] : Manage imgui assets here, importing fonts, textures, etc
        (m_ColorScheme == ImGuiColorScheme::Dark ) ?  Motion::Core::UI::UseColorDark() : Motion::Core::UI::UseColorLight();
    }

    void ImGuiLayer::OnDetach() 
    {

    }

    void ImGuiLayer::OnEvent(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e) 
    {
        if( m_AllowEvents )
        {
            ImGuiIO& io = ImGui::GetIO();
            if (e.Equals(Motion::Core::EventCategory::Keyboard) && !io.WantCaptureKeyboard)
            {
                io.WantCaptureKeyboard = true;
            }
            if (e.Equals(Motion::Core::EventCategory::Mouse) && !io.WantCaptureMouse)
            {
                io.WantCaptureMouse = true;
            }
        }
    }

    void ImGuiLayer::Begin()
    {
        if(Motion::Core::CoreAPI::GetBaseAPI()->API() & Motion::Core::BaseAPIs::GLFW && 
            Motion::Core::Renderer::GetAPI() & Motion::Core::RenderingAPI::OpenGL)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        //[TODO] : Add support for Win32
    }

    void ImGuiLayer::End()
    {
        ImGuiIO& io = ImGui::GetIO();
        std::weak_ptr<Motion::Core::IWindow> window = Motion::Core::WindowManager::Get(m_WindowHandle);
        if(!window.expired())
        {
            auto windowPtr = window.lock();
            io.DisplaySize = ImVec2(static_cast<float>(windowPtr->GetProperties().Width), static_cast<float>(windowPtr->GetProperties().Height));
        }

        if(Motion::Core::CoreAPI::GetBaseAPI()->API() & Motion::Core::BaseAPIs::GLFW && 
            Motion::Core::Renderer::GetAPI() & Motion::Core::RenderingAPI::OpenGL)
        {
            ImGui::EndFrame();
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            if( io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable )
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
        ( colorScheme == ImGuiColorScheme::Dark ) ? Motion::Core::UI::UseColorDark() : Motion::Core::UI::UseColorLight();
        m_ColorScheme = colorScheme;
    }
}