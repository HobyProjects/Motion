#include "CorePCH.hpp"
#include "GL_Texture.hpp"

namespace Motion
{
    /**
     * Constructs a new GL_Texture object with the specified UUID, name, width, and height. This
     * will generate a plain 2D texture with the specified width and height, and assign it the
     * specified UUID and name. The texture will be of type TextureType::DiffuseTexture.
     *
     * @param[in] uuid   The UUID of the texture
     * @param[in] name   The name of the texture
     * @param[in] width  The width of the texture
     * @param[in] height The height of the texture
     * @param[in] color  The color to fill the texture
     */
    GL_Texture::GL_Texture(UUID uuid, const std::string& name, std::int32_t width, std::int32_t height, const glm::vec3& color)
        : AssetBase<ITexture>(uuid, name, AssetType::Texture, "PlainTexture")
    {
        if (!GenerateTexture2D(width, height, color))
        {
            MOTION_ASSERT(false, "Unable to generate texture of size {0}x{1}", width, height);
            return;
        }

        m_Specification.Type = TextureType::BaseColorTexture;
        m_Specification.Source = TextureSource::GeneratedTexture;
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
    GL_Texture::GL_Texture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip)
        : AssetBase<ITexture>(uuid, name, AssetType::Texture, textureFile.string())
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
     * Destructor for GL_Texture.
     *
     * This destructor cleans up the OpenGL texture resources associated with this texture.
     * It deletes the texture ID if it has been created, ensuring that no memory leaks occur.
     */
    GL_Texture::~GL_Texture()
    {
        if (m_Specification.TexID)
        {
            glDeleteTextures(1, &m_Specification.TexID);
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
    void GL_Texture::Bind(std::int32_t bindingPoint) const noexcept
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
     * Loads a texture from the given file using stb_image library.
     *
     * @param[in] textureFile The path to the texture file to load
     * @param[in] flip        Whether to flip the image vertically
     *
     * @return Whether the texture could be loaded successfully
     */
    bool GL_Texture::LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip)
    {
        MOTION_ASSERT(std::filesystem::exists(textureFile), "Texture file not found: {}", textureFile.string());

        stbi_set_flip_vertically_on_load(flip);
        int width, height, channels;
        stbi_uc* data = stbi_load(textureFile.string().c_str(), &width, &height, &channels, 0);

        if (!data) {
            MOTION_CORE_ERROR("Failed to load texture: {} - {}", textureFile.string(), stbi_failure_reason());
            return false;
        }

        m_Specification.Width = width;
        m_Specification.Height = height;
        m_Specification.Channels = channels;

        // Format logic
        GLenum internalFormat = GL_RGB8;
        GLenum dataFormat = GL_RGB;

        switch (channels) {
        case 1:
            internalFormat = GL_R8;
            dataFormat = GL_RED;
            break;
        case 2:
            internalFormat = GL_RG8;
            dataFormat = GL_RG;
            break;
        case 3:
            internalFormat = GL_RGB8;
            dataFormat = GL_RGB;
            break;
        case 4:
            internalFormat = GL_RGBA8;
            dataFormat = GL_RGBA;
            break;
        default:
            MOTION_CORE_ERROR("Unsupported channel count: {}", channels);
            stbi_image_free(data);
            return false;
        }

        m_Specification.InternalDataFormat = internalFormat;
        m_Specification.TextureDataFormat = dataFormat;

        // Create texture
        glCreateTextures(GL_TEXTURE_2D, 1, &m_Specification.TexID);
        glBindTexture(GL_TEXTURE_2D, m_Specification.TexID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

        // Mipmap calculation based on resolution
        int mipLevels = 1 + static_cast<int>(std::floor(std::log2(std::max(width, height))));
        glGenerateTextureMipmap(m_Specification.TexID);

        // Set filtering and wrapping
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Anisotropy (if supported)
        GLfloat maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(4.0f, maxAniso)); // Cap to 4x for compatibility

        glBindTexture(GL_TEXTURE_2D, 0);
        stbi_image_free(data);

        return true;
    }


    /**
     * Generates a blank 2D texture with the specified width and height
     *
     * @param[in] width  The width of the texture
     * @param[in] height The height of the texture
     * @param[in] color  The color to fill the texture
     *
     * @return Whether the texture could be generated successfully
     */
    bool GL_Texture::GenerateTexture2D(std::int32_t width, std::int32_t height, const glm::vec3& color)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;
        m_Specification.Channels = 4; // RGBA
        m_Specification.InternalDataFormat = GL_RGBA8;
        m_Specification.TextureDataFormat = GL_RGBA;

        uint32_t textureSize = width * height * m_Specification.Channels;
        std::uint8_t* textureData = new std::uint8_t[textureSize];
        for (uint32_t i = 0; i < width * height; ++i) {
            textureData[i * 4 + 0] = static_cast<uint8_t>(color.r * 255);
            textureData[i * 4 + 1] = static_cast<uint8_t>(color.g * 255);
            textureData[i * 4 + 2] = static_cast<uint8_t>(color.b * 255);
            textureData[i * 4 + 3] = 255;
        }

        // Compute mip levels
        int mipLevels = static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;

        glCreateTextures(GL_TEXTURE_2D, 1, &m_Specification.TexID);
        glTextureStorage2D(m_Specification.TexID, mipLevels, m_Specification.InternalDataFormat, width, height);
        glTextureSubImage2D(m_Specification.TexID, 0, 0, 0, width, height, m_Specification.TextureDataFormat, GL_UNSIGNED_BYTE, textureData);

        // Filtering and wrapping
        glTextureParameteri(m_Specification.TexID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_Specification.TexID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_Specification.TexID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_Specification.TexID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Mipmaps
        glGenerateTextureMipmap(m_Specification.TexID);

        // Anisotropy
        GLfloat maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        glTextureParameterf(m_Specification.TexID, GL_TEXTURE_MAX_ANISOTROPY, std::min(4.0f, maxAniso));

        delete[] textureData;
        return true;
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
    GL_CubeMapTexture::GL_CubeMapTexture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile)
        : AssetBase<ICubeTexture>(uuid, name, AssetType::CubeTexture, textureFile.string())
    {
        if (!std::filesystem::exists(textureFile))
        {
            MOTION_CORE_ERROR("Cube map texture file {0} does not exist", textureFile.string());
        }

        std::int32_t width, height, channels;
        stbi_set_flip_vertically_on_load(false); // Cube maps should not be flipped
        float* data = stbi_loadf(textureFile.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data)
        {
            MOTION_CORE_ERROR("Failed to load cube map texture file {0}: {1}", textureFile.string(), stbi_failure_reason());
        }

        if (width != height)
        {
            float* resizedTexture{ nullptr };
            std::int32_t resizedWidth = width;
            std::int32_t resizedHeight = width; // Cube maps must be square

            MOTION_CORE_WARN("Cube map texture {0} must have square dimensions, but got {1} x {2} resizing...", textureFile.string(), width, height);

            resizedTexture = stbir_resize_float_linear(
                data, width, height, 0,
                0, resizedWidth, resizedHeight, 0, STBIR_RGBA
            );

            if (!resizedTexture)
            {
                MOTION_CORE_ERROR("Failed to resize cube map texture {0}: {1}", textureFile.string(), stbi_failure_reason());
                stbi_image_free(data);
            }
            else
            {
                stbi_image_free(data);
                data = nullptr; // Free original data

                MOTION_CORE_INFO("Cube map texture {0} resized to {1} x {2}", textureFile.string(), resizedWidth, resizedHeight);
                data = resizedTexture;
                width = resizedWidth;
                height = resizedHeight;
            }
        }

        std::int32_t internalDataformat = GL_RGBA32F; // High dynamic range format
        std::int32_t textureDataFormat = GL_RGBA;

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_TexID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_TexID);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalDataformat, width, height, 0, textureDataFormat, GL_FLOAT, data);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalDataformat, width, height, 0, textureDataFormat, GL_FLOAT, data);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalDataformat, width, height, 0, textureDataFormat, GL_FLOAT, data);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalDataformat, width, height, 0, textureDataFormat, GL_FLOAT, data);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalDataformat, width, height, 0, textureDataFormat, GL_FLOAT, data);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalDataformat, width, height, 0, textureDataFormat, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        stbi_image_free(data);
        AssetInfo.AssetSource = textureFile.string();
        AssetInfo.IsInitialized = true;


        MOTION_CORE_INFO("Cube map texture {0} loaded successfully with ID {1}", textureFile.string(), m_TexID);
    }


    GL_CubeMapTexture::GL_CubeMapTexture(UUID uuid, const std::string& name, const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture, const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture, const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) :
        AssetBase<ICubeTexture>(uuid, name, AssetType::CubeTexture, "CubeMapTexture")
    {
        if (!std::filesystem::exists(posX_texture) || !std::filesystem::exists(negX_texture) ||
            !std::filesystem::exists(posY_texture) || !std::filesystem::exists(negY_texture) ||
            !std::filesystem::exists(posZ_texture) || !std::filesystem::exists(negZ_texture))
        {
            MOTION_CORE_ERROR("One or more cube map texture files do not exist");
            return;
        }

        std::int32_t width, height, channels;
        stbi_set_flip_vertically_on_load(false); // Cube maps should not be flipped vertically

        float* posX = stbi_loadf(posX_texture.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        float* negX = stbi_loadf(negX_texture.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        float* posY = stbi_loadf(posY_texture.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        float* negY = stbi_loadf(negY_texture.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        float* posZ = stbi_loadf(posZ_texture.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        float* negZ = stbi_loadf(negZ_texture.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!posX || !negX || !posY || !negY || !posZ || !negZ)
        {
            MOTION_CORE_ERROR("Failed to load one or more cube map texture files");
            return;
        }

        std::int32_t internalDataFormat = GL_RGBA32F;
        std::int32_t textureDataFormat = GL_RGBA;

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_TexID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_TexID);

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, internalDataFormat, width, height, 0, textureDataFormat, GL_FLOAT, posX);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, internalDataFormat, width, height, 0, textureDataFormat, GL_FLOAT, negX);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, internalDataFormat, width, height, 0, textureDataFormat, GL_FLOAT, posY);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, internalDataFormat, width, height, 0, textureDataFormat, GL_FLOAT, negY);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, internalDataFormat, width, height, 0, textureDataFormat, GL_FLOAT, posZ);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, internalDataFormat, width, height, 0, textureDataFormat, GL_FLOAT, negZ);

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        stbi_image_free(posX);
        stbi_image_free(negX);
        stbi_image_free(posY);
        stbi_image_free(negY);
        stbi_image_free(posZ);
        stbi_image_free(negZ);

        AssetInfo.AssetSource = "CubeMapTexture";
        AssetInfo.IsInitialized = true;


        MOTION_CORE_INFO("Cube map textures loading success ID {0} -> {1}\n {2}\n {3}\n {4}\n {5}\n {6}\n", m_TexID,
            posX_texture.string(), negX_texture.string(), posY_texture.string(),
            negY_texture.string(), posZ_texture.string(), negZ_texture.string());
    }

    /**
     * @brief Destructor for GL_CubeMapTexture.
     *
     * This destructor cleans up the resources associated with the cube map texture,
     * including deleting the OpenGL texture object and freeing any allocated texture data.
     */
    GL_CubeMapTexture::~GL_CubeMapTexture()
    {
        glDeleteTextures(1, &m_TexID);
    }
    /**
     * @brief Binds the cube map texture to the specified binding point.
     *
     * This function binds the OpenGL cube map texture represented by this object to the given binding point,
     * making it active for subsequent rendering operations.
     *
     * @param bindingPoint The texture unit or binding point to which the texture should be bound.
     */
    void GL_CubeMapTexture::Bind(std::int32_t bindingPoint) const noexcept
    {
        glBindTextureUnit(bindingPoint, m_TexID);
    }

    /**
     * @brief Binds the cube map texture to the current OpenGL context.
     *
     * This method makes the cube map texture specified by m_Specification.TexID
     * active for subsequent OpenGL operations. It is a no-throw operation.
     */
    void GL_CubeMapTexture::Bind() const noexcept
    {
        glBindTextureUnit(0, m_TexID);
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
        glBindTextureUnit(0, 0);
    }

    /**
     * @brief Retrieves the OpenGL texture ID associated with this cube map texture.
     * @return TextureID The unique identifier for the OpenGL texture object.
     * @note This function does not modify any member variables and is guaranteed not to throw exceptions.
     */
    TextureID GL_CubeMapTexture::GetID() const noexcept
    {
        return m_TexID;
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
    void GL_CubeMapTexture::SetFace(std::int32_t face, std::int32_t mipLevel, std::int32_t width, std::int32_t height, std::int32_t format, const void* data)
    {
        if (data != nullptr)
        {
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_TexID);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, mipLevel, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        }
        else
        {
            MOTION_ASSERT(false, "Data pointer is null for setting cube map face {0} at mip level {1}", face, mipLevel);
        }
    }



    // s_CaptureProjection is a static 4x4 projection matrix used for capturing environment maps.
    // It is initialized as a perspective projection with a 90-degree field of view, 
    // an aspect ratio of 1.0, and near/far planes at 0.1 and 10.0 units, respectively.
    // This setup is commonly used for rendering to cubemaps in OpenGL.
    static glm::mat4 s_CaptureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    // s_CaptureViews is an array of 6 view matrices used for cubemap rendering.
    // Each matrix represents a camera view looking in one of the six directions (+X, -X, +Y, -Y, +Z, -Z)
    // from the origin, with appropriate up vectors for each face.
    // These are typically used for environment mapping or rendering to cubemap textures.
    static glm::mat4 s_CaptureViews[6] =
    {
        glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
    };

    // s_SimpleCubeVertices defines the 3D coordinates of the 8 corners of a cube centered at the origin.
    // Each group of three floats represents the (x, y, z) position of a vertex.
    // The vertices are ordered as follows:
    // 0: top-left-back     (-1.0f,  1.0f, -1.0f)
    // 1: bottom-left-back  (-1.0f, -1.0f, -1.0f)
    // 2: bottom-right-back ( 1.0f, -1.0f, -1.0f)
    // 3: top-right-back    ( 1.0f,  1.0f, -1.0f)
    // 4: top-left-front    (-1.0f,  1.0f,  1.0f)
    // 5: bottom-left-front (-1.0f, -1.0f,  1.0f)
    // 6: bottom-right-front( 1.0f, -1.0f,  1.0f)
    // 7: top-right-front   ( 1.0f,  1.0f,  1.0f)
    static std::array<float, 24> s_SimpleCubeVertices =
    {
        -1.0f,  1.0f, -1.0f,    // 0 top-left-back
        -1.0f, -1.0f, -1.0f,    // 1 bottom-left-back
        1.0f, -1.0f, -1.0f,     // 2 bottom-right-back
        1.0f,  1.0f, -1.0f,     // 3 top-right-back
        -1.0f,  1.0f,  1.0f,    // 4 top-left-front
        -1.0f, -1.0f,  1.0f,    // 5 bottom-left-front
        1.0f, -1.0f,  1.0f,     // 6 bottom-right-front
        1.0f,  1.0f,  1.0f      // 7 top-right-front
    };

    /**
     * @brief Indices for rendering a simple cube using triangles.
     *
     * This static array defines the index order for drawing the six faces of a cube,
     * with each face composed of two triangles (totaling 12 triangles, 36 indices).
     * The indices reference the cube's 8 vertices, assumed to be defined elsewhere.
     *
     * Face order and corresponding triangles:
     * - Back face:   0, 1, 2 and 2, 3, 0
     * - Front face:  4, 5, 6 and 6, 7, 4
     * - Left face:   4, 5, 1 and 1, 0, 4
     * - Right face:  3, 2, 6 and 6, 7, 3
     * - Bottom face: 1, 5, 6 and 6, 2, 1
     * - Top face:    4, 0, 3 and 3, 7, 4
     *
     * @note The winding order is counter-clockwise for front-facing triangles.
     */
    static std::array<std::uint32_t, 36> s_SimpleCubeIndices =
    {
        // back face
        0, 1, 2,
        2, 3, 0,

        // front face
        4, 5, 6,
        6, 7, 4,

        // left face
        4, 5, 1,
        1, 0, 4,

        // right face
        3, 2, 6,
        6, 7, 3,

        // bottom face
        1, 5, 6,
        6, 2, 1,

        // top face
        4, 0, 3,
        3, 7, 4
    };


    /**
     * @brief Constructs a GL_EnvironmentIrradianceTexture object with the specified resolution.
     *
     * This constructor initializes the vertex buffer, element buffer, and vertex array for rendering a simple cube.
     * It also sets up the OpenGL texture parameters for an irradiance cube map texture with the given resolution.
     * The shader for generating the irradiance texture is loaded from the asset manager.
     *
     * @param resolution The resolution of the irradiance texture (width and height).
     */
    GL_EnvironmentIrradianceTexture::GL_EnvironmentIrradianceTexture(std::int32_t resolution)
    {
        m_CubeVBO = BufferFactory::CreateVertexBuffer(s_SimpleCubeVertices.data(), static_cast<std::int32_t>(s_SimpleCubeVertices.size()));
        m_CubeEBO = BufferFactory::CreateElementBuffer(s_SimpleCubeIndices.data(), static_cast<std::int32_t>(s_SimpleCubeIndices.size()));
        m_CubeVAO = std::make_shared<GL_VertexArray>();

        BufferLayout layout({ { UniformCache::Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) } });
        m_CubeVBO->SetLayout(layout);
        m_CubeVAO->EmplaceVertexBuffer(m_CubeVBO);
        m_CubeVAO->EmplaceIndexBuffer(m_CubeEBO);

        m_Specification.Width = resolution;
        m_Specification.Height = resolution;
        m_Specification.Type = TextureType::IrradianceTexture;
        m_Specification.Source = TextureSource::GeneratedTexture;
        m_Specification.InternalDataFormat = GL_RGBA16F;
        m_Specification.TextureDataFormat = GL_RGBA;

        glGenTextures(1, &m_Specification.TexID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_Specification.TexID);

        for (std::uint32_t i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_Specification.InternalDataFormat, resolution, resolution, 0, m_Specification.TextureDataFormat, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        auto& assetManager = AssetManager::GetInstance();
        m_IrradianceShader = assetManager.Get<IShader>("EnvironmentIrradiance");
    }

    /**
     * @brief Destructor for GL_IrradianceCubeTexture.
     *
     * This destructor cleans up the OpenGL texture resources associated with this irradiance texture.
     * It deletes the texture ID to ensure no memory leaks occur.
     */
    GL_EnvironmentIrradianceTexture::~GL_EnvironmentIrradianceTexture()
    {
        glDeleteTextures(1, &m_Specification.TexID);
    }


    /**
     * @brief Resizes the irradiance cube texture to the specified resolution.
     *
     * This function checks if the new resolution is different from the current one.
     * If so, it deletes the existing texture, generates a new one with the specified resolution,
     * and sets up the texture parameters accordingly. If the resolution is unchanged, it logs a warning.
     *
     * @param resolution The new resolution for both width and height of the irradiance texture.
     */
    void GL_EnvironmentIrradianceTexture::Resize(std::int32_t resolution)
    {
        if (resolution != m_Specification.Width || resolution != m_Specification.Height)
        {
            m_Specification.Width = resolution;
            m_Specification.Height = resolution;

            glDeleteTextures(1, &m_Specification.TexID);
            m_Specification.TexID = 0; // Reset TexID to ensure it is recreated

            glGenTextures(1, &m_Specification.TexID);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_Specification.TexID);

            for (std::uint32_t i = 0; i < 6; ++i)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_Specification.InternalDataFormat, resolution, resolution, 0, m_Specification.TextureDataFormat, GL_FLOAT, nullptr);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }
        else
        {
            MOTION_CORE_WARN("Irradiance texture resize called with same dimensions: {0}x{1}", resolution, resolution);
            return;
        }
    }

    /**
     * @brief Generates the irradiance texture from the specified environment map ID.
     *
     * This function binds the capture frame buffer, sets up the irradiance shader, and renders the cube faces
     * to generate the irradiance texture. It uses the provided environment map ID as input.
     *
     * @param environmentMapID The OpenGL texture ID of the environment map to use for generating irradiance.
     * @param captureFrameBuffer The frame buffer used for capturing the rendered irradiance texture.
     */
    void GL_EnvironmentIrradianceTexture::GenerateIrradiance(std::uint32_t environmentMapID, const std::shared_ptr<ICaptureFrameBuffer>& captureFrameBuffer)
    {
        m_IrradianceShader->Bind();
        std::int32_t bindingPoint = TextureBinding::Point();
        glBindTextureUnit(bindingPoint, environmentMapID);
        m_IrradianceShader->SetUniform(UniformCache::EnvironmentTexture, bindingPoint);
        m_IrradianceShader->SetUniform(UniformCache::ProjectionMatrix, s_CaptureProjection);

        captureFrameBuffer->ResizeFrame(m_Specification.Width, m_Specification.Height);
        captureFrameBuffer->BindFrameBuffer();
        glViewport(0, 0, m_Specification.Width, m_Specification.Height);

        for (unsigned int i = 0; i < 6; ++i)
        {
            m_IrradianceShader->SetUniform(UniformCache::ViewMatrix, s_CaptureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_Specification.TexID, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderCube();
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        captureFrameBuffer->UnbindFrameBuffer();
        m_IrradianceShader->Unbind();
    }


    /**
     * @brief Renders the cube faces using the bound vertex array object.
     *
     * This function binds the cube vertex array object and issues a draw call to render the cube faces.
     * It uses the element buffer object to determine the indices for rendering.
     */
    void GL_EnvironmentIrradianceTexture::RenderCube()
    {
        m_CubeVAO->Bind();
        glDrawElements(GL_TRIANGLES, m_CubeEBO->GetElementCount(), GL_UNSIGNED_INT, nullptr);
        m_CubeVAO->Unbind();
    }


    /**
     * @brief Constructs a GL_EnvironmentPrefilteredTexture object with the specified resolution.
     *
     * This constructor initializes the prefiltered texture with the given resolution, sets its type to EnvironmentPrefilteredTexture,
     * and generates a cube map texture with the appropriate parameters.
     *
     * @param resolution The resolution for both width and height of the prefiltered texture.
     */
    GL_EnvironmentPrefilteredTexture::GL_EnvironmentPrefilteredTexture(std::int32_t resolution)
    {
        m_CubeVBO = BufferFactory::CreateVertexBuffer(s_SimpleCubeVertices.data(), static_cast<std::int32_t>(s_SimpleCubeVertices.size()));
        m_CubeEBO = BufferFactory::CreateElementBuffer(s_SimpleCubeIndices.data(), static_cast<std::int32_t>(s_SimpleCubeIndices.size()));
        m_CubeVAO = std::make_shared<GL_VertexArray>();

        BufferLayout layout({ { UniformCache::Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) } });
        m_CubeVBO->SetLayout(layout);
        m_CubeVAO->EmplaceVertexBuffer(m_CubeVBO);
        m_CubeVAO->EmplaceIndexBuffer(m_CubeEBO);

        m_Specification.Width = resolution;
        m_Specification.Height = resolution;
        m_Specification.Type = TextureType::IrradianceTexture;
        m_Specification.Source = TextureSource::GeneratedTexture;
        m_Specification.InternalDataFormat = GL_RGBA16F;
        m_Specification.TextureDataFormat = GL_RGBA;

        glGenTextures(1, &m_Specification.TexID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_Specification.TexID);

        for (std::uint32_t i = 0; i < 6; ++i)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_Specification.InternalDataFormat, m_Specification.Width, m_Specification.Height, 0, m_Specification.TextureDataFormat, GL_FLOAT, nullptr);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        auto& assetManager = AssetManager::GetInstance();
        m_PrefilteredShader = assetManager.Get<IShader>("EnvironmentPrefiltered");
    }

    /**
     * @brief Destructor for GL_EnvironmentPrefilteredTexture.
     *
     * This destructor cleans up the OpenGL texture resources associated with this prefiltered texture.
     * It deletes the texture ID to ensure no memory leaks occur.
     */
    GL_EnvironmentPrefilteredTexture::~GL_EnvironmentPrefilteredTexture()
    {
        glDeleteTextures(1, &m_Specification.TexID);
    }


    /**
     * @brief Resizes the prefiltered texture to the specified resolution.
     *
     * This function checks if the new resolution differs from the current one. If it does, it deletes the existing texture,
     * generates a new one with the updated dimensions, and sets the appropriate parameters for cube map textures.
     *
     * @param resolution The new resolution for both width and height of the prefiltered texture.
     */
    void GL_EnvironmentPrefilteredTexture::Resize(std::int32_t resolution)
    {
        if (resolution != m_Specification.Width || resolution != m_Specification.Height)
        {
            m_Specification.Width = resolution;
            m_Specification.Height = resolution;

            glDeleteTextures(1, &m_Specification.TexID);
            m_Specification.TexID = 0; // Reset TexID to ensure it is recreated

            glGenTextures(1, &m_Specification.TexID);
            glBindTexture(GL_TEXTURE_CUBE_MAP, m_Specification.TexID);

            for (std::uint32_t i = 0; i < 6; ++i)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, m_Specification.InternalDataFormat, m_Specification.Width, m_Specification.Height, 0, m_Specification.TextureDataFormat, GL_FLOAT, nullptr);
            }

            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
            glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        }
        else
        {
            MOTION_CORE_WARN("Irradiance texture resize called with same dimensions: {0}x{1}", resolution, resolution);
            return;
        }
    }

    /**
     * @brief Generates the prefiltered texture from the specified environment map ID.
     *
     * This function binds the capture frame buffer, sets up the prefiltered shader, and renders the cube faces
     * to generate the prefiltered texture. It uses the provided environment map ID as input.
     *
     * @param environmentMapID The OpenGL texture ID of the environment map to use for generating the prefiltered texture.
     * @param captureFrameBuffer The frame buffer used for capturing the rendered prefiltered texture.
     */
    void GL_EnvironmentPrefilteredTexture::GeneratePrefliteredTexture(std::uint32_t environmentMapID, const std::shared_ptr<ICaptureFrameBuffer>& captureFrameBuffer)
    {
        m_PrefilteredShader->Bind();
        std::int32_t bindingPoint = TextureBinding::Point();
        glBindTextureUnit(bindingPoint, environmentMapID);
        m_PrefilteredShader->SetUniform(UniformCache::EnvironmentTexture, bindingPoint);
        m_PrefilteredShader->SetUniform(UniformCache::ProjectionMatrix, s_CaptureProjection);

        captureFrameBuffer->ResizeFrame(m_Specification.Width, m_Specification.Height);
        captureFrameBuffer->BindFrameBuffer();

        std::uint32_t maxMipLevels = 5;
        for (std::uint32_t mip = 0; mip < maxMipLevels; mip++)
        {
            std::uint32_t mipWidth = static_cast<std::uint32_t>(m_Specification.Width * std::pow(0.5, mip));
            std::uint32_t mipHeight = static_cast<std::uint32_t>(m_Specification.Height * std::pow(0.5, mip));
            captureFrameBuffer->BindRenderBuffer();
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            m_PrefilteredShader->SetUniform(UniformCache::PrefilteredRoughness, roughness);

            for (std::uint32_t i = 0; i < 6; ++i)
            {
                m_PrefilteredShader->SetUniform(UniformCache::ViewMatrix, s_CaptureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_Specification.TexID, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                RenderCube();
            }
        }

        captureFrameBuffer->UnbindRenderBuffer();
        captureFrameBuffer->UnbindFrameBuffer();
        m_PrefilteredShader->Unbind();
    }

    /**
     * @brief Creates an unregistered plain OpenGL texture.
     *
     * This function constructs a shared pointer to a GL_Texture object with the specified name, width, and height.
     * The created texture is not registered with any texture manager or resource system.
     *
     * @param width The width of the texture in pixels.
     * @param height The height of the texture in pixels.
     * @param color The color to fill the texture.
     * @return std::shared_ptr<GL_Texture> A shared pointer to the newly created GL_Texture object.
     * @note This function is noexcept and guarantees not to throw exceptions.
     */
    std::shared_ptr<GL_Texture> GL_CreateUnregisteredPlainTexture(std::int32_t width, std::int32_t height, const glm::vec3& color) noexcept
    {
        return std::make_shared<GL_Texture>(UniqueIdentity::GetUniqueID(), "PlainTexture", width, height, color);
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
    std::shared_ptr<GL_Texture> GL_CreateUnregisteredTextureFromFile(const std::filesystem::path& textureFile, TextureType type, bool flip) noexcept
    {
        std::string name = textureFile.filename().string();
        return std::make_shared<GL_Texture>(UniqueIdentity::GetUniqueID(), name, textureFile, type, flip);
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
    std::shared_ptr<GL_CubeMapTexture> GL_CreateUnregisteredCubeMapTexture(const std::filesystem::path& textureFile) noexcept
    {
        std::string name = textureFile.filename().string();
        return std::make_shared<GL_CubeMapTexture>(UniqueIdentity::GetUniqueID(), name, textureFile);
    }

    /**
     * @brief Creates an unregistered OpenGL cube map texture from multiple texture files.
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
    std::shared_ptr<GL_CubeMapTexture> GL_CreateUnregisteredCubeMapTexture(const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture, const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture, const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) noexcept
    {
        return std::make_shared<GL_CubeMapTexture>(UniqueIdentity::GetUniqueID(), "CubeMapTexture", posX_texture, negX_texture, posY_texture, negY_texture, posZ_texture, negZ_texture);
    }
}
