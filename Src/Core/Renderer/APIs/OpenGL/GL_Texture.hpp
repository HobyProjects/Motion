#pragma once

#include "Texture.hpp"
#include "Asset.hpp"

namespace Motion::Core
{
    class GL_Texture final : public AssetBase<ITexture>
    {
    public:
        GL_Texture(UUID uuid, const std::string& name, std::uint32_t width, std::uint32_t height, const glm::vec3& color);
        GL_Texture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true);
        virtual ~GL_Texture();

        virtual void Bind() const noexcept override;
        virtual void Bind(std::uint32_t bindingPoint) const noexcept override;
        virtual void Unbind() const noexcept override;

        [[nodiscard]] virtual TextureID GetID() const noexcept override;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept override;
        [[nodiscard]] virtual TextureSource Source() const noexcept override;

    protected:
        [[nodiscard]] virtual bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) override;
        [[nodiscard]] virtual bool GenerateTexture2D(std::uint32_t width, std::uint32_t height, const glm::vec3& color) override;

    private:
        TextureSpecification m_Specification{};
        bool m_FromFile{ false };
    };

    class GL_CubeMapTexture final : public AssetBase<ICubeMapTexture>
    {
    public:
        GL_CubeMapTexture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile);
        virtual ~GL_CubeMapTexture();

        virtual void Bind() const noexcept override;
        virtual void Unbind() const noexcept override;

        [[nodiscard]] virtual TextureID GetID() const noexcept override;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept override;

        virtual void SetFace(std::uint32_t face, std::int32_t mipLevel, std::uint32_t width, std::uint32_t height, std::uint32_t format, const void* data) override;

    protected:
        [[nodiscard]] virtual bool LoadCubeMapTexture(const std::filesystem::path& textureFile) override;

    private:
        TextureSpecification m_Specification{};
    };

    [[nodiscard]] std::shared_ptr<GL_Texture> GL_CreateUnregisteredPlainTexture(std::uint32_t width = 100, std::uint32_t height = 100, const glm::vec3& color = { 1.0f, 1.0f, 1.0f }) noexcept;
    [[nodiscard]] std::shared_ptr<GL_Texture> GL_CreateUnregisteredTextureFromFile(const std::filesystem::path& textureFile, TextureType type = TextureType::BaseColorMapsTexture, bool flip = true) noexcept;
    [[nodiscard]] std::shared_ptr<GL_CubeMapTexture> GL_CreateUnregisteredCubeMapTexture(const std::filesystem::path& textureFile) noexcept;
}