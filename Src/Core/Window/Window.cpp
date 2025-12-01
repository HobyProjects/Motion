#include "CorePCH.hpp"

namespace Motion
{
    static PlatformBaseAPIs s_PlatformBaseAPI = PlatformBaseAPIs::GLFW;  

    bool CoreAPI::Init() noexcept
    {
        switch (s_PlatformBaseAPI)
        {
            case PlatformBaseAPIs::GLFW:    
                m_PlatformBaseAPIService = std::make_shared<GLFW_BaseAPI>(); 
                break;
            case PlatformBaseAPIs::Win32:   
                MOTION_ASSERT(false, "Win32 is not supported yet");
                break;
            default:                        
                MOTION_ASSERT(false, "Unknown Base API"); 
                break;
        };

        return m_PlatformBaseAPIService->Init();
    }

    void CoreAPI::Quit() noexcept
    {
        MOTION_CORE_INFO("Shutting down Core API...");
        if (m_PlatformBaseAPIService) m_PlatformBaseAPIService->Quit();
    }

    PlatformBaseAPIs CoreAPI::API() const noexcept
    {
        return s_PlatformBaseAPI;
    }

    std::shared_ptr<IPlatformBaseAPI> CoreAPI::GetBaseAPI() const noexcept
    {
        return m_PlatformBaseAPIService;
    }

    std::shared_ptr<IContext> IContext::GetContext()
    {
        switch(s_PlatformBaseAPI)
        {
            case PlatformBaseAPIs::GLFW:    
                return std::make_shared<GLFW_GL_Context>();
            case PlatformBaseAPIs::Win32:   
                MOTION_ASSERT(false, "Win32 is not supported yet");
                return nullptr;
            default:                        
                MOTION_ASSERT(false, "Unknown Base API"); 
                return nullptr;
        }
    }

    std::shared_ptr<IWindow> WindowManager::Create(const std::string& title, bool isVisible, NativeWindow sharedWindow) noexcept
    {
        WindowHandle uniqueHandle = UniqueIdentity::GetUniqueID();
        switch (s_PlatformBaseAPI)
        {
            case PlatformBaseAPIs::GLFW:
            {
                auto window = std::make_shared<GLFW_Window>(uniqueHandle, title, isVisible, sharedWindow);
                m_WindowManagementService[uniqueHandle] = window;

                MOTION_CORE_INFO("GLFW Window created with handle {:X}", uniqueHandle);
                return window;
            }
            case PlatformBaseAPIs::Win32:
            {
                MOTION_ASSERT(false, "Win32 is not supported yet");
                return nullptr;
            }
            default:
            {
                MOTION_ASSERT(false, "Unknown Base API");
                return nullptr;
            }
        }
    }

    void WindowManager::Destroy(WindowHandle windowHandle) noexcept
    {
        if (m_WindowManagementService.contains(windowHandle))
        {
            auto window = m_WindowManagementService[windowHandle];
            if (window)
            {
                m_WindowManagementService.erase(windowHandle);
                MOTION_CORE_INFO("Window with handle {:X} destroyed", windowHandle);
            }
        }
        else
        {
            MOTION_CORE_ERROR("Window with handle {:X} not found", windowHandle);
        }
    }

    std::shared_ptr<IWindow> WindowManager::GetWindow(WindowHandle handle) const noexcept
    {
        if (m_WindowManagementService.contains(handle))
        {
            return m_WindowManagementService.at(handle);
        }

        MOTION_CORE_ERROR("Window with handle {:X} not found", handle);
        return nullptr;
    }
    
    std::shared_ptr<IWindow> WindowManager::GetActiveWindow() const noexcept
    {
        for (auto& window : m_WindowManagementService)
        {
            if (window.second->IsActive() && window.second->IsFocused())
            {
                return window.second;
            }
        }

        return nullptr;
    }
}