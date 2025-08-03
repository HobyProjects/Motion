#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<IVertexArray> IVertexArray::Create()
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:         return GL_VertexArray::Create();
        case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr;
        case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr;
        default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }
}
