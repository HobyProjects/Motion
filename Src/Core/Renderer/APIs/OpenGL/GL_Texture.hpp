#pragma once

#include "Texture.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class GL_Texture final : public AssetBase<ITexture>
    {
        public:
            GL_Texture(const std::string& name, uint32_t width, uint32_t height);
            GL_Texture(UUID uuid, const std::string& name, uint32_t width, uint32_t height);
            GL_Texture(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true);
            GL_Texture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true);
            virtual ~GL_Texture();

            virtual void Bind() const override;
            virtual void Bind(uint32_t bindingPoint) const override;
            virtual void Unbind() const override;

            virtual TextureID GetID() const override { return m_Specification.TexID; }
            virtual TextureSpecification GetSpecification() const override { return m_Specification; }
            virtual bool IsFromFile() const override { return m_FromFile; }

            virtual void SetGlobalAnisotropy(uint32_t level) const override;
            virtual uint32_t GetGlobalAnisotropy() const override { return TextureSpecification::GlobalAnisotropyLevel; }

        private:
            TextureSpecification m_Specification;
            bool m_FromFile{ false };
    };

    class GL_TextureCubeMap final : public AssetBase<ITextureCubeMap>
    {
        public:
            GL_TextureCubeMap(UUID uuid, const std::string& name, uint32_t width, uint32_t height);
            GL_TextureCubeMap(const std::string& name, uint32_t width, uint32_t height);
            GL_TextureCubeMap(UUID uuid, const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true);
            GL_TextureCubeMap(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true);
            virtual ~GL_TextureCubeMap();

            virtual void Bind() const override;
            virtual void Bind(uint32_t bindingPoint) const override;
            virtual void Unbind() const override;

            virtual TextureID GetID() const override { return m_Specification.TexID; }
            virtual TextureSpecification GetSpecification() const override { return m_Specification; }
            virtual bool IsFromFile() const override { return m_FromFile; }

            virtual void SetGlobalAnisotropy(uint32_t level) const override;
            virtual uint32_t GetGlobalAnisotropy() const override { return TextureSpecification::GlobalAnisotropyLevel; }

            virtual void SetFace(uint32_t face, int mipLevel, uint32_t width, uint32_t height, uint32_t format, const void* data) override;

        private:
            TextureSpecification m_Specification;
            bool m_FromFile{ false };
    };
}