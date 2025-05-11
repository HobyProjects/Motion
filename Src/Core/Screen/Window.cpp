#include "CorePCH.hpp"

namespace Motion::Core
{
    #ifdef MOTION_PLATFORM_WINDOWS 

        //static BaseAPIs s_BaseAPI = BaseAPIs::Win32;
        static BaseAPIs s_BaseAPI = BaseAPIs::GLFW;

    #elif defined(MOTION_PLATFORM_LINUX)

        static BaseAPIs s_BaseAPI = BaseAPIs::GLFW;

    #endif

    static std::shared_ptr<IBaseAPI> s_BaseAPIService = nullptr;
    static std::shared_ptr<IContext> s_ContextService = nullptr;
    static std::unordered_map<WindowHandle, std::shared_ptr<IWindow>> s_WindowMap;

    bool CoreAPI::Init()
    {
        switch(s_BaseAPI)
        {
            case BaseAPIs::GLFW:  s_BaseAPIService = std::make_shared<GLFW_BaseAPI>(); break;
            case BaseAPIs::SDL:   MOTION_ASSERT(false, "SDL is not supported yet"); break;
            case BaseAPIs::Win32: MOTION_ASSERT(false, "Win32 is not supported yet"); break;
            default: MOTION_ASSERT(false, "Unknown Base API"); break;
        };

        if(!s_BaseAPIService->Init())
        {
            MOTION_ASSERT(false, "Failed to initialize Base API");
            return false;
        }

        if(Renderer::GetAPI() == RenderingAPI::OpenGL && s_BaseAPI == BaseAPIs::GLFW)
        {
            s_ContextService = std::make_shared<GLFW_GL_Context>();
            if(s_ContextService)
                return true;
        }

        if(Renderer::GetAPI() == RenderingAPI::OpenGL && s_BaseAPI == BaseAPIs::SDL)
        {
            MOTION_ASSERT(false, "SDL is not supported yet");
            return false;
        }

        if(Renderer::GetAPI() == RenderingAPI::OpenGL && s_BaseAPI == BaseAPIs::Win32)
        {
            MOTION_ASSERT(false, "Win32 is not supported yet");
            return false;
        }

        if(Renderer::GetAPI() == RenderingAPI::Vulkan && s_BaseAPI == BaseAPIs::SDL)
        {
            MOTION_ASSERT(false, "SDL is not supported yet");
            return false;
        }
        
        if(Renderer::GetAPI() == RenderingAPI::Vulkan && s_BaseAPI == BaseAPIs::GLFW)
        {
            MOTION_ASSERT(false, "Vulkan is not supported yet");
            return false;
        }

        return false;
    }

    void CoreAPI::Quit()
    {
        if(s_BaseAPIService)
        {
            s_BaseAPIService->Quit();
            s_BaseAPIService.reset();
            
            s_ContextService->Detach();
            s_ContextService.reset();
        }
    }

    std::shared_ptr<IBaseAPI> CoreAPI::GetBaseAPI()
    {
        return s_BaseAPIService;
    }

    std::shared_ptr<IContext> CoreAPI::GetContext()
    {
        return s_ContextService;
    }

    WindowHandle WindowBuilder::UniqueHandle()
    {
        static WindowHandle s_Handle = 0;
        return s_Handle++;
    }

    std::shared_ptr<IWindow> WindowBuilder::Create(const std::string& title)
    {
        WindowHandle uniqueHandle = UniqueHandle();
        switch(s_BaseAPI)
        {
            case BaseAPIs::GLFW:
            {
                auto window = std::make_shared<GLFW_Window>(uniqueHandle, title, s_ContextService);
                s_WindowMap[uniqueHandle] = window;
                return window;
            }
            case BaseAPIs::SDL:
            {
                MOTION_ASSERT(false, "SDL is not supported yet");
                return nullptr;
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

    void WindowBuilder::Destroy(std::shared_ptr<IWindow>& window)
    {
        if(window)
        {
            auto it = s_WindowMap.find(window->GetHandle());
            if(it != s_WindowMap.end())
            {
                s_WindowMap.erase(it);
            }
            window.reset();
        }
    }

    std::shared_ptr<IWindow> WindowBuilder::Get(WindowHandle handle)
    {
        auto it = s_WindowMap.find(handle);
        if(it != s_WindowMap.end())
        {
            return it->second;
        }
        return nullptr;
    }
}