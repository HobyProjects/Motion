#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<IVertexBuffer> IVertexBuffer::Create(std::int32_t allocatorSize)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_VertexBuffer::Create(allocatorSize);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return nullptr;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return nullptr;
        };
    }

    std::shared_ptr<IVertexBuffer> IVertexBuffer::Create(const Vertex* data, std::uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_VertexBuffer::Create(data, size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return nullptr;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return nullptr;
        };
    }

    std::shared_ptr<IVertexBuffer> IVertexBuffer::Create(const float* data, std::uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_VertexBuffer::Create(data, size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return nullptr;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return nullptr;
        };
    }

    std::shared_ptr<IElementBuffer> IElementBuffer::Create(const std::uint32_t* data, std::uint32_t size)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ElementBuffer::Create(data, size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return nullptr;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return nullptr;
        };
    }

    std::shared_ptr<IShaderBuffer> IShaderBuffer::Create(std::int32_t size, BindingPoint binding)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_ShaderBuffer::Create(size, binding);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return nullptr;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return nullptr;
        };
    }

    std::shared_ptr<IUniformBuffer> IUniformBuffer::Create(std::int32_t size, BindingPoint binding)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_UniformBuffer::Create(size, binding);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return nullptr;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return nullptr;
        };
    }

    std::shared_ptr<IFrameBuffer> IFrameBuffer::Create(const FrameBufferSpecification& specification)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_FrameBuffer::Create(specification);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return nullptr;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return nullptr;
        };
    }

    std::shared_ptr<ICaptureFrameBuffer> ICaptureFrameBuffer::Create(std::int32_t width, std::int32_t height)
    {
        switch (Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CaptureFrameBuffer::Create(width, height);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!");      return nullptr;
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!");     return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!");              return nullptr;
        };
    }
}
