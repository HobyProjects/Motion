#include "CorePCH.hpp"

namespace Motion::Core
{
    std::shared_ptr<ITexture> TextureBuilder::CreatePlainTexture(uint32_t width, uint32_t height)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreatePlainTexture(width, height);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }

    std::shared_ptr<ITexture> TextureBuilder::CreateTextureFromFile(const std::filesystem::path & filePath, TextureType type, bool flipTexture)
    {
        switch(Renderer::GetAPI())
        {
            case RenderingAPI::OpenGL:         return GL_CreateTextureFromFile(filePath, type, flipTexture);
            case RenderingAPI::Vulkan:         MOTION_ASSERT(false, "Vulkan is not implemented yet!"); return nullptr; 
            case RenderingAPI::DirectX:        MOTION_ASSERT(false, "DirectX is not implemented yet!"); return nullptr; 
            default:                           MOTION_ASSERT(false, "Unknown rendering API!"); return nullptr;
        };
    }
}


