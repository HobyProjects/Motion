#include "CorePCH.hpp"
#include "Texture.hpp"

namespace Motion::Core
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
    std::shared_ptr<ITexture> CreateUnregisteredPlainTexture(const std::string& name, std::uint32_t width = 100, std::uint32_t height = 100, const glm::vec3& color = { 1.0f, 1.0f, 1.0f }) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CreateUnregisteredPlainTexture(name, width, height, color);
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
    std::shared_ptr<ITexture> CreateUnregisteredTextureFromFile(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CreateUnregisteredTextureFromFile(name, textureFile, type, flip);
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
     * @brief Creates an unregistered cube map texture for the specified rendering API.
     *
     * This function attempts to create a cube map texture using the current rendering API.
     * If the API is not implemented (e.g., Vulkan or DirectX), it asserts and returns nullptr.
     *
     * @param name The name to associate with the cube map texture.
     * @param textureFile The filesystem path to the texture file.
     * @return std::shared_ptr<ICubeMapTexture> A shared pointer to the created cube map texture,
     *         or nullptr if the API is not implemented or an error occurs.
     *
     * @note Currently, only the OpenGL API is implemented for cube map textures.
     */
    std::shared_ptr<ICubeMapTexture> CreateUnregisteredCubeMapTexture(const std::string& name, const std::filesystem::path& textureFile) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CreateUnregisteredCubeMapTexture(name, textureFile);
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
