#include "CorePCH.hpp"

namespace Motion
{
    /**
     * @brief Creates an unregistered plain texture with the specified name, width, and height.
     *
     * This function creates a texture object that is not registered with any resource manager.
     * The implementation depends on the current rendering API (OpenGL, Vulkan, DirectX).
     * For unsupported APIs, the function asserts and returns nullptr.
     *
     * @param name The name of the texture.
     * @param width The width of the texture in pixels.
     * @param height The height of the texture in pixels.
     * @param color The color to fill the texture with (default is white).
     * @return std::shared_ptr<ITexture> A shared pointer to the created texture, or nullptr if the API is unsupported.
     * @note Currently, only the OpenGL API is implemented. Vulkan and DirectX will assert and return nullptr.
     */
    std::shared_ptr<ITexture> ITexture::Create(std::int32_t width, std::int32_t height, const glm::vec3& color)
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

    /**
     * @brief Creates an unregistered texture from a file for the specified rendering API.
     *
     * This function loads a texture from the given file path and creates an unregistered texture object.
     * The implementation depends on the current rendering API (OpenGL, Vulkan, DirectX).
     * For unsupported APIs, the function asserts and returns nullptr.
     *
     * @param name The name to assign to the texture.
     * @param textureFile The filesystem path to the texture file.
     * @param type The type of texture to create (e.g., 2D, 3D, Cube).
     * @param flip Whether to vertically flip the texture during loading.
     * @return std::shared_ptr<ITexture> A shared pointer to the created texture, or nullptr if the API is unsupported.
     * @note Vulkan and DirectX implementations are not yet available.
     */
    std::shared_ptr<ITexture> ITexture::Create(const std::filesystem::path& textureFile, TextureType type, bool flip)
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_Texture::Create(textureFile, type, flip);
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

    /**
     * @brief Creates an unregistered texture from raw pixel data for the specified rendering API.
     *
     * This function creates a texture object from raw pixel data, allowing for custom texture generation.
     * The implementation depends on the current rendering API (OpenGL, Vulkan, DirectX).
     * For unsupported APIs, the function asserts and returns nullptr.
     *
     * @param data Pointer to the raw pixel data.
     * @param type The type of texture to create (e.g., 2D, 3D).
     * @param width The width of the texture in pixels.
     * @param height The height of the texture in pixels.
     * @param channels The number of color channels in the pixel data.
     * @return std::shared_ptr<ITexture> A shared pointer to the created texture, or nullptr if the API is unsupported.
     */
    std::shared_ptr<ITexture> ITexture::Create(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels)
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

    /**
     * @brief Creates an unregistered cube map texture for the specified rendering API.
     *
     * This function attempts to create a cube map texture using the current rendering API.
     * If the API is not implemented (e.g., Vulkan or DirectX), it asserts and returns nullptr.
     *
     * @param name The name to associate with the cube map texture.
     * @param textureFile The filesystem path to the texture file.
     * @return std::shared_ptr<ICubeTexture> A shared pointer to the created cube map texture,
     *         or nullptr if the API is not implemented or an error occurs.
     *
     * @note Currently, only the OpenGL API is implemented for cube map textures.
     */
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

    /**
     * @brief Creates an unregistered cube map texture from multiple texture files.
     *
     * This function constructs a shared pointer to a GL_CubeTexture object using the specified paths for each face of the cube map.
     * The created cube map texture is not registered with any texture manager or resource system.
     *
     * @param posX_texture The filesystem path to the positive X face texture.
     * @param negX_texture The filesystem path to the negative X face texture.
     * @param posY_texture The filesystem path to the positive Y face texture.
     * @param negY_texture The filesystem path to the negative Y face texture.
     * @param posZ_texture The filesystem path to the positive Z face texture.
     * @param negZ_texture The filesystem path to the negative Z face texture.
     * @return std::shared_ptr<GL_CubeTexture> A shared pointer to the newly created GL_CubeTexture object.
     */
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

