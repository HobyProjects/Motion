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
        GL_Texture(const std::filesystem::path& textureFile, TextureType type);
        GL_Texture(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels);
        virtual ~GL_Texture();

        void Bind() const noexcept override;
        void Bind(std::int32_t bindingPoint) const noexcept override;
        void Unbind() const noexcept override;
        bool ReloadFromFile(const std::filesystem::path& textureFile, TextureType type, bool flip = true) override;

        [[nodiscard]] TextureID GetID() const noexcept override;
        [[nodiscard]] TextureSpecification& GetSpecification() noexcept override;
        [[nodiscard]] TextureSource Source() const noexcept override;

        [[nodiscard]] static std::shared_ptr<GL_Texture> Create(std::int32_t width, std::int32_t height, const glm::vec3& color) noexcept;
        [[nodiscard]] static std::shared_ptr<GL_Texture> Create(const std::filesystem::path& textureFile, TextureType type) noexcept;
        [[nodiscard]] static std::shared_ptr<GL_Texture> Create(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels) noexcept;


    protected:
        bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) override;
        bool GenerateTexture2D(std::int32_t width, std::int32_t height, const glm::vec3& color) override;

    private:
        bool UploadRGBA8(int width, int height, const stbi_uc* data, bool useSRGB);

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

        void Bind(std::int32_t bindingPoint = 0) const noexcept override;
        void Bind() const noexcept override;
        void Unbind() const noexcept override;
        void SetFace(std::int32_t face, std::int32_t mipLevel, std::int32_t width, std::int32_t height, std::int32_t format, const void* data) override;

        [[nodiscard]] TextureID GetID() const noexcept override;

        [[nodiscard]] static std::shared_ptr<GL_CubeTexture> Create(const std::filesystem::path& textureFile) noexcept;
        [[nodiscard]] static std::shared_ptr<GL_CubeTexture> Create(
            const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture,
            const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture,
            const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) noexcept;

    private:
        TextureID m_TexID{ 0 };
    };
}