#pragma once

#include "Texture.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class GL_Texture final : public AssetBase<ITexture>
    {
        public:
            GL_Texture(const std::string& name, uint32_t width, uint32_t height);
            GL_Texture(const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true);
            virtual ~GL_Texture();

            virtual void Bind() const override;
            virtual void Bind(uint32_t bindingPoint) const override;
            virtual void Unbind() const override;

            virtual TextureID GetID() const override { return m_Specification.TexID; }
            virtual TextureSpecification GetSpecification() const override { return m_Specification; }
            virtual const AssetMetaData& GetMetaData() const override { return m_MetaData; }
            virtual AssetType GetType() const override { return AssetType::Texture; }

        private:
            bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip = true);
            bool GenerateTexture(uint32_t width, uint32_t height);

        private:
            TextureSpecification m_Specification;
            AssetMetaData m_MetaData;
            bool m_FromFile{ false };
    };
}