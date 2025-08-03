#pragma once

#include "Texture.hpp"
#include "Arrays.hpp"
#include "Buffers.hpp"
#include "Asset.hpp"

namespace Motion
{
    class GL_Texture final : public ITexture
    {
    public:
        GL_Texture(std::int32_t width, std::int32_t height, const glm::vec3& color);
        GL_Texture(const std::filesystem::path& textureFile, TextureType type, bool flip = true);
        GL_Texture(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels);
        virtual ~GL_Texture();

        virtual void Bind() const noexcept override;
        virtual void Bind(std::int32_t bindingPoint) const noexcept override;
        virtual void Unbind() const noexcept override;

        [[nodiscard]] virtual TextureID GetID() const noexcept override;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept override;
        [[nodiscard]] virtual TextureSource Source() const noexcept override;

        [[nodiscard]] static std::shared_ptr<GL_Texture> Create(std::int32_t width = 100, std::int32_t height = 100, const glm::vec3& color = { 1.0f, 1.0f, 1.0f }) noexcept;
        [[nodiscard]] static std::shared_ptr<GL_Texture> Create(const std::filesystem::path& textureFile, TextureType type = TextureType::BaseColorTexture, bool flip = true) noexcept;
        [[nodiscard]] static std::shared_ptr<GL_Texture> Create(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels) noexcept;

    protected:
        [[nodiscard]] virtual bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) override;
        [[nodiscard]] virtual bool GenerateTexture2D(std::int32_t width, std::int32_t height, const glm::vec3& color) override;

    private:
        TextureSpecification m_Specification{};
    };

    class GL_CubeTexture final : public ICubeTexture
    {
    public:
        GL_CubeTexture(const std::filesystem::path& textureFile);
        GL_CubeTexture(
            const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture,
            const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture,
            const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture);

        virtual ~GL_CubeTexture();

        virtual void Bind(std::int32_t bindingPoint = 0) const noexcept override;
        virtual void Bind() const noexcept override;
        virtual void Unbind() const noexcept override;
        virtual void SetFace(std::int32_t face, std::int32_t mipLevel, std::int32_t width, std::int32_t height, std::int32_t format, const void* data) override;

        [[nodiscard]] virtual TextureID GetID() const noexcept override;

        [[nodiscard]] static std::shared_ptr<GL_CubeTexture> Create(const std::filesystem::path& textureFile) noexcept;
        [[nodiscard]] static std::shared_ptr<GL_CubeTexture> Create(
            const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture,
            const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture,
            const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) noexcept;

    private:
        TextureID m_TexID{ 0 };
    };
}