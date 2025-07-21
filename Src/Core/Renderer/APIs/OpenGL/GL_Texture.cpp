#include "CorePCH.hpp"
#include "GL_Texture.hpp"

namespace Motion::Core
{
    /**
     * Generates a plain 2D texture with the specified name, width, and height.
     *
     * @param[in] name   The name of the texture
     * @param[in] width  The width of the texture
     * @param[in] height The height of the texture
     *
     * @return A new GL_Texture object
     */
    GL_Texture::GL_Texture(const std::string& name, uint32_t width, uint32_t height) : AssetBase<ITexture>(UniqueIdentity::GetUniqueID(), name, AssetType::Texture, "PlainTexture")
    {
        if (!GenerateTexture2D(width, height))
        {
            MOTION_ASSERT(false, "Unable to generate texture of size {0}x{1}", width, height);
            return;
        }

        m_Specification.Type = TextureType::BaseColorMapsTexture;
        m_Specification.Source = TextureSource::GeneratedTexture;
        AssetInfo.IsInitialized = true;
    }

    /**
     * Constructs a new GL_Texture object with the specified UUID, name, width, and height. This
     * will generate a plain 2D texture with the specified width and height, and assign it the
     * specified UUID and name. The texture will be of type TextureType::BaseColorMapsTexture.
     *
     * @param[in] uuid   The UUID of the texture
     * @param[in] name   The name of the texture
     * @param[in] width  The width of the texture
     * @param[in] height The height of the texture
     */
    GL_Texture::GL_Texture(UUID uuid, const std::string& name, uint32_t width, uint32_t height) : AssetBase<ITexture>(uuid, name, AssetType::Texture, "PlainTexture")
    {
        if (!GenerateTexture2D(width, height))
        {
            MOTION_ASSERT(false, "Unable to generate texture of size {0}x{1}", width, height);
            return;
        }

        m_Specification.Type = TextureType::BaseColorMapsTexture;
        m_Specification.Source = TextureSource::GeneratedTexture;
        AssetInfo.IsInitialized = true;
    }

    /**
    * Constructs a new GL_Texture object using the specified name and texture file path.
    * Loads the texture from the given file and sets its type and flip option as specified.
    *
    * @param[in] name        The name of the texture
    * @param[in] textureFile The filesystem path to the texture file
    * @param[in] type        The type of the texture (e.g., diffuse, specular)
    * @param[in] flip        Whether to flip the texture vertically during loading
    *
    * @note If the texture cannot be loaded from the file, an assertion will be triggered.
    */
    GL_Texture::GL_Texture(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip) : AssetBase<ITexture>(UniqueIdentity::GetUniqueID(), name, AssetType::Texture, textureFile.string())
    {
        if (!LoadTextureFromFile(textureFile, flip))
        {
            MOTION_ASSERT(false, "Unable to load texture file {0}", textureFile.string());
            return;
        }

        m_Specification.Type = type;
        m_Specification.Source = TextureSource::TextureFile;
        AssetInfo.IsInitialized = true;
    }

