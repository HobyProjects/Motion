#include "CorePCH.hpp"

namespace Motion
{
    static PlatformBaseAPIs s_PlatformBaseAPI = PlatformBaseAPIs::GLFW;  

    /**
     * Initializes the Core API with the specified platform base API.
     * This function is responsible for initializing the underlying platform
     * base API and returning true if the initialization was successful, false
     * otherwise.
     * If the platform base API is not supported, an assert will be triggered.
     * @return true if the initialization was successful, false otherwise
     */
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

    /**
     * Shuts down the Core API.
     * This function is responsible for cleaning up the underlying platform
     * base API and releasing any resources allocated by the Core API.
     * It is recommended to call this function when the application is shutting
     * down to ensure that all resources are released properly.
     */
    void CoreAPI::Quit() noexcept
    {
        MOTION_CORE_INFO("Shutting down Core API...");
        if (m_PlatformBaseAPIService) m_PlatformBaseAPIService->Quit();
    }

    /**
     * @return The platform base API being used by the Core API.
     * This function returns the platform base API being used by the Core API.
     * It is useful for determining which platform base API to use when
     * initializing the Core API manually.
     */
    PlatformBaseAPIs CoreAPI::API() const noexcept
    {
        return s_PlatformBaseAPI;
    }

    /**
     * @brief Gets the platform base API being used by the Core API.
     * This function returns the platform base API being used by the Core API.
     * It is useful for determining which platform base API to use when
     * initializing the Core API manually.
     * @return The platform base API being used by the Core API.
     * @note The returned pointer is valid until the Core API is shut down.
     */
    std::shared_ptr<IPlatformBaseAPI> CoreAPI::GetBaseAPI() const noexcept
    {
        return m_PlatformBaseAPIService;
    }

    /**
     * @brief Gets the context based on the specified platform base API.
     * This function returns the context based on the specified platform base API.
     * It is useful for determining which context to use when
     * initializing the Core API manually.
     * @return The context based on the specified platform base API.
     * @note The returned pointer is valid until the Core API is shut down.
     */
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

    /**
     * @brief Creates a new window with the specified title and visibility.
     * This function creates a new window with the specified title and visibility.
     * It is useful for creating new windows programmatically.
     * @param title The title of the window.
     * @param isVisible Whether the window is visible or not.
     * @param sharedWindow The shared window to create the new window from.
     * @return A pointer to the newly created window.
     * @note The returned pointer is valid until the Core API is shut down.
     */
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

    /**
     * @brief Destroys a window with the specified handle.
     * This function destroys a window with the specified handle.
     * It is useful for destroying windows programmatically.
     * @param windowHandle The handle of the window to destroy.
     * @note The returned pointer is valid until the Core API is shut down.
     */
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

    /**
     * @brief Gets a window with the specified handle.
     * This function gets a window with the specified handle.
     * It is useful for getting a window programmatically.
     * @param handle The handle of the window to get.
     * @return A pointer to the window with the specified handle, or nullptr if not found.
     * @note The returned pointer is valid until the Core API is shut down.
     */
    std::shared_ptr<IWindow> WindowManager::GetWindow(WindowHandle handle) const noexcept
    {
        if (m_WindowManagementService.contains(handle))
        {
            return m_WindowManagementService.at(handle);
        }

        MOTION_CORE_ERROR("Window with handle {:X} not found", handle);
        return nullptr;
    }
    
    /**
     * @brief Gets the active window from the window management service.
     * This function gets the active window from the window management service.
     * The active window is the window that is currently focused and active.
     * @return A pointer to the active window, or nullptr if no active window is found.
     * @note The returned pointer is valid until the Core API is shut down.
     */
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