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

        static uint32_t AnisotropyLevel;
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

        virtual void SetGlobalAnisotropy(std::uint32_t level) const noexcept = 0;
        [[nodiscard]] virtual std::uint32_t GetGlobalAnisotropy() const noexcept = 0;

    protected:
        [[nodiscard]] virtual bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) = 0;
        [[nodiscard]] virtual bool GenerateTexture2D(std::uint32_t width, std::uint32_t height) = 0;
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
        [[nodiscard]] virtual bool LoadCubeMapTextureHDR(const std::filesystem::path& textureFile) = 0;
    };

    enum class FrameBufferTextureFormat : std::uint32_t
    {
        None = 0,
        RGBA8,
        RGB8,
        R16F,
        R32F,
        R16I,
        R32I,
        Depth24Stencil8,
        Depth32F,
        Depth24,
        Depth32
    };

    class IFrameTexture : public IAsset
    {
    public:
        IFrameTexture() = default;
        virtual ~IFrameTexture() = default;

        virtual void Bind() const noexcept = 0;
        virtual void Unbind() const noexcept = 0;
        virtual void Reset(FrameTextureID* textureID) noexcept = 0;

        [[nodiscard]] virtual FrameTextureID* GetID() const noexcept = 0;

    };

    template<typename T>
    concept TextureExpected = requires(T texture)
    {
        { texture.Bind() } -> std::same_as<void>;
        { texture.GetID() } -> std::same_as<std::uint32_t>;
    };
}