#include "CorePCH.hpp"
#include "Buffers.hpp"

namespace Motion::Core
{
    std::shared_ptr<IVertexBuffer> BufferFactory::CreateVertexBuffer(std::uint32_t alloca_size)
    {
        auto& renderer = Renderer::GetInstance();
        switch (renderer.GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateVertexBuffer(alloca_size);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IVertexBuffer> BufferFactory::CreateVertexBuffer(float* data, std::uint32_t size)
    {
        auto& renderer = Renderer::GetInstance();
        switch (renderer.GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateVertexBuffer(data, size);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IElementBuffer> BufferFactory::CreateElementBuffer(std::uint32_t* data, std::uint32_t size)
    {
        auto& renderer = Renderer::GetInstance();
        switch (renderer.GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateElementBuffer(data, size);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IShaderBuffer> BufferFactory::CreateShaderBuffer(std::uint32_t size, BindingPoint binding)
    {
        auto& renderer = Renderer::GetInstance();
        switch (renderer.GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateShaderBuffer(size, binding);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IUniformBuffer> BufferFactory::CreateUniformBuffer(std::uint32_t size, BindingPoint binding)
    {
        auto& renderer = Renderer::GetInstance();
        switch (renderer.GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateUniformBuffer(size, binding);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IFrameBuffer> BufferFactory::CreateFrameBuffer(const FrameBufferSpecification& specification)
    {
        auto& renderer = Renderer::GetInstance();
        switch (renderer.GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_CreateFrameBuffer(specification);
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }


}
