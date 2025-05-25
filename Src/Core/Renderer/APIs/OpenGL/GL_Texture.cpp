#include "CorePCH.hpp"

namespace Motion::Core
{
    GL_Texture::GL_Texture(uint32_t width, uint32_t height) 
    {
        if( !GenerateTexture(width, height) )
        {
            MOTION_ASSERT(false, "Unable to generate texture of size {0}x{1}", width, height);
            return;
        }

        m_Specification.Type = TextureType::BaseColorMapsTexture;
    }

    GL_Texture::GL_Texture(const std::filesystem::path& textureFile, TextureType type, bool flip) 
    {
        if( !LoadTextureFromFile(textureFile, flip) )
        {
            MOTION_ASSERT(false, "Unable to load texture file {0}", textureFile.string());
            return;
        }

        m_Specification.Type = type;
    }

    GL_Texture::~GL_Texture() 
    {
        if( !m_FromFile )
        {
            glDeleteTextures(1, &m_Specification.TexID);
            delete[] m_Specification.TextureData;
            m_Specification.TextureData = nullptr;
        }

        if( m_FromFile )
        {
            glDeleteTextures(1, &m_Specification.TexID);
            SOIL_free_image_data(m_Specification.TextureData);
            m_Specification.TexID = 0;
        }
    }
    void GL_Texture::Bind() const 
    {
        glBindTexture(GL_TEXTURE_2D, m_Specification.TexID);
    }

    void GL_Texture::Bind(uint32_t bindingPoint) const 
    {
        glBindTextureUnit(bindingPoint, m_Specification.TexID);
    }

    void GL_Texture::Unbind() const 
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    bool GL_Texture::LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) 
    {
        MOTION_ASSERT(std::filesystem::exists(textureFile), "Unable to load texture file {0}", textureFile.string());
        int32_t loaderFlags = ( flip ) ? ( SOIL_LOAD_RGBA | SOIL_FLAG_INVERT_Y | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_COMPRESS_TO_DXT ) : ( SOIL_LOAD_RGBA | SOIL_FLAG_DDS_LOAD_DIRECT | SOIL_FLAG_MULTIPLY_ALPHA | SOIL_FLAG_COMPRESS_TO_DXT );
        m_Specification.TexID = SOIL_load_OGL_texture(textureFile.string().c_str(), SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, loaderFlags);
        MOTION_ASSERT(m_Specification.TexID, "Unable to load texture file {0}; {1}", textureFile.string(), SOIL_last_result());

        m_FromFile = true;
        if( m_Specification.TextureData )
            return true;

        return false;
    }

    
    bool GL_Texture::GenerateTexture(uint32_t width, uint32_t height)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;
        m_Specification.NumberOfChannels = GL_RGBA;
        m_Specification.InternalDataFormat = GL_RGBA8;
        m_Specification.TextureDataFormat = GL_RGBA;

        uint32_t textureAllocateSize = m_Specification.Width * m_Specification.Height * m_Specification.NumberOfChannels;
        m_Specification.TextureData = new uint8_t [textureAllocateSize];
        std::memset(m_Specification.TextureData, 255, textureAllocateSize);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_Specification.TexID);
        glBindTexture(GL_TEXTURE_2D, m_Specification.TexID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, m_Specification.InternalDataFormat, m_Specification.Width, m_Specification.Height, 0, m_Specification.TextureDataFormat, GL_UNSIGNED_BYTE, m_Specification.TextureData);
        glGenerateMipmap(GL_TEXTURE_2D);

        return true;
    }

    std::shared_ptr<GL_Texture> GL_CreatePlainTexture(uint32_t width, uint32_t height)
    {
        return std::make_shared<GL_Texture>(width, height);
    }

    std::shared_ptr<GL_Texture> GL_CreateTextureFromFile(const std::filesystem::path & filePath, TextureType type, bool flipTexture)
    {
        return std::make_shared<GL_Texture>(filePath, type, flipTexture);
    }
}

