#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<IEnvironment> IEnvironment::Create(const std::filesystem::path& hdrFile)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL: return std::make_shared<GL_Environment>(hdrFile);
        case RenderingAPI::Vulkan:  MOTION_CORE_ERROR("Vulkan environment not implemented.");  return nullptr;
        case RenderingAPI::DirectX: MOTION_CORE_ERROR("DirectX environment not implemented."); return nullptr;
        default: MOTION_CORE_ERROR("Unknown rendering API."); return nullptr;
        }
    }
}