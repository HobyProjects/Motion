#include "CorePCH.hpp"
#include "Renderer.hpp"
#include "GL_Renderer.hpp"

namespace Motion {

#ifdef MOTION_PLATFORM_WINDOWS
    static RenderingAPI s_RenderingAPI = RenderingAPI::OpenGL;
#elif defined(MOTION_PLATFORM_LINUX)
    static RenderingAPI s_RenderingAPI = RenderingAPI::OpenGL;
#else
#   error "Unknown platform!"
#endif

    static GpuCaps s_Caps{};

    void Renderer::Init()
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_Init(); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
        QueryCaps_();
    }

    void Renderer::Quit()
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_Quit(); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    void Renderer::Clear()
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_Clear(); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    void Renderer::ClearColor(const glm::vec4& color)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_ClearColor(color); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    void Renderer::Clear(const ClearParams& p)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_ClearEx(p); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    void Renderer::SetViewport(std::int32_t x, std::int32_t y, std::int32_t w, std::int32_t h)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_SetViewport(x, y, w, h); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    void Renderer::DrawIndexed(std::int32_t indicesCount)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_DrawIndexed(indicesCount); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    void Renderer::DrawIndexed(const DrawIndexedArgs& args)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_DrawIndexed(args); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    std::int32_t Renderer::GetMaxTextureSlots() noexcept
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  return GL_GetMaxTextureSlots();
            case RenderingAPI::Vulkan:  return 0;
            case RenderingAPI::DirectX: return 0;
            default:                    return 0;
        }
    }

    void Renderer::BindTextureUnit(std::int32_t slot, std::uint32_t textureID)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_BindTextureUnit(slot, textureID); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    void Renderer::UnbindTextureUnit(std::int32_t slot)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_UnbindTextureUnit(slot); break;
            case RenderingAPI::Vulkan:  MOTION_ASSERT(false, "Vulkan not implemented!"); break;
            case RenderingAPI::DirectX: MOTION_ASSERT(false, "DirectX not implemented!"); break;
            default:                    MOTION_ASSERT(false, "Unknown rendering API!"); break;
        }
    }

    void Renderer::PushDebugGroup(const char* label)
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_PushDebugGroup(label); break;
            default: break;
        }
    }

    void Renderer::PopDebugGroup()
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_PopDebugGroup(); break;
            default: break;
        }
    }

    const GpuCaps& Renderer::Caps() { return s_Caps; }

    void Renderer::QueryCaps_()
    {
        switch (s_RenderingAPI)
        {
            case RenderingAPI::OpenGL:  GL_QueryCaps(s_Caps); break;
            default: break;
        }
    }

    RenderingAPI Renderer::GetAPI() noexcept { return s_RenderingAPI; }

} 
