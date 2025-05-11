#include "CorePCH.hpp"
#include "Buffers.hpp"

namespace Motion::Core
{
    std::shared_ptr<IVertexBuffer> BuffersBuilder::CreateVertexBuffer(uint32_t alloca_size)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateVertexBuffer(alloca_size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IVertexBuffer> BuffersBuilder::CreateVertexBuffer(float * data, uint32_t size)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateVertexBuffer(data, size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IElementBuffer> BuffersBuilder::CreateElementBuffer(uint32_t * data, uint32_t size)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateElementBuffer(data, size);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IShaderBuffer> BuffersBuilder::CreateShaderBuffer(uint32_t size, BindingPoint binding)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateShaderBuffer(size, binding);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IUniformBuffer> BuffersBuilder::CreateUniformBuffer(uint32_t size, BindingPoint binding)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateUniformBuffer(size, binding);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<IFrameBuffer> BuffersBuilder::CreateFrameBuffer(const FrameBufferSpecification & specification)
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