    /**
     * Constructs a new GL_Texture object using the specified UUID, name, and texture file path.
     * Loads the texture from the given file and sets its type and flip option as specified.
     *
     * @param[in] uuid        The UUID of the texture
     * @param[in] name        The name of the texture
     * @param[in] textureFile The filesystem path to the texture file
     * @param[in] type        The type of the texture (e.g., diffuse, specular)
     * @param[in] flip        Whether to flip the texture vertically during loading
     *
     * @note If the texture cannot be loaded from the file, an assertion will be triggered.
     */
    GL_Texture::GL_Texture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip) : AssetBase<ITexture>(uuid, name, AssetType::Texture, textureFile.string())
    {
        if (!LoadTextureFromFile(textureFile, flip))
        {
            MOTION_ASSERT(false, "Unable to load texture file {0}", textureFile.string());
            return;
        }

        m_Specification.Type = type;
        m_Specification.Source = TextureSource::TextureFile;
        AssetInfo.IsInitialized = true;
    }

    /**
     * Destructor for GL_Texture objects.
     *
     * Frees up all resources allocated by the GL_Texture object. If the texture was not loaded from a file, the
     * destructor will delete the texture data pointer and call glDeleteTextures to free up the OpenGL texture object.
     * If the texture was loaded from a file, the destructor will call SOIL_free_image_data to free up the texture data
     * and call glDeleteTextures to free up the OpenGL texture object.
     */
    GL_Texture::~GL_Texture()
    {
        if (!m_FromFile)
        {
            glDeleteTextures(1, &m_Specification.TexID);
            m_Specification.TextureData.reset();
        }

        if (m_FromFile)
        {
            glDeleteTextures(1, &m_Specification.TexID);
            SOIL_free_image_data(m_Specification.TextureData.get());
            m_Specification.TexID = 0;
        }
    }

    /**
    * Binds the 2D texture to the current OpenGL context.
    *
    * This function sets the active texture to this texture's ID, allowing
    * it to be used for subsequent OpenGL operations.
    */
    void GL_Texture::Bind() const noexcept
    {
        glBindTexture(GL_TEXTURE_2D, m_Specification.TexID);
    }

    /**
     * @brief Binds the texture to the specified binding point.
     *
     * This function binds the OpenGL texture represented by this object to the given binding point,
     * making it active for subsequent rendering operations.
     *
     * @param bindingPoint The texture unit or binding point to which the texture should be bound.
     */
    void GL_Texture::Bind(uint32_t bindingPoint) const noexcept
    {
        glBindTextureUnit(bindingPoint, m_Specification.TexID);
    }

    /**
     * @brief Unbinds the currently bound 2D texture from the OpenGL context.
     *
     * This method resets the active texture binding for GL_TEXTURE_2D to 0,
     * effectively unbinding any texture that was previously bound. This is
     * useful to prevent unintended modifications to textures or to ensure
     * that no texture is bound when rendering.
     */
    void GL_Texture::Unbind() const noexcept
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    /**
     * @brief Returns the OpenGL texture ID of this texture.
     *
     * This function retrieves the unique identifier for the OpenGL texture
     * associated with this GL_Texture instance.
     *
     * @return The OpenGL texture ID.
     */
    TextureID GL_Texture::GetID() const noexcept
    {
        return m_Specification.TexID;
    }

    /**
     * @brief Returns the texture specification for this texture.
     *
     * This function retrieves the texture specification, which includes
     * details such as width, height, format, and other properties of the texture.
     *
     * @return The texture specification.
     */
    TextureSpecification& GL_Texture::GetSpecification() noexcept
    {
        return m_Specification;
    }


    /**
     * @brief Returns the source of the texture.
     *
     * This function retrieves the source type of the texture, indicating whether it was loaded from a file,
     * generated programmatically, or created from other means.
     *
     * @return The source type of the texture.
     */
    TextureSource GL_Texture::Source() const noexcept
    {
        return m_Specification.Source;
    }

    /**
     * Loads a texture from the given file using SOIL
     *
     * @param[in] textureFile The path to the texture file to load
     * @param[in] flip        Whether to flip the image vertically
     *
     * @return Whether the texture could be loaded successfully
     */
    bool GL_Texture::LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip)
    {
        MOTION_ASSERT(std::filesystem::exists(textureFile), "Unable to load texture file {0}", textureFile.string());

        std::int32_t loaderFlags = (flip) ?
            (SOIL_LOAD_RGBA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_COMPRESS_TO_DXT) :
            (SOIL_LOAD_RGBA | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_COMPRESS_TO_DXT);

        m_Specification.TexID = SOIL_load_OGL_texture(textureFile.string().c_str(), SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, loaderFlags);
        MOTION_ASSERT(m_Specification.TexID, "Unable to load texture file {0}; {1}", textureFile.string(), SOIL_last_result());

        m_FromFile = true;
        if (m_Specification.TextureData)
        {
            AssetInfo.AssetName = textureFile.filename().string();
            AssetInfo.AssetSource = textureFile.string();
            AssetInfo.IsInitialized = true;
            return true;
        }

        return false;
    }


    /**
     * Generates a blank 2D texture with the specified width and height
     *
     * @param[in] width  The width of the texture
     * @param[in] height The height of the texture
     *
     * @return Whether the texture could be generated successfully
     */
    bool GL_Texture::GenerateTexture2D(std::uint32_t width, std::uint32_t height)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;
        m_Specification.NumberOfChannels = GL_RGBA;
        m_Specification.InternalDataFormat = GL_RGBA8;
        m_Specification.TextureDataFormat = GL_RGBA;

        uint32_t textureAllocateSize = m_Specification.Width * m_Specification.Height * m_Specification.NumberOfChannels;
        m_Specification.TextureData = std::make_unique<std::uint8_t[]>(textureAllocateSize);
        std::memset(m_Specification.TextureData.get(), 255, textureAllocateSize);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_Specification.TexID);
        glBindTexture(GL_TEXTURE_2D, m_Specification.TexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, m_Specification.InternalDataFormat, m_Specification.Width, m_Specification.Height, 0, m_Specification.TextureDataFormat, GL_UNSIGNED_BYTE, m_Specification.TextureData.get());
        glGenerateMipmap(GL_TEXTURE_2D);

        return true;
    }



    /**
     * @brief Constructs a GL_CubeMapTexture object by loading a cube map texture from the specified file.
     *
     * @param name The name to assign to the cube map texture asset.
     * @param textureFile The filesystem path to the cube map texture file.
     *
     * This constructor initializes the cube map texture asset with a unique ID, name, asset type, and file path.
     * It attempts to load the cube map texture from the provided file path using LoadCubeMapTextureHDR().
     * If loading fails, an assertion is triggered and the constructor returns early.
     * On successful load, the texture's metadata is marked as loaded.
     */
    GL_CubeMapTexture::GL_CubeMapTexture(const std::string& name, const std::filesystem::path& textureFile) : AssetBase<ICubeMapTexture>(UniqueIdentity::GetUniqueID(), name, AssetType::Texture, textureFile.string())
    {
        if (!LoadCubeMapTexture(textureFile))
        {
            MOTION_ASSERT(false, "Unable to load cube map texture file {0}", textureFile.string());
            return;
        }

        AssetInfo.IsInitialized = true;
    }

    /**
     * @brief Constructs a GL_CubeMapTexture object and attempts to load a cube map texture from the specified file.
     *
     * @param uuid        The unique identifier for the texture asset.
     * @param name        The name of the texture asset.
     * @param textureFile The filesystem path to the cube map texture file.
     *
     * This constructor initializes the base AssetBase with the provided UUID, name, asset type, and file path.
     * It then attempts to load the cube map texture in HDR format from the given file path. If loading fails,
     * an assertion is triggered and the constructor returns early. On successful load, the texture's metadata
     * is marked as loaded.
     */
    GL_CubeMapTexture::GL_CubeMapTexture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile) : AssetBase<ICubeMapTexture>(uuid, name, AssetType::Texture, textureFile.string())
    {
        if (!LoadCubeMapTexture(textureFile))
        {
            MOTION_ASSERT(false, "Unable to load cube map texture file {0}", textureFile.string());
            return;
        }

        AssetInfo.IsInitialized = true;
    }

    /**
     * @brief Destructor for GL_CubeMapTexture.
     *
     * This destructor cleans up the resources associated with the cube map texture,
     * including deleting the OpenGL texture object and freeing any allocated texture data.
     */
    GL_CubeMapTexture::~GL_CubeMapTexture()
    {
        if (m_Specification.TexID != 0)
        {
            glDeleteTextures(1, &m_Specification.TexID);
            m_Specification.TexID = 0;
        }

        if (m_Specification.TextureData)
        {
            SOIL_free_image_data(m_Specification.TextureData.get());
            m_Specification.TextureData.reset();
            m_Specification.TextureData = nullptr;
        }
    }

    /**
     * @brief Binds the cube map texture to the current OpenGL context.
     *
     * This method makes the cube map texture specified by m_Specification.TexID
     * active for subsequent OpenGL operations. It is a no-throw operation.
     */
    void GL_CubeMapTexture::Bind() const noexcept
    {
        glBindTextureUnit(0, m_Specification.TexID);
    }

    /**
     * @brief Unbinds the currently bound OpenGL cube map texture.
     *
     * This method unbinds any cube map texture that is currently bound to the OpenGL context
     * by binding texture ID 0 to the GL_TEXTURE_CUBE_MAP target. This is typically used to
     * prevent accidental modification of the texture or to reset the texture binding state.
     *
     * @note This function does not throw exceptions.
     */
    void GL_CubeMapTexture::Unbind() const noexcept
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    }

    /**
     * @brief Retrieves the OpenGL texture ID associated with this cube map texture.
     * @return TextureID The unique identifier for the OpenGL texture object.
     * @note This function does not modify any member variables and is guaranteed not to throw exceptions.
     */
    TextureID GL_CubeMapTexture::GetID() const noexcept
    {
        return m_Specification.TexID;
    }

    /**
     * @brief Retrieves the specification of the cube map texture.
     *
     * This method returns the current TextureSpecification associated with this
     * GL_CubeMapTexture instance. The specification typically contains details
     * such as texture format, dimensions, filtering, and wrapping modes.
     *
     * @return The TextureSpecification of the cube map texture.
     */
    TextureSpecification& GL_CubeMapTexture::GetSpecification() noexcept
    {
        return m_Specification;
    }

    /**
     * @brief Sets the data for a specific face of the cube map texture at a given mipmap level.
     *
     * This function binds the cube map texture and uploads the provided image data to the specified face and mipmap level.
     *
     * @param face      The index of the cube map face (0 for positive X, 1 for negative X, 2 for positive Y, etc.).
     * @param mipLevel  The mipmap level to set the data for.
     * @param width     The width of the texture image.
     * @param height    The height of the texture image.
     * @param format    The format of the texture data (e.g., GL_RGB, GL_RGBA).
     * @param data      Pointer to the image data to upload.
     */
    void GL_CubeMapTexture::SetFace(std::uint32_t face, std::int32_t mipLevel, std::uint32_t width, std::uint32_t height, std::uint32_t format, const void* data)
    {
        if (data != nullptr)
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_Specification.TexID);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mipLevel, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        else
        {
            MOTION_ASSERT(false, "Data pointer is null for setting cube map face {0} at mip level {1}", face, mipLevel);
        }
    }

    /**
     * @brief Loads a cube map texture from the specified HDR file.
     *
     * This function attempts to load a cube map texture in HDR format from the given file path.
     * It uses SOIL to load the texture and sets the texture ID in the specification if successful.
     *
     * @param textureFile The filesystem path to the cube map texture file.
     * @return True if the texture was loaded successfully, false otherwise.
     */
    bool GL_CubeMapTexture::LoadCubeMapTexture(const std::filesystem::path& textureFile)
    {
        if (!std::filesystem::exists(textureFile))
        {
            MOTION_CORE_ERROR("Cube map texture file {0} does not exist", textureFile.string());
            return false;
        }

        m_Specification.TextureData.reset(SOIL_load_image(textureFile.string().c_str(), &m_Specification.Width, &m_Specification.Height, &m_Specification.NumberOfChannels, SOIL_LOAD_AUTO | SOIL_FLAG_TEXTURE_REPEATS | SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y));
        if (!m_Specification.TextureData)
        {
            MOTION_CORE_ERROR("Failed to load cube map texture from file {0}: {1}", textureFile.string(), SOIL_last_result());
            return false;
        }

        AssetInfo.AssetName = textureFile.filename().string();
        AssetInfo.AssetSource = textureFile.string();
        AssetInfo.IsInitialized = true;

        m_Specification.InternalDataFormat = GL_RGBA;
        m_Specification.TextureDataFormat = GL_RGBA;

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_Specification.TexID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_Specification.TexID);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        for (std::uint32_t face = 0; face < 6; ++face)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, m_Specification.InternalDataFormat, m_Specification.Width, m_Specification.Height, 0, m_Specification.TextureDataFormat, GL_UNSIGNED_BYTE, m_Specification.TextureData.get());
        }

        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        SOIL_free_image_data(m_Specification.TextureData.get());
        m_Specification.TextureData = nullptr;
        return true;
    }

    /**
     * @brief Creates an unregistered plain OpenGL texture.
     *
     * This function constructs a shared pointer to a GL_Texture object with the specified name, width, and height.
     * The created texture is not registered with any texture manager or resource system.
     *
     * @param name The name to assign to the texture.
     * @param width The width of the texture in pixels.
     * @param height The height of the texture in pixels.
     * @return std::shared_ptr<GL_Texture> A shared pointer to the newly created GL_Texture object.
     * @note This function is noexcept and guarantees not to throw exceptions.
     */
    std::shared_ptr<GL_Texture> GL_CreateUnregisteredPlainTexture(const std::string& name, std::uint32_t width, std::uint32_t height) noexcept
    {
        return std::make_shared<GL_Texture>(name, width, height);
    }

    /**
     * @brief Creates an unregistered OpenGL texture from a file.
     *
     * This function constructs a shared pointer to a GL_Texture object using the specified parameters.
     * The texture is not registered with any texture manager or resource system.
     *
     * @param name        The name to assign to the texture.
     * @param textureFile The filesystem path to the texture file.
     * @param type        The type of texture (e.g., 2D).
     * @param flip        Whether to vertically flip the texture during loading.
     * @return std::shared_ptr<GL_Texture> A shared pointer to the created GL_Texture object.
     * @note The function is noexcept and will not throw exceptions.
     */
    std::shared_ptr<GL_Texture> GL_CreateUnregisteredTextureFromFile(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip) noexcept
    {
        return std::make_shared<GL_Texture>(name, textureFile, type, flip);
    }

    /**
     * @brief Creates an unregistered OpenGL cube map texture from a file.
     *
     * This function constructs a shared pointer to a GL_CubeMapTexture object using the specified name and texture file.
     * The created cube map texture is not registered with any texture manager or resource system.
     *
     * @param name        The name to assign to the cube map texture.
     * @param textureFile The filesystem path to the cube map texture file.
     * @return std::shared_ptr<GL_CubeMapTexture> A shared pointer to the newly created GL_CubeMapTexture object.
     * @note This function is noexcept and guarantees not to throw exceptions.
     */
    std::shared_ptr<GL_CubeMapTexture> GL_CreateUnregisteredCubeMapTexture(const std::string& name, const std::filesystem::path& textureFile) noexcept
    {
        return std::make_shared<GL_CubeMapTexture>(name, textureFile);
    }
}