#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include "Texture.hpp"
#include "Shaders.hpp"
#include "Buffers.hpp"
#include "Asset.hpp"

#define MAX_MATERIAL_LAYERS 4


namespace Motion
{
    using MaterialTexture = std::shared_ptr<ITexture>;

    struct MaterialDefaultValues
    {
        static constexpr glm::vec3 EmissiveColor = { 0.0f, 0.0f, 0.0f };
        static constexpr glm::vec3 BaseColor = { 1.0f, 1.0f, 1.0f };
        static constexpr float Metallic = 0.0f;
        static constexpr float Roughness = 1.0f;
        static constexpr float Opacity = 1.0f;
        static constexpr float AmbientOcclusion = 1.0f;
        static constexpr float ClearCoat = 0.0f;
        static constexpr float ClearCoatRoughness = 0.0f;
        static constexpr float Sheen = 0.0f;
        static constexpr float SheenRoughness = 0.5f;
        static constexpr float Transmission = 0.0f;
        static constexpr float IOR = 1.5f;
        static constexpr float Blend = 1.0f;
    };

    struct MaterialLayerData
    {
        glm::vec3 BaseColor{ MaterialDefaultValues::BaseColor };
        float Metallic{ MaterialDefaultValues::Metallic };
        float Roughness{ MaterialDefaultValues::Roughness };
        float Opacity{ MaterialDefaultValues::Opacity };
        float AmbientOcclusion{ MaterialDefaultValues::AmbientOcclusion };
        float ClearCoat{ MaterialDefaultValues::ClearCoat };
        float ClearCoatRoughness{ MaterialDefaultValues::ClearCoatRoughness };
        float Sheen{ MaterialDefaultValues::Sheen };
        float SheenRoughness{ MaterialDefaultValues::SheenRoughness };
        float Transmission{ MaterialDefaultValues::Transmission };
        float IOR{ MaterialDefaultValues::IOR };
        float Blend{ MaterialDefaultValues::Blend };
    };

    struct MaterialLayer
    {
        MaterialLayerData Data{};
        std::unordered_map<std::string_view, std::shared_ptr<ITexture>> Textures;
    };

    class Material : public AssetBase<IAsset>
    {
    public:
        Material() = default;
        Material(const UUID& uuid, const std::string& name);
        virtual ~Material() = default;

        void Bind() noexcept;
        void Unbind() noexcept;

        [[nodiscard]] MaterialLayer& GetLayer(std::uint32_t index) noexcept;

        void InsertLayerData(std::uint32_t layerIndex, const MaterialLayerData& data) noexcept;
        void InsertLayerTexture(std::uint32_t layerIndex, std::string_view textureName, const std::shared_ptr<ITexture>& texture) noexcept;

    private:
        std::array<MaterialLayer, MAX_MATERIAL_LAYERS> m_Layers{};
        std::shared_ptr<IShaderBuffer> m_ShaderBuffer{ nullptr };
        std::shared_ptr<IShader> m_Shader{ nullptr };

    public:
        glm::vec3 EmissiveColor{ MaterialDefaultValues::EmissiveColor };
        std::shared_ptr<ITexture> EmissiveTexture{ nullptr };
    };
}
