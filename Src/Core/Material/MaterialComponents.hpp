#pragma once

#include "Material.hpp"

namespace Motion
{
    struct TextureSlot
    {
        static constexpr std::int32_t Irradiance                 = 1;
        static constexpr std::int32_t Prefilter                  = 2;
        static constexpr std::int32_t BRDFLUT                    = 3;
        static constexpr std::int32_t Skybox                     = 4;
        
        static constexpr std::int32_t Base                       = 8;
        static constexpr std::int32_t BaseColor                  = Base + 0;
        static constexpr std::int32_t Metallic                   = Base + 1;
        static constexpr std::int32_t Roughness                  = Base + 2;
        static constexpr std::int32_t Normal                     = Base + 3;
        static constexpr std::int32_t AO                         = Base + 4;
        static constexpr std::int32_t Emissive                   = Base + 5;
        static constexpr std::int32_t Opacity                    = Base + 6;
        static constexpr std::int32_t ORM                        = Base + 7;
        static constexpr std::int32_t Clearcoat                  = Base + 8;
        static constexpr std::int32_t ClearcoatR                 = Base + 9;
        static constexpr std::int32_t SpecularColor              = Base + 10;
        static constexpr std::int32_t Specular                   = Base + 11;
        static constexpr std::int32_t SheenColor                 = Base + 12;
        static constexpr std::int32_t SheenR                     = Base + 13;
        static constexpr std::int32_t Transmission               = Base + 14;
        static constexpr std::int32_t Thickness                  = Base + 15;
        static constexpr std::int32_t ClearcoatN                 = Base + 16;
        static constexpr std::int32_t Displacement               = Base + 17;
        static constexpr std::int32_t Anisotropy                 = Base + 18;
        static constexpr std::int32_t Iridescence                = Base + 19;
        static constexpr std::int32_t IridescenceThickness       = Base + 20;
    };

    enum class TexturesBitMask : std::uint32_t
    {
        None               = 0,
        HasBaseColor       = MOTION_BIT(0),
        HasNormal          = MOTION_BIT(1),
        HasMetallic        = MOTION_BIT(2),
        HasRoughness       = MOTION_BIT(3),
        HasOcclusion       = MOTION_BIT(4),
        HasEmissive        = MOTION_BIT(5),
        HasDisplacement    = MOTION_BIT(6),
        HasOpacity         = MOTION_BIT(7),
        HasORM             = MOTION_BIT(8)
    };

    template <>
    struct enable_bitmask_operations<TexturesBitMask> : std::true_type {};

    struct CorePBR
    {
        std::shared_ptr<ITexture> BaseColorTexture{ nullptr };
        std::shared_ptr<ITexture> NormalTexture{ nullptr };
        std::shared_ptr<ITexture> MetallicTexture{ nullptr };
        std::shared_ptr<ITexture> RoughnessTexture{ nullptr };
        std::shared_ptr<ITexture> OcclusionTexture{ nullptr };
        std::shared_ptr<ITexture> EmissiveTexture{ nullptr };
        std::shared_ptr<ITexture> DisplacementTexture{ nullptr };

        glm::vec4 BaseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec3 EmissiveFactor{ 0.0f, 0.0f, 0.0f };

        float NormalScale{ 1.0f };
        float MetallicFactor{ 0.0f };
        float RoughnessFactor{ 0.5f };
        float OcclusionStrength{ 1.0f }; 
        float EmissiveStrength{ 1.0f };
        float OpacityFactor{ 1.0f };
    };

    struct PackedMaps
    {
        std::shared_ptr<ITexture> ORMTexture{ nullptr };
    };

    struct ResolvedMaterials 
    {
        std::weak_ptr<ITexture> BaseColor;
        std::weak_ptr<ITexture> Normal;
        std::weak_ptr<ITexture> ORM;
        std::weak_ptr<ITexture> Metallic;
        std::weak_ptr<ITexture> Roughness;
        std::weak_ptr<ITexture> AO;
        std::weak_ptr<ITexture> Emissive;
        std::weak_ptr<ITexture> Opacity;

        glm::vec4 BaseColorFactor   { 1.0f, 1.0f, 1.0f, 1.0f };
        glm::vec3 EmissiveFactor    { 0.0f, 0.0f, 0.0f };

        float NormalScale           = 1.0f;
        float MetallicFactor        = 0.0f;   
        float RoughnessFactor       = 0.5f;  
        float AOStrength            = 1.0f;
        float OpacityFactor         = 1.0f;
        float EmissiveStrength      = 1.0f;

        TexturesBitMask TMask{ TexturesBitMask::None };
    };

    ResolvedMaterials GetResolvedMaterials(Material* material);

    #if 0
    struct ClearcoatExtension
    {
        std::shared_ptr<ITexture> ClearcoatTexture{ nullptr };           
        std::shared_ptr<ITexture> ClearcoatRoughnessTexture{ nullptr };  
        std::shared_ptr<ITexture> ClearcoatNormalTexture{ nullptr };   
        float ClearcoatFactor{ 0.0f };
        float ClearcoatRoughnessFactor{ 0.0f };
        float ClearcoatNormalScale{ 1.0f };
    };
    struct SpecularExtension
    {     
        std::shared_ptr<ITexture> SpecularTexture{ nullptr };       
        std::shared_ptr<ITexture> SpecularColorTexture{ nullptr };  
        float SpecularFactor{ 1.0f };        
        glm::vec3 SpecularColorFactor{ 0.04f, 0.04f, 0.04f }; 
    };
    struct SheenExtension
    {
        std::shared_ptr<ITexture> SheenColorTexture{ nullptr };
        std::shared_ptr<ITexture> SheenRoughnessTexture{ nullptr };
        glm::vec3 SheenColorFactor{ 0.0f, 0.0f, 0.0f };
        float SheenRoughnessFactor{ 0.0f };
    };
    struct TransmissionExtension
    {
        std::shared_ptr<ITexture> TransmissionTexture{ nullptr };
        float TransmissionFactor{ 0.0f };
    };
    struct VolumeExtension
    {
        std::shared_ptr<ITexture> ThicknessTexture{ nullptr };
        float ThicknessFactor{ 0.0f };          
        float AttenuationDistance{ 0.0f };     
        glm::vec3 AttenuationColor{ 1.0f, 1.0f, 1.0f };
        float IOR{ 1.5f };
    };
    struct AnisotropyExtension
    {
        std::shared_ptr<ITexture> AnisotropyTexture{ nullptr }; 
        float AnisotropyStrength{ 0.0f };  
        float AnisotropyRotation{ 0.0f };
    };
    struct IridescenceExtension
    {
        std::shared_ptr<ITexture> IridescenceTexture{ nullptr };         
        std::shared_ptr<ITexture> IridescenceThicknessTexture{ nullptr };
        float IridescenceFactor{ 0.0f };
        float IridescenceIor{ 1.3f };
        float IridescenceThicknessMin{ 100.0f };  
        float IridescenceThicknessMax{ 400.0f }; 
    };
    #endif
}