#include "CorePCH.hpp"

namespace Motion
{
    static inline bool ShouldFlipFor(Motion::TextureType type) noexcept
    {
        using TT = Motion::TextureType;
        switch (type)
        {
        case TT::CubeTexture:
        case TT::IrradianceTexture:
        case TT::PrefilteredTexture:
        case TT::BRDFTexture:
            return false;
        default:
            return true;
        }
    }

    static inline bool IsSRGB(Motion::TextureType type) noexcept
    {
        using TT = Motion::TextureType;
        switch (type)
        {
        case TT::BaseColorTexture:
        case TT::EmissiveTexture:
        case TT::SpecularColorTexture:
            return true;
        default:
            return false;
        }
    }

    GL_Texture::GL_Texture(std::int32_t width, std::int32_t height, const glm::vec3& color)
    {
        if (!GenerateTexture2D(width, height, color))
        {
            MOTION_ASSERT(false, "Unable to generate texture of size {0}x{1}", width, height);
            return;
        }

        m_Specification.Type = TextureType::BaseColorTexture;
        m_Specification.Source = TextureSource::GeneratedTexture;
        m_Specification.TextureFile = "System Generated";
        m_Specification.Name = std::format("System Generated:{} Texture", GetTextureTypeString(m_Specification.Type));
        m_Specification.FlipOnLoadDefault = true;
        m_Specification.InvertGreen = false;
    }

    GL_Texture::GL_Texture(const std::filesystem::path& textureFile, TextureType type)
    {
        m_Specification.Type = type;
        m_Specification.FlipOnLoadDefault = ShouldFlipFor(type);
        m_Specification.InvertGreen = false;

        if (!LoadTextureFromFile(textureFile, m_Specification.FlipOnLoadDefault))
        {
            MOTION_ASSERT(false, "Unable to load texture file {0}", textureFile.string());
            return;
        }

        m_Specification.Source = TextureSource::TextureFile;
        m_Specification.TextureFile = textureFile.string();
        m_Specification.Name = std::format("{} Texture", GetTextureTypeString(m_Specification.Type));
    }

    GL_Texture::GL_Texture(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels)
    {
        m_Specification.Type = type;
        m_Specification.FlipOnLoadDefault = ShouldFlipFor(type);
        m_Specification.InvertGreen = false;

        // select formats (sRGB for color maps)
        const bool useSRGB = IsSRGB(type);
        GLenum internalFormat = GL_RGB8;
        GLenum dataFormat = GL_RGB;

        switch (channels)
        {
        case 1: internalFormat = GL_R8;                                 dataFormat = GL_RED;  break;
        case 2: internalFormat = GL_RG8;                                dataFormat = GL_RG;   break;
        case 3: internalFormat = useSRGB ? GL_SRGB8 : GL_RGB8;          dataFormat = GL_RGB;  break;
        case 4: internalFormat = useSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;  dataFormat = GL_RGBA; break;
        default:
            MOTION_CORE_ERROR("Unsupported channel count: {}", channels);
        }

        m_Specification.Width = width;
        m_Specification.Height = height;
        m_Specification.Channels = channels;
        m_Specification.InternalDataFormat = internalFormat;
        m_Specification.TextureDataFormat = dataFormat;

        // Safe upload (row alignment)
        GLint prevUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_Specification.TexID);
        glBindTexture(GL_TEXTURE_2D, m_Specification.TexID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

        glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpack);

        // Mips & sampler
        glGenerateTextureMipmap(m_Specification.TexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GLfloat maxAniso = 0.0f; glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(4.0f, maxAniso));

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    GL_Texture::~GL_Texture()
    {
        if (m_Specification.TexID)
            glDeleteTextures(1, &m_Specification.TexID);
    }

    void GL_Texture::Bind() const noexcept
    {
        glBindTexture(GL_TEXTURE_2D, m_Specification.TexID);
    }

    void GL_Texture::Bind(std::int32_t bindingPoint) const noexcept
    {
        glBindTextureUnit(bindingPoint, m_Specification.TexID);
    }

    void GL_Texture::Unbind() const noexcept
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    TextureID GL_Texture::GetID() const noexcept
    {
        return m_Specification.TexID;
    }

    TextureSpecification& GL_Texture::GetSpecification() noexcept
    {
        return m_Specification;
    }

    TextureSource GL_Texture::Source() const noexcept
    {
        return m_Specification.Source;
    }

