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
    struct MaterialAttributes
    {
        glm::vec3 BaseColor{ 1.0f, 1.0f, 1.0f };
        float Metallic{ 0.0f };
        float Roughness{ 1.0f };
        float AmbientOcclusion{ 1.0f };
        float Opacity{ 1.0f };
        float DisplacementScale{ 0.05f };
        float PADDING1{ 0.0f }; // Padding to ensure proper alignment
        float PADDING2{ 0.0f }; // Padding to ensure proper alignment
    };

    constexpr std::size_t MATERIAL_ATTRIBUTES_SIZE = sizeof(MaterialAttributes);

    class Material : public AssetBase<IAsset>
    {
    public:
        Material() = default;
        Material(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile);
        virtual ~Material() = default;

        void Bind();
        void Unbind();

        [[nodiscard]] std::shared_ptr<IShader> GetShader() const noexcept { return Shader; }
        [[nodiscard]] std::shared_ptr<IShaderBuffer> GetUniformBuffer() const noexcept { return UniformBuffer; }
        [[nodiscard]] std::int32_t GetTexturesCount() const noexcept { return Texture.size(); }

        static void ImportMaterial(const std::filesystem::path& materialYAML) noexcept;

    public:
        std::unordered_map<std::string_view, std::shared_ptr<ITexture>> Texture{};
        MaterialAttributes Attributes{ };

    private:
        std::shared_ptr<IShader> Shader{ nullptr };
        std::shared_ptr<IShaderBuffer> UniformBuffer{ nullptr };
    };

    class MaterialInstance
    {
    public:
        MaterialInstance() = default;
        MaterialInstance(const std::string& name, const std::shared_ptr<Material>& baseMaterial);
        virtual ~MaterialInstance() = default;

        void Bind();
        void Unbind();

        std::string GetName() const noexcept { return Name; }
        [[nodiscard]] std::int32_t GetTexturesCount() const noexcept { return Texture.size() + (BaseMaterial ? BaseMaterial->Texture.size() : 0); }

    public:
        std::shared_ptr<Material> BaseMaterial{ nullptr };
        std::unordered_map<std::string_view, std::shared_ptr<ITexture>> Texture{};
        MaterialAttributes Attributes{};

    private:
        std::string Name{ "Unnamed Material Instance" };
        std::shared_ptr<IShader> Shader{ nullptr };
        std::shared_ptr<IShaderBuffer> UniformBuffer{ nullptr };
    };
}
