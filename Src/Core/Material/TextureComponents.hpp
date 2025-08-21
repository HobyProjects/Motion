#pragma once

#include "Material.hpp"

namespace Motion
{
    // --- Alpha handling (material-level convenience) --------------------------
    enum class AlphaMode : uint8_t { Opaque, Mask, Blend };

    struct AlphaProperties
    {
        AlphaMode Mode{ AlphaMode::Opaque };
        float AlphaCutoff{ 0.5f }; // used only when Mode == Mask
        // Some content exports an Opacity texture; keep factor for scalar control.
        float OpacityFactor{ 1.0f };
        std::shared_ptr<ITexture> OpacityTexture{ nullptr };
    };

    // --- Core PBR Metallic-Roughness -----------------------------------------
    struct CorePBR
    {
        // Base color / albedo (sRGB)
        std::shared_ptr<ITexture> BaseColorTexture{ nullptr };
        glm::vec3 BaseColorFactor{ 1.0f, 1.0f, 1.0f };

        // Metallic/Roughness workflow (linear)
        // Support either separate textures or packed ORM (see PackedMaps).
        std::shared_ptr<ITexture> MetallicTexture{ nullptr };
        std::shared_ptr<ITexture> RoughnessTexture{ nullptr };
        float MetallicFactor{ 0.0f };
        float RoughnessFactor{ 0.5f }; // 0.5 is a sensible neutral

        // Normal map
        std::shared_ptr<ITexture> NormalTexture{ nullptr };
        float NormalScale{ 1.0f }; // glTF normalTexture.scale

        // Ambient Occlusion
        std::shared_ptr<ITexture> OcclusionTexture{ nullptr };
        float OcclusionStrength{ 1.0f }; // glTF occlusionTexture.strength

        // Emissive (sRGB)
        std::shared_ptr<ITexture> EmissiveTexture{ nullptr };
        glm::vec3 EmissiveFactor{ 0.0f, 0.0f, 0.0f };
        float EmissiveStrength{ 1.0f }; // KHR_materials_emissive_strength

        // Optional height/displacement used by your engine (not in glTF core shading)
        std::shared_ptr<ITexture> DisplacementTexture{ nullptr };
        float DisplacementScale{ 0.05f }; // engine-defined
        float DisplacementBias{ -0.025f };  // engine-defined
    };

    // --- Packed maps (optimization path) -------------------------------------
    struct PackedMaps
    {
        // ORM: R=Occlusion, G=Roughness, B=Metallic
        std::shared_ptr<ITexture> ORMTexture{ nullptr };
        // If provided, this overrides separate AO/roughness/metallic textures.
    };

    // --- KHR_materials_clearcoat ---------------------------------------------
    struct ClearcoatExtension
    {
        std::shared_ptr<ITexture> ClearcoatTexture{ nullptr };           // factor
        std::shared_ptr<ITexture> ClearcoatRoughnessTexture{ nullptr };  // roughness
        std::shared_ptr<ITexture> ClearcoatNormalTexture{ nullptr };     // normal
        float ClearcoatFactor{ 0.0f };
        float ClearcoatRoughnessFactor{ 0.0f };
        float ClearcoatNormalScale{ 1.0f };
    };

    // --- KHR_materials_specular ----------------------------------------------
    struct SpecularExtension
    {
        // “Specular level” (amount) and color tint
        std::shared_ptr<ITexture> SpecularTexture{ nullptr };       // level
        std::shared_ptr<ITexture> SpecularColorTexture{ nullptr };  // color
        float SpecularFactor{ 1.0f };               // default 1.0 keeps energy
        glm::vec3 SpecularColorFactor{ 0.04f, 0.04f, 0.04f }; // F0 for dielectrics
    };

    // --- KHR_materials_sheen --------------------------------------------------
    struct SheenExtension
    {
        std::shared_ptr<ITexture> SheenColorTexture{ nullptr };
        std::shared_ptr<ITexture> SheenRoughnessTexture{ nullptr };
        glm::vec3 SheenColorFactor{ 0.0f, 0.0f, 0.0f };
        float SheenRoughnessFactor{ 0.0f };
    };

    // --- KHR_materials_transmission ------------------------------------------
    struct TransmissionExtension
    {
        std::shared_ptr<ITexture> TransmissionTexture{ nullptr };
        float TransmissionFactor{ 0.0f };
    };

    // --- KHR_materials_volume -------------------------------------------------
    struct VolumeExtension
    {
        std::shared_ptr<ITexture> ThicknessTexture{ nullptr };
        float ThicknessFactor{ 0.0f };          // meters, typically [0..1] range from exporters
        float AttenuationDistance{ 0.0f };      // 0 = infinity (no absorption)
        glm::vec3 AttenuationColor{ 1.0f, 1.0f, 1.0f };
    };

    // --- KHR_materials_ior ----------------------------------------------------
    struct IORExtension
    {
        float IOR{ 1.5f };
    };

    // --- KHR_materials_anisotropy --------------------------------------------
    struct AnisotropyExtension
    {
        std::shared_ptr<ITexture> AnisotropyTexture{ nullptr }; // encodes direction & strength
        float AnisotropyStrength{ 0.0f };  // 0 = off
        float AnisotropyRotation{ 0.0f };  // radians in tangent space
    };

    // --- KHR_materials_iridescence -------------------------------------------
    struct IridescenceExtension
    {
        std::shared_ptr<ITexture> IridescenceTexture{ nullptr };          // factor
        std::shared_ptr<ITexture> IridescenceThicknessTexture{ nullptr }; // thickness
        float IridescenceFactor{ 0.0f };
        float IridescenceIor{ 1.3f };
        float IridescenceThicknessMin{ 100.0f };  // nm
        float IridescenceThicknessMax{ 400.0f };  // nm
    };
}