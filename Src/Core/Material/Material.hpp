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

namespace Motion
{
    struct PhysicalBasedMaterialAttribute
    {
        glm::vec3 BaseColor{ 1.0f, 1.0f, 1.0f };
        float Metallic{ 1.0f };
        float Roughness{ 1.0f };
        float Opacity{ 1.0f };
    };

    struct StandardMaterialAttribute
    {
        glm::vec3 DiffuseColor{ 1.0f, 1.0f, 1.0f };
        glm::vec3 SpecularColor{ 1.0f, 1.0f, 1.0f };
        glm::vec3 AmbientColor{ 1.0f, 1.0f, 1.0f };
        glm::vec3 EmissiveColor{ 0.0f, 0.0f, 0.0f };
        float Shininess{ 32.0f };
        float Opacity{ 1.0f };
    };

    constexpr std::size_t MATERIAL_PBR_ATTRIBUTES_SIZE = sizeof(PhysicalBasedMaterialAttribute);
    constexpr std::size_t MATERIAL_STANDARD_ATTRIBUTES_SIZE = sizeof(StandardMaterialAttribute);

    enum class ShadingMethod
    {
        Standard,
        PhysicalBased,
    };

    struct PhysicalBasedMaterial : public AssetBase<IAsset>
    {
        std::unordered_map<TextureType, std::shared_ptr<ITexture>> Texture{};
        PhysicalBasedMaterialAttribute Attributes{ };

        PhysicalBasedMaterial() = default;
        PhysicalBasedMaterial(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile);
        virtual ~PhysicalBasedMaterial() = default;

        static void Import(const std::filesystem::path& materialYAML);
    };

    class PhysicalBasedMaterialInstance
    {
    public:
        PhysicalBasedMaterialInstance() = default;
        PhysicalBasedMaterialInstance(const std::string& materialID, const std::shared_ptr<PhysicalBasedMaterial>& baseMaterial);
        virtual ~PhysicalBasedMaterialInstance() = default;

        void UploadAttributes();

        std::string GetMaterialID() const noexcept { return m_MaterialID; }
        [[nodiscard]] std::int32_t GetTexturesCount() const noexcept { return Texture.size() + (BaseMaterial ? BaseMaterial->Texture.size() : 0); }

    public:
        std::shared_ptr<PhysicalBasedMaterial> BaseMaterial{ nullptr };
        std::unordered_map<TextureType, std::shared_ptr<ITexture>> Texture{};
        PhysicalBasedMaterialAttribute Attributes{};

    private:
        std::string m_MaterialID{};
        std::shared_ptr<IShaderBuffer> m_UniformBuffer{ nullptr };
    };

    struct StandardMaterial : public AssetBase<IAsset>
    {
        std::unordered_map<TextureType, std::shared_ptr<ITexture>> Texture{};
        StandardMaterialAttribute Attributes{};

        StandardMaterial() = default;
        StandardMaterial(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile);
        virtual ~StandardMaterial() = default;

        static void Import(const std::filesystem::path& materialYAML);
    };

    class StandardMaterialInstance
    {
    public:
        StandardMaterialInstance() = default;
        StandardMaterialInstance(const std::string& materialID, const std::shared_ptr<StandardMaterial>& baseMaterial);
        virtual ~StandardMaterialInstance() = default;

        void UploadAttributes();

        std::string GetMaterialID() const noexcept { return m_MaterialID; }
        [[nodiscard]] std::int32_t GetTexturesCount() const noexcept { return Texture.size() + (BaseMaterial ? BaseMaterial->Texture.size() : 0); }

    public:
        std::shared_ptr<StandardMaterial> BaseMaterial{ nullptr };
        std::unordered_map<TextureType, std::shared_ptr<ITexture>> Texture{};
        StandardMaterialAttribute Attributes{};

    private:
        std::string m_MaterialID{};
        std::shared_ptr<IShaderBuffer> m_UniformBuffer{ nullptr };
    };
}
