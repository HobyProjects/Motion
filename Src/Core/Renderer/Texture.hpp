#pragma once

#include <filesystem>
#include <memory>
#include <cstdint>
#include <concepts>

#include "Asset.hpp"

namespace Motion
{
    using TextureID = std::uint32_t;

    enum class TextureType : std::int32_t
    {
        BaseColorTexture,
        MetallicTexture,
        RoughnessTexture,
        AmbientOcclusionTexture,
        EmissiveTexture,
        ClearCoatTexture,
        SheenTexture,
        TransmissionTexture,
        NormalTexture,
        OpacityTexture,

        CubeMapTexture,
        UnknownTexture
    };

    enum class TextureSource : std::int32_t
    {
        Undefined = 0,
        TextureFile,
        GeneratedTexture,
        CubeMapTextureFile,
    };

    inline TextureType operator|(TextureType a, TextureType b) { return static_cast<TextureType>(static_cast<std::int32_t>(a) | static_cast<std::int32_t>(b)); }
    inline TextureType operator&(TextureType a, TextureType b) { return static_cast<TextureType>(static_cast<std::int32_t>(a) & static_cast<std::int32_t>(b)); }
    inline TextureType operator|=(TextureType& a, TextureType b) { return a = a | b; }
    inline TextureType operator&=(TextureType& a, TextureType b) { return a = a & b; }

    struct TextureSpecification
    {
        TextureID TexID{ 0 };
        std::int32_t Width{ 0 }, Height{ 0 }, Channels{ 0 };
        std::int32_t InternalDataFormat{ 0 }, TextureDataFormat{ 0 };
        TextureType Type{ TextureType::BaseColorTexture };
        TextureSource Source{ TextureSource::Undefined };
    };

    class ITexture : public IAsset
    {
    public:
        ITexture() = default;
        virtual ~ITexture() = default;

        virtual void Bind() const noexcept = 0;
        virtual void Bind(std::int32_t bindingPoint) const noexcept = 0;
        virtual void Unbind() const noexcept = 0;

        [[nodiscard]] virtual TextureID GetID() const noexcept = 0;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept = 0;
        [[nodiscard]] virtual TextureSource Source() const noexcept = 0;

    protected:
        [[nodiscard]] virtual bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) = 0;
        [[nodiscard]] virtual bool GenerateTexture2D(std::int32_t width, std::int32_t height, const glm::vec3& color) = 0;
    };

    class TextureBinding
    {
    private:
        TextureBinding() = default;
        ~TextureBinding() = default;

        TextureBinding(const TextureBinding&) = delete;
        TextureBinding& operator=(const TextureBinding&) = delete;
        TextureBinding(TextureBinding&&) = delete;
        TextureBinding& operator=(TextureBinding&&) = delete;

    public:
        [[nodiscard]] static std::int32_t Point() noexcept;
        static void Reset() noexcept;
    };


    class ICubeMapTexture : public IAsset
    {
    public:
        ICubeMapTexture() = default;
        virtual ~ICubeMapTexture() = default;

        virtual void Bind(std::int32_t bindingPoint = 0) const noexcept = 0;
        virtual void Bind() const noexcept = 0;
        virtual void Unbind() const noexcept = 0;

        [[nodiscard]] virtual TextureID GetID() const noexcept = 0;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept = 0;

        virtual void SetFace(std::int32_t face, std::int32_t mipLevel, std::int32_t width, std::int32_t height, std::int32_t format, const void* data) = 0;

    protected:
        [[nodiscard]] virtual bool LoadCubeMapTexture(const std::filesystem::path& textureFile) = 0;
    };

    /**
     * @brief Concept to check if a type is a valid texture.
     *
     * This concept checks if the type T has the methods Bind() and GetID() with the expected return types.
     */
    template<typename T>
    concept TextureExpected = requires(T texture)
    {
        { texture.Bind() } -> std::same_as<void>;
        { texture.GetID() } -> std::same_as<std::int32_t>;
    };

    [[nodiscard]] std::shared_ptr<ITexture> CreateUnregisteredPlainTexture(std::int32_t width = 100, std::int32_t height = 100, const glm::vec3& color = { 1.0f, 1.0f, 1.0f }) noexcept;
    [[nodiscard]] std::shared_ptr<ITexture> CreateUnregisteredTextureFromFile(const std::filesystem::path& textureFile, TextureType type = TextureType::BaseColorTexture, bool flip = true) noexcept;
    [[nodiscard]] std::shared_ptr<ICubeMapTexture> CreateUnregisteredCubeMapTexture(const std::filesystem::path& textureFile) noexcept;
}