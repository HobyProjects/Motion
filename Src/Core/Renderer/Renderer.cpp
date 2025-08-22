#include "CorePCH.hpp"
#include "Renderer.hpp"

namespace Motion
{
#ifdef MOTION_PLATFORM_WINDOWS
    // This should be DirectX but for now we are using OpenGL
    static RenderingAPI s_RenderingAPI = RenderingAPI::OpenGL;
#elif defined(MOTION_PLATFORM_LINUX)
    // This should be Vulkan but for now we are using OpenGL
    static RenderingAPI s_RenderingAPI = RenderingAPI::OpenGL;
#else
#error "Unknown platform!"
#endif

    void Renderer::Init()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Init();
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    void Renderer::Quit()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Quit();
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    RenderingAPI Renderer::GetAPI() noexcept
    {
        return s_RenderingAPI;
    }

    void Renderer::Clear()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_Clear();
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    void Renderer::ClearColor(const glm::vec4& color)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_ClearColor(color);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    void Renderer::SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_SetViewport(x, y, width, height);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    std::int32_t Renderer::GetMaxTextureSlots() noexcept
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            return GL_GetMaxTextureSlots();
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            return 0;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            return 0;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            return 0;
        }
    }

    void Renderer::BindTextureUnit(std::int32_t slot, std::uint32_t textureID)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_BindTextureUnit(slot, textureID);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    void Renderer::UnbindTextureUnit(std::int32_t slot)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_UnbindTextureUnit(slot);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    void Renderer::DrawIndexed(std::int32_t indicesCount)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_DrawIndexed(indicesCount);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    void Renderer::ApplyDrawFlags(DrawFlags flags)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_ApplyDrawFlags(flags);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        };
    }

    void Renderer::ResetDrawFlags(DrawFlags flags)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_ResetDrawFlags(flags);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }

    void Renderer::ResetDepthFunction()
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_ResetDepthFunction();
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }


    void Renderer::ApplyDepthFunction(DepthFunction depthFunction)
    {
        switch (s_RenderingAPI)
        {
        case RenderingAPI::OpenGL:
            GL_ApplyDepthFunction(depthFunction);
            break;
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan is not implemented yet!");
            break;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX is not implemented yet!");
            break;
        default:
            MOTION_ASSERT(false, "Unknown rendering API!");
            break;
        }
    }
}