#pragma once

#include <filesystem>
#include <memory>
#include <cstdint>
#include <concepts>

#include "Asset.hpp"

namespace Motion::Core
{
    using TextureID = std::uint32_t;
    using FrameTextureID = std::uint32_t;

    enum class TextureType : std::uint32_t
    {
        // Legacy Texture Types
        DiffuseTexture = 0,
        AmbientTexture,
        SpecularTexture,
        EmissiveTexture,
        NormalMapsTexture,
        HeightMaps,
        ShininessTexture,
        OpacityMapsTexture,
        LightMapsTexture,

        // PBR Texture Type
        BaseColorMapsTexture,
        MetallicMapsTexture,
        RoughnessMapsTexture,
        AOMapsTexture,
        EmissiveMapsTexture,
        ClearCoatMapsTexture,
        SheenMapsTexture,
        TransmissionMapsTexture,
        UnknownTextureType,

        // Cube Maps
        CubeMapTexture
    };

    enum class TextureSource : std::uint32_t
    {
        Undefined = 0,
        TextureFile,
        GeneratedTexture,
        CubeMapTextureFile,
    };

    inline std::uint32_t operator|(TextureType a, TextureType b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(TextureType a, TextureType b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }

    struct TextureSpecification
    {
        std::string Name{};
        std::unique_ptr<std::uint8_t[]> TextureData{ nullptr };
        std::int32_t Width{ 0 }, Height{ 0 }, NumberOfChannels{ 0 };
        std::uint32_t InternalDataFormat{ 0 }, TextureDataFormat{ 0 }, TexID{ 0 };
        TextureType Type{ TextureType::BaseColorMapsTexture };
        TextureSource Source{ TextureSource::Undefined };
    };

    class ITexture : public IAsset
    {
    public:
        ITexture() = default;
        virtual ~ITexture() = default;

        virtual void Bind() const noexcept = 0;
        virtual void Bind(std::uint32_t bindingPoint) const noexcept = 0;
        virtual void Unbind() const noexcept = 0;

        [[nodiscard]] virtual TextureID GetID() const noexcept = 0;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept = 0;
        [[nodiscard]] virtual TextureSource Source() const noexcept = 0;

    protected:
        [[nodiscard]] virtual bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) = 0;
        [[nodiscard]] virtual bool GenerateTexture2D(std::uint32_t width, std::uint32_t height, const glm::vec3& color) = 0;
    };


    class ICubeMapTexture : public IAsset
    {
    public:
        ICubeMapTexture() = default;
        virtual ~ICubeMapTexture() = default;

        virtual void Bind() const noexcept = 0;
        virtual void Unbind() const noexcept = 0;

        [[nodiscard]] virtual TextureID GetID() const noexcept = 0;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept = 0;

        virtual void SetFace(std::uint32_t face, std::int32_t mipLevel, std::uint32_t width, std::uint32_t height, std::uint32_t format, const void* data) = 0;

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
        { texture.GetID() } -> std::same_as<std::uint32_t>;
    };

    [[nodiscard]] std::shared_ptr<ITexture> CreateUnregisteredPlainTexture(const std::string& name, std::uint32_t width = 100, std::uint32_t height = 100, const glm::vec3& color = { 1.0f, 1.0f, 1.0f }) noexcept;
    [[nodiscard]] std::shared_ptr<ITexture> CreateUnregisteredTextureFromFile(const std::string& name, const std::filesystem::path& textureFile, TextureType type = TextureType::BaseColorMapsTexture, bool flip = true) noexcept;
    [[nodiscard]] std::shared_ptr<ICubeMapTexture> CreateUnregisteredCubeMapTexture(const std::string& name, const std::filesystem::path& textureFile) noexcept;
}