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
            bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip = true);
            bool GenerateTexture(uint32_t width, uint32_t height);

        private:
            TextureSpecification m_Specification;
            bool m_FromFile{ false };
    };

    class GL_FrameTexture final : public AssetBase<IFrameTexture>
    {
        public:
            GL_FrameTexture(UUID uuid, const std::string& name, TextureID texID, FrameBufferSpecification& spec);
            GL_FrameTexture(const std::string& name, TextureID texID, FrameBufferSpecification& spec);
            virtual ~GL_FrameTexture() = default;

            virtual void Bind() override;
            virtual void Unbind() override;

            virtual TextureID GetID() const override { return m_TextureID; }
            virtual FrameBufferSpecification& GetFrameSpecification() override { return m_Specification; }

        private:
            TextureID m_TextureID{ 0 };
            FrameBufferSpecification& m_Specification;
    };
}