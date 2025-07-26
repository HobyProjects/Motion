#pragma once

#include "Texture.hpp"
#include "Arrays.hpp"
#include "Asset.hpp"

namespace Motion
{
    class GL_Texture final : public AssetBase<ITexture>
    {
    public:
        GL_Texture(UUID uuid, const std::string& name, std::int32_t width, std::int32_t height, const glm::vec3& color);
        GL_Texture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile, TextureType type, bool flip = true);
        virtual ~GL_Texture();

        virtual void Bind() const noexcept override;
        virtual void Bind(std::int32_t bindingPoint) const noexcept override;
        virtual void Unbind() const noexcept override;

        [[nodiscard]] virtual TextureID GetID() const noexcept override;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept override;
        [[nodiscard]] virtual TextureSource Source() const noexcept override;

    protected:
        [[nodiscard]] virtual bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) override;
        [[nodiscard]] virtual bool GenerateTexture2D(std::int32_t width, std::int32_t height, const glm::vec3& color) override;

    private:
        TextureSpecification m_Specification{};
    };

    class GL_CubeMapTexture final : public AssetBase<ICubeTexture>
    {
    public:
        GL_CubeMapTexture(UUID uuid, const std::string& name, const std::filesystem::path& textureFile);
        GL_CubeMapTexture(UUID uuid, const std::string& name,
            const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture,
            const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture,
            const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture);
        virtual ~GL_CubeMapTexture();

        virtual void Bind(std::int32_t bindingPoint = 0) const noexcept override;
        virtual void Bind() const noexcept override;
        virtual void Unbind() const noexcept override;
        virtual void SetFace(std::int32_t face, std::int32_t mipLevel, std::int32_t width, std::int32_t height, std::int32_t format, const void* data) override;

        [[nodiscard]] virtual TextureID GetID() const noexcept override;

    private:
        TextureID m_TexID{ 0 };
    };

    class GL_EnvironmentIrradianceTexture : public EnvironmentIrradianceTexture
    {
    public:
        GL_EnvironmentIrradianceTexture(std::int32_t resolution);
        virtual ~GL_EnvironmentIrradianceTexture();

        virtual void Resize(std::int32_t resolution) override;
        virtual void GenerateIrradiance(std::uint32_t environmentMapID, const std::shared_ptr<ICaptureFrameBuffer>& captureFrameBuffer) override;

        [[nodiscard]] virtual TextureID GetID() const noexcept override { return m_Specification.TexID; }
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept override { return m_Specification; }

    private:
        void RenderCube();

    private:
        TextureSpecification m_Specification{};
        std::shared_ptr<IShader> m_IrradianceShader{ nullptr };

        std::shared_ptr<IVertexArray> m_CubeVAO{ nullptr };
        std::shared_ptr<IVertexBuffer> m_CubeVBO{ nullptr };
        std::shared_ptr<IElementBuffer> m_CubeEBO{ nullptr };
    };

    class GL_EnvironmentPrefilteredTexture : public EnvironmentPrefilteredTexture
    {
    public:
        GL_EnvironmentPrefilteredTexture(std::int32_t resolution);
        virtual ~GL_EnvironmentPrefilteredTexture();

        virtual void Resize(std::int32_t resolution) override;
        virtual void GeneratePrefliteredTexture(std::uint32_t environmentMapID, const std::shared_ptr<ICaptureFrameBuffer>& captureFrameBuffer) override;

        [[nodiscard]] virtual TextureID GetID() const noexcept override { return m_Specification.TexID; }
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept override { return m_Specification; }

    private:
        void RenderCube();

    private:
        TextureSpecification m_Specification{};
        std::shared_ptr<IShader> m_PrefilteredShader{ nullptr };

        std::shared_ptr<IVertexArray> m_CubeVAO{ nullptr };
        std::shared_ptr<IVertexBuffer> m_CubeVBO{ nullptr };
        std::shared_ptr<IElementBuffer> m_CubeEBO{ nullptr };
    };


    [[nodiscard]] std::shared_ptr<GL_Texture> GL_CreateUnregisteredPlainTexture(std::int32_t width = 100, std::int32_t height = 100, const glm::vec3& color = { 1.0f, 1.0f, 1.0f }) noexcept;
    [[nodiscard]] std::shared_ptr<GL_Texture> GL_CreateUnregisteredTextureFromFile(const std::filesystem::path& textureFile, TextureType type = TextureType::BaseColorTexture, bool flip = true) noexcept;
    [[nodiscard]] std::shared_ptr<GL_CubeMapTexture> GL_CreateUnregisteredCubeMapTexture(const std::filesystem::path& textureFile) noexcept;
    [[nodiscard]] std::shared_ptr<GL_CubeMapTexture> GL_CreateUnregisteredCubeMapTexture(const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture, const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture, const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) noexcept;
}