#pragma once

#include <filesystem>
#include <memory>
#include <cstdint>
#include <concepts>

#include "Asset.hpp"

namespace Motion::Core
{
    using TextureID = std::uint32_t;

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

    inline std::uint32_t operator|(TextureType a, TextureType b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(TextureType a, TextureType b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }

    struct TextureSpecification
    {
        std::string Name{};
        std::uint8_t* TextureData{nullptr};
        std::int32_t Width{0}, Height{0}, NumberOfChannels{0};
        std::uint32_t InternalDataFormat{0}, TextureDataFormat{0}, TexID{0};
        TextureType Type {TextureType::BaseColorMapsTexture};
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
            [[nodiscard]] virtual TextureSpecification GetSpecification() const noexcept = 0;
            [[nodiscard]] virtual bool IsFromFile() const noexcept = 0;

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
            [[nodiscard]] virtual TextureSpecification GetSpecification() const noexcept = 0;

            virtual void SetFace(std::uint32_t face, std::int32_t mipLevel, std::uint32_t width, std::uint32_t height, std::uint32_t format, const void* data) = 0;

        protected:
            [[nodiscard]] virtual bool LoadCubeMapTextureHDR(const std::filesystem::path& textureFile) = 0;
    };

    template<typename T>
    concept TextureExpected = requires(T texture)
    {
        { texture.Bind() } -> std::same_as<void>;
        { texture.GetID() } -> std::same_as<std::uint32_t>;
        { texture.GetSpecification() } -> std::same_as<TextureSpecification>;
    };
}