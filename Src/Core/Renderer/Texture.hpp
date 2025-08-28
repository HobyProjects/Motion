#pragma once

#include <filesystem>
#include <memory>
#include <cstdint>
#include <concepts>

#include "Asset.hpp"
#include "Buffers.hpp"

namespace Motion
{
    using TextureID = std::uint32_t;

    enum class TextureType : std::int32_t
    {
        SpecularTexture,
        NormalTexture,
        EmissiveTexture,
        OpacityTexture,
        BaseColorTexture,
        MetallicTexture,
        RoughnessTexture,
        AmbientOcclusionTexture,
        DisplacementTexture,
        SheenTexture,
        TransmissionTexture,
        ORMTexture,

        ClearcoatTexture,
        ClearcoatRoughnessTexture,
        ClearcoatNormalTexture,
        SpecularColorTexture,
        SheenColorTexture,
        SheenRoughnessTexture,
        ThicknessTexture,
        AnisotropyTexture,
        IridescenceTexture,
        IridescenceThicknessTexture,

        CubeTexture,
        IrradianceTexture,
        PrefilteredTexture,
        BRDFTexture,
        UnknownTexture
    };

    inline TextureType operator|(TextureType a, TextureType b) { return static_cast<TextureType>(static_cast<std::int32_t>(a) | static_cast<std::int32_t>(b)); }
    inline TextureType operator&(TextureType a, TextureType b) { return static_cast<TextureType>(static_cast<std::int32_t>(a) & static_cast<std::int32_t>(b)); }
    inline TextureType operator|=(TextureType& a, TextureType b) { return a = a | b; }
    inline TextureType operator&=(TextureType& a, TextureType b) { return a = a & b; }

    inline std::string GetTextureTypeString(TextureType type)
    {
        switch (type)
        {
            case TextureType::SpecularTexture:              return "Specular";
            case TextureType::NormalTexture:                return "Normal";
            case TextureType::EmissiveTexture:              return "Emissive";
            case TextureType::OpacityTexture:               return "Opacity";
            case TextureType::BaseColorTexture:             return "Base Color";
            case TextureType::MetallicTexture:              return "Metallic";
            case TextureType::RoughnessTexture:             return "Roughness";
            case TextureType::AmbientOcclusionTexture:      return "Ambient Occlusion";
            case TextureType::DisplacementTexture:          return "Displacement";
            case TextureType::SheenTexture:                 return "Sheen";
            case TextureType::TransmissionTexture:          return "Transmission";
            case TextureType::ORMTexture:                   return "ORM (AO/R/M)";
            case TextureType::ClearcoatTexture:             return "Clearcoat";
            case TextureType::ClearcoatRoughnessTexture:    return "Clearcoat Roughness";
            case TextureType::ClearcoatNormalTexture:       return "Clearcoat Normal";
            case TextureType::SpecularColorTexture:         return "Specular Color";
            case TextureType::SheenColorTexture:            return "Sheen Color";
            case TextureType::SheenRoughnessTexture:        return "Sheen Roughness";
            case TextureType::ThicknessTexture:             return "Thickness";
            case TextureType::AnisotropyTexture:            return "Anisotropy";
            case TextureType::IridescenceTexture:           return "Iridescence";
            case TextureType::IridescenceThicknessTexture:  return "Iridescence Thickness";
            case TextureType::CubeTexture:                  return "Cube";
            case TextureType::IrradianceTexture:            return "Irradiance";
            case TextureType::PrefilteredTexture:           return "Prefiltered";
            case TextureType::BRDFTexture:                  return "BRDF";
            default:                                        return "Unknown";
        }
    }

    struct TextureSpecification
    {
        std::string Name{};
        std::string TextureFile{};
        TextureID TexID{ 0 };
        std::int32_t Width{ 0 }, Height{ 0 }, Channels{ 0 };
        std::int32_t InternalDataFormat{ 0 }, TextureDataFormat{ 0 };
        TextureType Type{ TextureType::BaseColorTexture };
        bool FlipOnLoadDefault{ true };
        bool InvertGreen{ false };
    };

    class ITexture
    {
    public:
        ITexture() = default;
        virtual ~ITexture() = default;

        virtual void Bind() const noexcept = 0;
        virtual void Bind(std::int32_t bindingPoint) const noexcept = 0;
        virtual void Unbind() const noexcept = 0;
        virtual bool ReloadFromFile(const std::filesystem::path& textureFile, TextureType type, bool flip = true) = 0;

        [[nodiscard]] virtual TextureID GetID() const noexcept = 0;
        [[nodiscard]] virtual TextureSpecification& GetSpecification() noexcept = 0;

        [[nodiscard]] static std::shared_ptr<ITexture> Create(std::int32_t width, std::int32_t height, const glm::vec3& color) noexcept;
        [[nodiscard]] static std::shared_ptr<ITexture> Create(const std::filesystem::path& textureFile, TextureType type) noexcept;
        [[nodiscard]] static std::shared_ptr<ITexture> Create(std::uint8_t* data, TextureType type, std::int32_t width, std::int32_t height, std::int32_t channels) noexcept;


    protected:
        [[nodiscard]] virtual bool LoadTextureFromFile(const std::filesystem::path& textureFile, bool flip) = 0;
        [[nodiscard]] virtual bool GenerateTexture2D(std::int32_t width, std::int32_t height, const glm::vec3& color) = 0;
    };

    class ICubeMapTexture
    {
    public:
        ICubeMapTexture() = default;
        virtual ~ICubeMapTexture() = default;

        virtual void Bind(std::int32_t bindingPoint = 0) const noexcept = 0;
        virtual void Bind() const noexcept = 0;
        virtual void Unbind() const noexcept = 0;
        virtual void SetFace(std::int32_t face, std::int32_t mipLevel, std::int32_t width, std::int32_t height, std::int32_t format, const void* data) = 0;

        [[nodiscard]] virtual TextureID GetID() const noexcept = 0;

        [[nodiscard]] static std::shared_ptr<ICubeMapTexture> Create(const std::filesystem::path& textureFile) noexcept;
        [[nodiscard]] static std::shared_ptr<ICubeMapTexture> Create(
            const std::filesystem::path& posX_texture, const std::filesystem::path& negX_texture,
            const std::filesystem::path& posY_texture, const std::filesystem::path& negY_texture,
            const std::filesystem::path& posZ_texture, const std::filesystem::path& negZ_texture) noexcept;
    };

    template<typename T>
    concept TextureExpected = requires(T texture)
    {
        { texture.Bind() } -> std::same_as<void>;
        { texture.GetID() } -> std::same_as<std::int32_t>;
    };
}