#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<ITexture> ITexture::Create(std::int32_t width, std::int32_t height, const glm::vec3& color) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_Texture::Create(width, height, color);
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan API is not yet implemented for plain textures.");
            return nullptr;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX API is not yet implemented for plain textures.");
            return nullptr;
        default:
            MOTION_ASSERT(false, "Unknown rendering API.");
            return nullptr;
        }
    }

    std::shared_ptr<ITexture> ITexture::Create(const std::filesystem::path& textureFile, TextureType type) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_Texture::Create(textureFile, type);
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan API is not yet implemented for texture files.");
            return nullptr;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX API is not yet implemented for texture files.");
            return nullptr;
        default:
            MOTION_ASSERT(false, "Unknown rendering API.");
            return nullptr;
        }
    }


    std::shared_ptr<ITexture> ITexture::Create(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_Texture::Create(data, type, width, height, channels);
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan API is not yet implemented for raw texture data.");
            return nullptr;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX API is not yet implemented for raw texture data.");
            return nullptr;
        default:
            MOTION_ASSERT(false, "Unknown rendering API.");
            return nullptr;
        }
    }


    std::shared_ptr<ICubeTexture> ICubeTexture::Create(const std::filesystem::path& textureFile) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CubeTexture::Create(textureFile);
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan API is not yet implemented for cube map textures.");
            return nullptr;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX API is not yet implemented for cube map textures.");
            return nullptr;
        default:
            MOTION_ASSERT(false, "Unknown rendering API.");
            return nullptr;
        }
    }


    std::shared_ptr<ICubeTexture> ICubeTexture::Create(const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture, const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture, const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CubeTexture::Create(posX_texture, negX_texture, posY_texture, negY_texture, posZ_texture, negZ_texture);
        case RenderingAPI::Vulkan:
            MOTION_ASSERT(false, "Vulkan API is not yet implemented for cube map textures.");
            return nullptr;
        case RenderingAPI::DirectX:
            MOTION_ASSERT(false, "DirectX API is not yet implemented for cube map textures.");
            return nullptr;
        default:
            MOTION_ASSERT(false, "Unknown rendering API.");
            return nullptr;
        }
    }
}

