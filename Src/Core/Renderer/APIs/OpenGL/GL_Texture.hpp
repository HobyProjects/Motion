#pragma once

#include "Texture.hpp"

namespace Motion::Core
{
    class GL_Texture : public ITexture
    {
        public:
            GL_Texture(uint32_t width, uint32_t height);
            GL_Texture(const std::filesystem::path& textureFile, TextureType type, bool flip = true);
            virtual ~GL_Texture();

            virtual void Bind() const override;
            virtual void Bind(uint32_t bindingPoint) const override;
            virtual void Unbind() const override;

            virtual TextureID GetID() const override { return m_Specification.TexID; }
            virtual TextureSpecification GetSpecification() const override { return m_Specification; }

        private:
            bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip = true);
            bool GenerateTexture(uint32_t width, uint32_t height);

        private:
            TextureSpecification m_Specification;
            bool m_FromFile{ false };
    };

    std::shared_ptr<GL_Texture> GL_CreatePlainTexture(uint32_t width, uint32_t height);
    std::shared_ptr<GL_Texture> GL_CreateTextureFromFile(const std::filesystem::path& filePath, TextureType type, bool flipTexture = true);
}