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

    BaseAPIs CoreAPI::API()
    {
        return s_PlatformBaseAPI;
    }

    std::shared_ptr<IPlatformBaseAPI> CoreAPI::GetBaseAPI()
    {
        return s_PlatformBaseAPIService;
    }

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
                MOTION_CORE_INFO("Window created with handle {0}", uniqueHandle);
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

    std::shared_ptr<IWindow> WindowManager::Get(WindowHandle handle)
    {
        auto it = s_WindowManagementService.find(handle);
        if(it != s_WindowManagementService.end())
        {
            return it->second;
        }

        MOTION_CORE_WARN("Window with handle {0} does not exist", handle);
        return nullptr;
    }
}