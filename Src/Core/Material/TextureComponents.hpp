#pragma once

#include "Material.hpp"

namespace Motion
{
    struct CoreTextures
    {
        std::shared_ptr<ITexture> AlbedoTexture{ nullptr };
        std::shared_ptr<ITexture> MetallicTexture{ nullptr };
        std::shared_ptr<ITexture> RoughnessTexture{ nullptr };
        std::shared_ptr<ITexture> NormalMapTexture{ nullptr };
        std::shared_ptr<ITexture> AmbientOcclusionTexture{ nullptr };
        std::shared_ptr<ITexture> DisplacementTexture{ nullptr };
        std::shared_ptr<ITexture> EmissiveTexture{ nullptr };
        std::shared_ptr<ITexture> OpacityTexture{ nullptr };

        glm::vec3 BaseColor{ 1.0f, 1.0f, 1.0f };
        float MetallicFactor{ 0.0f };
        float RoughnessFactor{ 0.0f };
        float Opacity{ 1.0f };
    };

    struct ExtendedTextures
    {
        std::shared_ptr<ITexture> ClearcoatTexture{ nullptr };
        std::shared_ptr<ITexture> ClearcoatRoughnessTexture{ nullptr };
        std::shared_ptr<ITexture> SpecularTexture{ nullptr };
        std::shared_ptr<ITexture> SpecularColorTexture{ nullptr };

        float ClearcoatFactor{ 0.0f };
        float ClearcoatRoughnessFactor{ 0.1f };

        float SpecularLevel{ 1.0f };
        glm::vec3 SpecularColor{ 0.04f, 0.04f, 0.04f };
    };

    struct PackedTextures
    {
        std::shared_ptr<ITexture> ORMTexture{ nullptr }; // Ambient Occlusion, Roughness, Metallic
    };

    struct SheenFabricTextures
    {
        std::shared_ptr<ITexture> SheenTexture{ nullptr };
        std::shared_ptr<ITexture> SheenRoughnessTexture{ nullptr };

        glm::vec3 SheenColor{ 0.0f, 0.0f, 0.0f };
        float SheenRoughness{ 0.5f };
    };

    struct TransmissionSubsurfaceTextures
    {
        std::shared_ptr<ITexture> TransmissionTexture{ nullptr };
        std::shared_ptr<ITexture> ThicknessTexture{ nullptr };

        float Transmission{ 0.0f };
        float Thickness{ 0.0f };

        glm::vec3 AttenuationColor{ 1.0f, 1.0f, 1.0f };
        float AttenuationDistance{ 1.0f };
        float IOR{ 1.5f };
    };
}