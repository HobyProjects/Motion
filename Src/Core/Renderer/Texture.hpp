#pragma once

#include <filesystem>
#include <memory>
#include "Asset.hpp"

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

    class ITexture : public IAsset
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
}