#include "CorePCH.hpp"

namespace Motion::Core
{
#ifdef MOTION_PLATFORM_WINDOWS 

    /**
     *  @note Since Win32 is not implemented yet, the platform base API is set to GLFW.
     */
    static PlatformBaseAPIs s_PlatformBaseAPI = PlatformBaseAPIs::GLFW;

#elif defined(MOTION_PLATFORM_LINUX)

    static PlatformBaseAPIs s_PlatformBaseAPI = PlatformBaseAPIs::GLFW;

#endif

    /**
     * Initializes the CoreAPI by setting up the platform base API service and context service.
     *
     * @return true if initialization is successful; false otherwise.
     *
     * This function first determines the platform base API to use based on the current platform
     * and sets up the corresponding platform base API service. It asserts if an unsupported or
     * unknown base API is encountered. After initializing the platform base API service,
     * it sets up the context service based on the rendering API and platform base API.
     * Currently, only the combination of OpenGL and GLFW is supported.
     *
     * Assertions are triggered for unsupported combinations like Win32 platform base API and
     * Vulkan rendering API.
     */
    bool CoreAPI::Init() noexcept
    {
        switch (s_PlatformBaseAPI)
        {
        case PlatformBaseAPIs::GLFW:    m_PlatformBaseAPIService = std::make_shared<GLFW_BaseAPI>(); break;
        case PlatformBaseAPIs::Win32:   MOTION_ASSERT(false, "Win32 is not supported yet"); break;
        default:                        MOTION_ASSERT(false, "Unknown Base API"); break;
        };

        return m_PlatformBaseAPIService->Init();
    }

    /**
     * Shutdown the Core API.
     *
     * This function is idempotent and should be called once before the program exits.
     *
     * @note This function is NOT thread-safe and should not be called from multiple threads.
     */
    void CoreAPI::Quit() noexcept
    {
        MOTION_CORE_INFO("Shutting down Core API...");
        if (m_PlatformBaseAPIService)
            m_PlatformBaseAPIService->Quit();
    }

    /**
     * Returns the platform base API used by the Core API.
     *
     * @return The platform base API used by the Core API.
     *
     * This function is thread-safe. It simply returns the value of the static member variable.
     * Note that this function does not create a copy of the platform base API instance.
     */
    PlatformBaseAPIs CoreAPI::API() const noexcept
    {
        return s_PlatformBaseAPI;
    }

    /**
     * Retrieves the platform base API service used by the Core API.
     *
     * @return A shared pointer to the platform base API service.
     *
     * This function provides access to the platform base API service currently in use.
     * It returns a shared pointer to the IPlatformBaseAPI instance, allowing for further
     * interaction with the platform-specific implementation details.
     */

    std::shared_ptr<IPlatformBaseAPI> CoreAPI::GetBaseAPI() const noexcept
    {
        return m_PlatformBaseAPIService;
    }

    /**
     * @brief Creates a new window instance based on the current platform API.
     *
     * This function generates a unique window handle and creates a window using the
     * appropriate platform-specific implementation. The created window is stored in
     * the window management service for future reference.
     *
     * @param title The title of the window to be created.
     * @return std::shared_ptr<IWindow> A shared pointer to the created window instance.
     *         Returns nullptr if the platform API is not supported or unknown.
     *
     * @note Currently, only the GLFW platform is supported. Attempting to use Win32 or
     *       an unknown API will trigger an assertion and return nullptr.
     */
    std::shared_ptr<IWindow> WindowManager::Create(const std::string& title) noexcept
    {
        WindowHandle uniqueHandle = UniqueIdentity::GetUniqueID();
        switch (s_PlatformBaseAPI)
        {
        case PlatformBaseAPIs::GLFW:
        {
            auto& coreAPI = CoreAPI::GetInstance();
            auto window = std::make_shared<GLFW_Window>(uniqueHandle, title);
            m_WindowManagementService[uniqueHandle] = window;

            MOTION_CORE_INFO("Window created with handle {:X}", uniqueHandle);
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
     * Destroys the window with the specified handle.
     *
     * This function checks if the window exists in the management service and removes it.
     * If the window is found, it is destroyed and removed from the service.
     * If the window is not found, an error message is logged.
     *
     * @param windowHandle The handle of the window to be destroyed.
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
     * Retrieves a shared pointer to the window with the specified handle.
     *
     * @param handle The handle of the window to be retrieved.
     *
     * @return A shared pointer to the window with the specified handle, or nullptr if the window is not found.
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
     * Retrieves the currently active and focused window.
     *
     * This function iterates over all managed windows and returns the first window
     * that is both active and focused. If no such window is found, it returns nullptr.
     *
     * @return A shared pointer to the active and focused window, or nullptr if no window
     *         meets the criteria.
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