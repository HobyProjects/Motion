#include "CorePCH.hpp"

namespace Motion::Core
{
    #ifdef MOTION_PLATFORM_WINDOWS 

        //static BaseAPIs s_PlatformBaseAPI = BaseAPIs::Win32;
        static BaseAPIs s_PlatformBaseAPI = BaseAPIs::GLFW;

    #elif defined(MOTION_PLATFORM_LINUX)

        static BaseAPIs s_PlatformBaseAPI = BaseAPIs::GLFW;

    #endif

    static std::shared_ptr<IPlatformBaseAPI> s_PlatformBaseAPIService = nullptr;
    static std::shared_ptr<IContext> s_ContextService = nullptr;
    static std::unordered_map<WindowHandle, std::shared_ptr<IWindow>> s_WindowManagementService;

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

    bool CoreAPI::Init()
    {
        switch(s_PlatformBaseAPI)
        {
            case BaseAPIs::GLFW:  s_PlatformBaseAPIService = std::make_shared<GLFW_BaseAPI>(); break;
            case BaseAPIs::Win32: MOTION_ASSERT(false, "Win32 is not supported yet"); break;
            default: MOTION_ASSERT(false, "Unknown Base API"); break;
        };

        if(!s_PlatformBaseAPIService->Init())
        {
            MOTION_ASSERT(false, "Failed to initialize Base API");
            return false;
        }

        if(Renderer::GetAPI() == RenderingAPI::OpenGL && s_PlatformBaseAPI == BaseAPIs::GLFW)
        {
            s_ContextService = std::make_shared<GLFW_GL_Context>();
            if(s_ContextService)
                return true;
        }

        if(Renderer::GetAPI() == RenderingAPI::OpenGL && s_PlatformBaseAPI == BaseAPIs::Win32)
        {
            MOTION_ASSERT(false, "Win32 is not supported yet");
            return false;
        }

        if(Renderer::GetAPI() == RenderingAPI::Vulkan && s_PlatformBaseAPI == BaseAPIs::GLFW)
        {
            MOTION_ASSERT(false, "Vulkan is not supported yet");
            return false;
        }

        return false;
    }

    /**
     * Shutdown the Core API.
     *
     * This function is idempotent and should be called once before the program exits.
     *
     * @note This function is NOT thread-safe and should not be called from multiple threads.
     */
    void CoreAPI::Quit()
    {
        if(s_PlatformBaseAPIService)
        {
            s_PlatformBaseAPIService->Quit();
            s_PlatformBaseAPIService.reset();
            
            s_ContextService->Detach();
            s_ContextService.reset();
        }
    }

    /**
     * Returns the platform base API used by the Core API.
     * 
     * @return The platform base API used by the Core API.
     * 
     * This function is thread-safe. It simply returns the value of the static member variable.
     * Note that this function does not create a copy of the platform base API instance.
     */
    BaseAPIs CoreAPI::API()
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

    std::shared_ptr<IPlatformBaseAPI> CoreAPI::GetBaseAPI()
    {
        return s_PlatformBaseAPIService;
    }

    /**
     * Retrieves the context service used by the Core API.
     *
     * @return A shared pointer to the context service.
     *
     * This function provides access to the context service currently in use.
     * It returns a shared pointer to the IContext instance, allowing for further
     * interaction with the context-specific implementation details.
     */
    std::shared_ptr<IContext> CoreAPI::GetContext()
    {
        return s_ContextService;
    }

    std::shared_ptr<IWindow> WindowManager::Create(const std::string& title)
    {
        WindowHandle uniqueHandle = UniqueIdentity::GetUniqueID();
        switch(s_PlatformBaseAPI)
        {
            case BaseAPIs::GLFW:
            {
                auto window = std::make_shared<GLFW_Window>(uniqueHandle, title, s_ContextService);
                s_WindowManagementService[uniqueHandle] = window;
                MOTION_CORE_INFO("Window created with handle {:X}", uniqueHandle);
                return window;
            }
            case BaseAPIs::Win32:
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
     * Destroys the specified window and removes it from the window management service.
     *
     * @param window A shared pointer to the window to be destroyed.
     *
     * This function checks if the provided window is not null. If it exists, it retrieves the
     * window handle and searches for it in the window management service map. If the window
     * is found, it is removed from the map. Finally, the shared pointer to the window is reset,
     * effectively destroying the window instance.
     */

    void WindowManager::Destroy(std::shared_ptr<IWindow>& window)
    {
        if(window)
        {
            auto it = s_WindowManagementService.find(window->GetHandle());
            if(it != s_WindowManagementService.end())
            {
                s_WindowManagementService.erase(it);
            }
            window.reset();
        }
    }

    /**
     * Retrieves a shared pointer to the window with the specified handle.
     *
     * @param handle The handle of the window to be retrieved.
     *
     * @return A shared pointer to the window with the specified handle, or nullptr if the window is not found.
     */
    std::shared_ptr<IWindow> WindowManager::Get(WindowHandle handle)
    {
        if(s_WindowManagementService.contains(handle))
        {
            return s_WindowManagementService[handle];
        }

        MOTION_CORE_ERROR("Window with handle {:X} not found", handle);
        return nullptr;
    }
}