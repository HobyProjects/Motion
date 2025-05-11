#include "CorePCH.hpp"
#include "Arrays.hpp"

namespace Motion::Core
{
    std::shared_ptr<IVertexArray> ArrayBuilder::CreateVertexArray()
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateVertexArray();
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }
}
