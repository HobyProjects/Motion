#pragma once

#include "Material.hpp"

namespace Motion
{
    enum class TexturesBitMask : std::uint32_t
    {
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

    inline TexturesBitMask operator|(TexturesBitMask a, TexturesBitMask b) { return static_cast<TexturesBitMask>(static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b)); }
    inline TexturesBitMask operator&(TexturesBitMask a, TexturesBitMask b)  { return static_cast<TexturesBitMask>(static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b)); }
    inline TexturesBitMask& operator|=(TexturesBitMask& a, TexturesBitMask b) { a = a | b; return a; }
    inline TexturesBitMask& operator&=(TexturesBitMask& a, TexturesBitMask b) { a = a & b; return a; }

    enum class AlphaMode : std::uint32_t { Opaque, Mask, Blend };
    enum class AlphaBlendMode : std::uint32_t { Premultiplied, Straight };

    struct AlphaProperties
    {
        AlphaMode      Mode             { AlphaMode::Opaque };
        AlphaBlendMode BlendMode        { AlphaBlendMode::Straight };

        float   AlphaCutoff      { 0.5f };
        float   OpacityFactor    { 1.0f };

        std::shared_ptr<ITexture> OpacityTexture{ nullptr };
    };

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
        float NormalScale{ 1.0f };
        float MetallicFactor{ 0.0f };
        float RoughnessFactor{ 0.5f };
        float OcclusionStrength{ 1.0f }; 
        glm::vec3 EmissiveFactor{ 1.0f, 1.0f, 1.0f };
        float EmissiveStrength{ 1.0f };
        float DisplacementScale{ 0.05f };
        float DisplacementBias{ -0.025f };

        struct AdvancedDisplacement
        {
            float TessellationMin{2.0f};
            float TessellationMax{8.0f};
            float PixelsPerEdge{20.0f};
            float LODNear{5.0f};   
            float LODFar{50.0f};
        };

        AdvancedDisplacement AdvDisplacement;
    };

    struct PackedMaps
    {
        std::shared_ptr<ITexture> ORMTexture{ nullptr };
    };

    struct ResolvedMaterials 
    {
        ITexture* BaseColor     = nullptr;
        ITexture* Normal        = nullptr;
        ITexture* ORM           = nullptr;
        ITexture* Metallic      = nullptr;
        ITexture* Roughness     = nullptr;
        ITexture* AO            = nullptr;
        ITexture* Emissive      = nullptr;
        ITexture* Opacity       = nullptr;
        ITexture* Displacement  = nullptr;

        TexturesBitMask  TMask;
        bool UseORMTextures{false};

        glm::vec4 BaseColorFactor   {1.0f, 1.0f, 1.0f, 1.0f};
        float NormalScale           = 1.0f;
        float MetallicFactor        = 0.0f;   
        float RoughnessFactor       = 0.5f;  
        float AOStrength            = 1.0f;

        glm::vec3 EmissiveFactor    {1.0f, 1.0f, 1.0f};
        float EmissiveStrength      = 1.0f;
        float OpacityFactor         = 1.0f;

        AlphaMode Mode              = AlphaMode::Opaque;
        AlphaBlendMode BlendMode    = AlphaBlendMode::Straight;
        float AlphaCutoff           = 0.5f;

        float DispScale             = 0.05f;
        float DispBias              = -0.025f;

        CorePBR::AdvancedDisplacement AdvDisplacement;
    };

    ResolvedMaterials GetResolvedMaterials(const CorePBR& c, const PackedMaps& p, const AlphaProperties& a);

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