    std::shared_ptr<GL_Texture> GL_Texture::Create(std::int32_t width, std::int32_t height, const glm::vec3& color) noexcept
    {
        return std::make_shared<GL_Texture>(width, height, color);
    }

    std::shared_ptr<GL_Texture> GL_Texture::Create(const std::filesystem::path& textureFile, TextureType type) noexcept
    {
        return std::make_shared<GL_Texture>(textureFile, type);
    }

    std::shared_ptr<GL_Texture> GL_Texture::Create(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels) noexcept
    {
        return std::make_shared<GL_Texture>(data, type, width, height, channels);
    }

    bool GL_Texture::UploadRGBA8(int width, int height, const stbi_uc* data, bool useSRGB)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;
        m_Specification.Channels = 4;

        const GLenum internalFormat = useSRGB ? GL_SRGB8_ALPHA8 : GL_RGBA8;
        const GLenum dataFormat = GL_RGBA;

        m_Specification.InternalDataFormat = internalFormat;
        m_Specification.TextureDataFormat = dataFormat;

        if (m_Specification.TexID == 0)
            glCreateTextures(GL_TEXTURE_2D, 1, &m_Specification.TexID);

        GLint prevUnpack = 4;
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevUnpack);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glBindTexture(GL_TEXTURE_2D, m_Specification.TexID);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);

        glPixelStorei(GL_UNPACK_ALIGNMENT, prevUnpack);

        glGenerateTextureMipmap(m_Specification.TexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        GLfloat maxAniso = 0.0f; glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, std::min(4.0f, maxAniso));

        glBindTexture(GL_TEXTURE_2D, 0);
        return true;
    }

    bool GL_Texture::LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip)
    {
        MOTION_ASSERT(std::filesystem::exists(textureFile), "Texture file not found: {}", textureFile.string());
        stbi_set_flip_vertically_on_load(flip);

        int w = 0, h = 0, n = 0;
        stbi_uc* pixels = stbi_load(textureFile.string().c_str(), &w, &h, &n, STBI_rgb_alpha);
        if (!pixels)
        {
            MOTION_CORE_ERROR("Failed to load texture: {} - {}", textureFile.string(), stbi_failure_reason());
            return false;
        }

        const bool ok = UploadRGBA8(w, h, pixels, IsSRGB(m_Specification.Type));
        stbi_image_free(pixels);

        m_Specification.Source = TextureSource::TextureFile;
        m_Specification.TextureFile = textureFile.string();
        return ok;
    }

    bool GL_Texture::ReloadFromFile(const std::filesystem::path& textureFile, TextureType type, bool flip)
    {
        m_Specification.Type = type;
        m_Specification.FlipOnLoadDefault = flip;

        stbi_set_flip_vertically_on_load(flip);
        int w = 0, h = 0, n = 0;
        stbi_uc* pixels = stbi_load(textureFile.string().c_str(), &w, &h, &n, STBI_rgb_alpha);
        if (!pixels)
        {
            MOTION_CORE_ERROR("Reload failed: {} - {}", textureFile.string(), stbi_failure_reason());
            return false;
        }

        const bool ok = UploadRGBA8(w, h, pixels, IsSRGB(type));
        stbi_image_free(pixels);

        m_Specification.Source = TextureSource::TextureFile;
        m_Specification.TextureFile = textureFile.string();
        m_Specification.Name = std::format("{} Texture", GetTextureTypeString(type));
        return ok;
    }

    bool GL_Texture::GenerateTexture2D(std::int32_t width, std::int32_t height, const glm::vec3& color)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;
        m_Specification.Channels = 4;
        m_Specification.InternalDataFormat = GL_RGBA8;
        m_Specification.TextureDataFormat = GL_RGBA;

        const uint32_t textureSize = width * height * m_Specification.Channels;
        std::uint8_t* textureData = new std::uint8_t[textureSize];
        for (uint32_t i = 0; i < width * height; ++i)
        {
            textureData[i * 4 + 0] = static_cast<uint8_t>(color.r * 255);
            textureData[i * 4 + 1] = static_cast<uint8_t>(color.g * 255);
            textureData[i * 4 + 2] = static_cast<uint8_t>(color.b * 255);
            textureData[i * 4 + 3] = 255;
        }

        const int mipLevels = static_cast<int>(std::floor(std::log2(std::max(width, height)))) + 1;

        glCreateTextures(GL_TEXTURE_2D, 1, &m_Specification.TexID);
        glTextureStorage2D(m_Specification.TexID, mipLevels, m_Specification.InternalDataFormat, width, height);
        glTextureSubImage2D(m_Specification.TexID, 0, 0, 0, width, height, m_Specification.TextureDataFormat, GL_UNSIGNED_BYTE, textureData);

        glTextureParameteri(m_Specification.TexID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_Specification.TexID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_Specification.TexID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_Specification.TexID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glGenerateTextureMipmap(m_Specification.TexID);

        GLfloat maxAniso = 0.0f; glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
        glTextureParameterf(m_Specification.TexID, GL_TEXTURE_MAX_ANISOTROPY, std::min(4.0f, maxAniso));

        delete[] textureData;
        return true;
    }

    GL_CubeTexture::GL_CubeTexture(const std::filesystem::path& textureFile)
    {
        if (!std::filesystem::exists(textureFile))
        {
            MOTION_CORE_ERROR("Cube map texture file {0} does not exist", textureFile.string());
        }

        std::int32_t width, height, channels;
        stbi_set_flip_vertically_on_load(false);
        float* data = stbi_loadf(textureFile.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data)
        {
            MOTION_CORE_ERROR("Failed to load cube map texture file {0}: {1}", textureFile.string(), stbi_failure_reason());
        }

        if (width != height)
        {
            float* resizedTexture{ nullptr };
            std::int32_t resizedWidth = width;
            std::int32_t resizedHeight = width;
            MOTION_CORE_WARN("Cube map texture {0} must have square dimensions, but got {1} x {2} resizing...", textureFile.string(), width, height);
            resizedTexture = stbir_resize_float_linear(data, width, height, 0, 0, resizedWidth, resizedHeight, 0, STBIR_RGBA);

            if (!resizedTexture)
            {
                MOTION_CORE_ERROR("Failed to resize cube map texture {0}: {1}", textureFile.string(), stbi_failure_reason());
                stbi_image_free(data);
            }
            else
            {
                stbi_image_free(data);
                data = nullptr;

                MOTION_CORE_INFO("Cube map texture {0} resized to {1} x {2}", textureFile.string(), resizedWidth, resizedHeight);
                data = resizedTexture;
                width = resizedWidth;
                height = resizedHeight;
            }
        }

        std::int32_t internalDataformat = GL_RGBA32F;
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
        MOTION_CORE_INFO("Cube map texture {0} loaded successfully with ID {1}", textureFile.string(), m_TexID);
    }

    GL_CubeTexture::GL_CubeTexture(
        const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture,
        const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture,
        const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture)
    {
        if (!std::filesystem::exists(posX_texture) || !std::filesystem::exists(negX_texture) ||
            !std::filesystem::exists(posY_texture) || !std::filesystem::exists(negY_texture) ||
            !std::filesystem::exists(posZ_texture) || !std::filesystem::exists(negZ_texture))
        {
            MOTION_CORE_ERROR("One or more cube map texture files do not exist");
            return;
        }

        std::int32_t width, height, channels;
        stbi_set_flip_vertically_on_load(false); // no flip

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
    }

    GL_CubeTexture::~GL_CubeTexture()
    {
        glDeleteTextures(1, &m_TexID);
    }

    void GL_CubeTexture::Bind(std::int32_t bindingPoint) const noexcept
    {
        glBindTextureUnit(bindingPoint, m_TexID);
    }

    void GL_CubeTexture::Bind() const noexcept
    {
        glBindTextureUnit(0, m_TexID);
    }

    void GL_CubeTexture::Unbind() const noexcept
    {
        glBindTextureUnit(0, 0);
    }

    TextureID GL_CubeTexture::GetID() const noexcept
    {
        return m_TexID;
    }

    void GL_CubeTexture::SetFace(std::int32_t face, std::int32_t mipLevel, std::int32_t width, std::int32_t height, std::int32_t format, const void* data)
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

    std::shared_ptr<GL_CubeTexture> GL_CubeTexture::Create(const std::filesystem::path& textureFile) noexcept
    {
        return std::make_shared<GL_CubeTexture>(textureFile);
    }

    std::shared_ptr<GL_CubeTexture> GL_CubeTexture::Create(
        const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture,
        const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture,
        const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) noexcept
    {
        return std::make_shared<GL_CubeTexture>(posX_texture, negX_texture, posY_texture, negY_texture, posZ_texture, negZ_texture);
    }
}
