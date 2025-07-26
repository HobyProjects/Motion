#include "CorePCH.hpp"
#include "Texture.hpp"

namespace Motion
{
    /**
     * @brief Represents a binding point for textures in the rendering pipeline.
     *
     * This class provides static methods to manage texture binding points,
     * allowing for efficient texture management during rendering operations.
     */
    static std::int32_t s_BindingPoint = 0;

    /**
     * @brief Returns a new binding point for textures.
     *
     * This method increments the static binding point counter and returns the new value.
     * It is used to assign unique binding points to textures in the rendering pipeline.
     *
     * @return BindingPoint The next available binding point.
     */
    std::int32_t TextureBinding::Point() noexcept
    {
        if (s_BindingPoint >= Renderer::GetMaxTextureSlots())
        {
            MOTION_CORE_ERROR("Exceeded maximum texture slots available in the renderer.");
            return -1; // Return -1 or handle error appropriately
        }

        return s_BindingPoint++;
    }

    /**
     * @brief Resets the texture binding point to zero.
     *
     * This method sets the static binding point counter back to zero,
     * effectively clearing any previously assigned binding points.
     * It is useful for resetting the state of texture bindings in the rendering pipeline.
     */
    void TextureBinding::Reset() noexcept
    {
        s_BindingPoint = 0;
    }

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
    std::shared_ptr<ITexture> CreateUnregisteredPlainTexture(std::int32_t width, std::int32_t height, const glm::vec3& color) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CreateUnregisteredPlainTexture(width, height, color);
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
    std::shared_ptr<ITexture> CreateUnregisteredTextureFromFile(const std::filesystem::path& textureFile, TextureType type, bool flip) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CreateUnregisteredTextureFromFile(textureFile, type, flip);
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
     * @return std::shared_ptr<ICubeTexture> A shared pointer to the created cube map texture,
     *         or nullptr if the API is not implemented or an error occurs.
     *
     * @note Currently, only the OpenGL API is implemented for cube map textures.
     */
    std::shared_ptr<ICubeTexture> CreateUnregisteredCubeMapTexture(const std::filesystem::path& textureFile) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CreateUnregisteredCubeMapTexture(textureFile);
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
     * This function constructs a shared pointer to a GL_CubeMapTexture object using the specified paths for each face of the cube map.
     * The created cube map texture is not registered with any texture manager or resource system.
     *
     * @param posX_texture The filesystem path to the positive X face texture.
     * @param negX_texture The filesystem path to the negative X face texture.
     * @param posY_texture The filesystem path to the positive Y face texture.
     * @param negY_texture The filesystem path to the negative Y face texture.
     * @param posZ_texture The filesystem path to the positive Z face texture.
     * @param negZ_texture The filesystem path to the negative Z face texture.
     * @return std::shared_ptr<GL_CubeMapTexture> A shared pointer to the newly created GL_CubeMapTexture object.
     */
    std::shared_ptr<ICubeTexture> Motion::CreateUnregisteredCubeMapTexture(const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture, const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture, const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) noexcept
    {
        switch (Renderer::GetAPI())
        {
        case RenderingAPI::OpenGL:
            return GL_CreateUnregisteredCubeMapTexture(posX_texture, negX_texture, posY_texture, negY_texture, posZ_texture, negZ_texture);
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

