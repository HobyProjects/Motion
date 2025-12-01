#include "CorePCH.hpp"
#include "PresentationLayer.hpp"

namespace Motion
{
    PresentationLayer::PresentationLayer(WindowHandle handle) : m_WindowHandle(handle), Layer("ImGuiLayer") {}

    void PresentationLayer::OnAttach()
    {
        UserInterface::Init(m_WindowHandle);
        UserInterface::FontManager::MaterialIconsPath   = "Assets/Fonts/IconFonts/MaterialIcons-Regular.ttf";
        UserInterface::FontManager::FontAwesomePath     = "Assets/Fonts/IconFonts/fa-regular-400.ttf";
        UserInterface::FontManager::BeginFontLoading();
        
        UserInterface::FontManager::LoadFontWithIcons(
            "JetBrainsMono-Regular", 
            "Assets/Fonts/JetBrainsMono/JetBrainsMono-Regular.ttf", 
            21.0f,
            true,  
            true  
        );
        
        UserInterface::FontManager::LoadFontWithIcons(
            "JetBrainsMono-Bold-H1", 
            "Assets/Fonts/JetBrainsMono/JetBrainsMono-Bold.ttf", 
            32.0f,
            true, true
        );
        
        UserInterface::FontManager::LoadFontWithIcons(
            "JetBrainsMono-Bold-H2", 
            "Assets/Fonts/JetBrainsMono/JetBrainsMono-Bold.ttf", 
            28.5f,
            true, true
        );
        
        UserInterface::FontManager::LoadFontWithIcons(
            "JetBrainsMono-Bold-H3", 
            "Assets/Fonts/JetBrainsMono/JetBrainsMono-Bold.ttf", 
            24.0f,
            true, true
        );
        
        UserInterface::FontManager::LoadFontWithIcons(
            "JetBrainsMono-Bold-H4", 
            "Assets/Fonts/JetBrainsMono/JetBrainsMono-Bold.ttf", 
            20.5f,
            true, true
        );
        
        UserInterface::FontManager::LoadFontWithIcons(
            "JetBrainsMono-Bold-H5", 
            "Assets/Fonts/JetBrainsMono/JetBrainsMono-Bold.ttf", 
            18.5f,
            true, true
        );
        
        UserInterface::FontManager::LoadFontWithIcons(
            "JetBrainsMono-Bold-H6", 
            "Assets/Fonts/JetBrainsMono/JetBrainsMono-Bold.ttf", 
            16.5f,
            true, true
        );
        
        UserInterface::FontManager::Build();
        UserInterface::FontManager::SetAsDefaultFont("JetBrainsMono-Regular");
        UserInterface::ThemeManager::UseColorScheme(UserInterface::ThemeManager::GetMaterialDesignScheme());
        UserInterface::ThemeManager::SetAccentColor(Colors::MaterialGreen500);

#ifdef MOTION_PLATFORM_WINDOWS
        UserInterface::EnableMicaEffect(m_WindowHandle);
#endif
    }

    void PresentationLayer::OnDetach()
    {
        UserInterface::Quit();
    }

    void PresentationLayer::OnEvent([[maybe_unused]] WindowHandle handle, IEvent& e)
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
            // Render notifications before ending the frame
            Notific::Render();
            
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
}