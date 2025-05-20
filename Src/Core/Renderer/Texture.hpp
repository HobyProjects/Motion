#pragma once

#include <filesystem>
#include <memory>

namespace Motion::Core
{
    using TextureID = uint32_t;

    enum class TextureType
    {
        // Legacy Texture Types
        DiffuseTexture,
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
        TransmissionMapsTexture
    };

    struct TextureSpecification
    {
        uint8_t* TextureData{nullptr};
        int32_t Width{0}, Height{0}, NumberOfChannels{0};
        uint32_t InternalDataFormat{0}, TextureDataFormat{0}, TexID{0};
        TextureType Type {TextureType::BaseColorMapsTexture};
    };

    class ITexture
    {
        public:
            ITexture() = default;
            virtual ~ITexture() = default;

            virtual void Bind() const = 0;
            virtual void Bind(uint32_t bindingPoint) const = 0;
            virtual void Unbind() const = 0;

            virtual TextureID GetID() const = 0;
            virtual TextureSpecification GetSpecification() const = 0;
    };

    class TextureBuilder
    {
        private:
            TextureBuilder() = default;
            ~TextureBuilder() = default;

            TextureBuilder(const TextureBuilder&) = delete;
            TextureBuilder& operator=(const TextureBuilder&) = delete;
            TextureBuilder(const TextureBuilder&&) = delete;
            TextureBuilder& operator=(TextureBuilder&&) = delete;

        public:
            static std::shared_ptr<ITexture> CreatePlainTexture(uint32_t width, uint32_t height);
            static std::shared_ptr<ITexture> CreateTextureFromFile(const std::filesystem::path& filePath, TextureType type, bool flipTexture = true);
    };
}