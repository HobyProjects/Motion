#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<IRenderingStage> IRenderingStage::Create()
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:          return std::make_shared<GL_RenderingStage>();
            case RenderingAPI::Vulkan:          MOTION_ASSERT(false, "Vulkan is not implemented yet!"); break;
            case RenderingAPI::DirectX:         MOTION_ASSERT(false, "DirectX is not implemented yet!"); break;
            default:                            MOTION_ASSERT(false, "Unknown rendering API!");break;
        }

        return nullptr;
    }
}