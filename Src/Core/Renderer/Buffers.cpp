#include "CorePCH.hpp"
#include "Buffers.hpp"

namespace Motion::Core
{
    std::shared_ptr<IVertexBuffer> BufferFactory::CreateVertexBuffer(uint32_t alloca_size)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateVertexBuffer(alloca_size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IVertexBuffer> BufferFactory::CreateVertexBuffer(float * data, uint32_t size)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateVertexBuffer(data, size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IElementBuffer> BufferFactory::CreateElementBuffer(uint32_t * data, uint32_t size)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateElementBuffer(data, size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IShaderBuffer> BufferFactory::CreateShaderBuffer(uint32_t size, BindingPoint binding)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateShaderBuffer(size, binding);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IUniformBuffer> BufferFactory::CreateUniformBuffer(uint32_t size, BindingPoint binding)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateUniformBuffer(size, binding);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IFrameBuffer> BufferFactory::CreateFrameBuffer(const FrameBufferSpecification & specification)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateFrameBuffer(specification);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }


}
