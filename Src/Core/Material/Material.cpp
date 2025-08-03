#include "CorePCH.hpp"

namespace Motion
{
    void Material::ImportMaterial(const std::filesystem::path& materialYAML) noexcept
    {
        if (!std::filesystem::exists(materialYAML)) {
            MOTION_CORE_ERROR("Material file '{}' does not exist!", materialYAML.string());
            return;
        }

        if (materialYAML.extension() != ".yaml" && materialYAML.extension() != ".yml") {
            MOTION_CORE_ERROR("Material file '{}' is not a valid YAML file!", materialYAML.string());
            return;
        }

        try
        {
            YAML::Node root = YAML::LoadFile(std::filesystem::absolute(materialYAML).string());
            YAML::Node materialNode = root["Material"];

            std::string name = materialNode["Name"].as<std::string>();

            auto& assetManager = AssetManager::GetInstance();
            auto material = assetManager.Create<Material>(name, materialYAML);

            // Load parameters
            auto& data = material->Attributes;
            auto params = materialNode["Parameters"];
            if (params) {
                if (params["BaseColor"])
                    data.BaseColor = glm::vec3(params["BaseColor"][0].as<float>(), params["BaseColor"][1].as<float>(), params["BaseColor"][2].as<float>());
                if (params["Metallic"])
                    data.Metallic = params["Metallic"].as<float>();
                if (params["Roughness"])
                    data.Roughness = params["Roughness"].as<float>();
                if (params["AmbientOcclusion"])
                    data.AmbientOcclusion = params["AmbientOcclusion"].as<float>();
                if (params["Opacity"])
                    data.Opacity = params["Opacity"].as<float>();
                if (params["DisplacementScale"])
                    data.DisplacementScale = params["DisplacementScale"].as<float>();
            }

            // Load textures
            auto textures = materialNode["Textures"];
            if (textures)
            {
                auto tryLoad =
                    [&](const std::string& key, TextureType type, const std::string_view uniformName)
                    {
                        if (textures[key])
                        {
                            std::filesystem::path texPath = textures[key].as<std::string>();
                            material->Texture[uniformName] = ITexture::Create(texPath, type, true);
                        }
                    };

                tryLoad("BaseColor", TextureType::BaseColorTexture, UniformCache::BaseColorTextures);
                tryLoad("Normal", TextureType::NormalTexture, UniformCache::NormalTextures);
                tryLoad("Roughness", TextureType::RoughnessTexture, UniformCache::RoughnessTextures);
                tryLoad("AmbientOcclusion", TextureType::RoughnessTexture, UniformCache::AmbientOcclusionTextures);
                tryLoad("Metallic", TextureType::MetallicTexture, UniformCache::MetallicTextures);
                tryLoad("Displacement", TextureType::DisplacementTexture, UniformCache::DisplacementTextures);
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to import material from '{}': {}", materialYAML.filename().string(), e.what());
            return;
        }
    }

    Material::Material(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile) :
        AssetBase<IAsset>(uniqueID, materialName, AssetType::Material, materialFile.string())
    {
        // Initialize shader and uniform buffer
        auto& assetManager = AssetManager::GetInstance();
        Shader = assetManager.Get<IShader>("PBR");
        UniformBuffer = IShaderBuffer::Create(MATERIAL_ATTRIBUTES_SIZE, 0);
    }

    void Material::Bind()
    {
        UniformBuffer->Bind();
        UniformBuffer->SetRawBufferData(MATERIAL_ATTRIBUTES_SIZE, &Attributes);
    }

    void Material::Unbind()
    {
        UniformBuffer->Unbind();
    }

    MaterialInstance::MaterialInstance(const std::shared_ptr<Material>& baseMaterial)
        : BaseMaterial(baseMaterial)
    {
        auto& assetManager = AssetManager::GetInstance();
        Shader = assetManager.Get<IShader>("PBR");
        UniformBuffer = IShaderBuffer::Create(MATERIAL_ATTRIBUTES_SIZE, 0);
    }

    void MaterialInstance::Bind()
    {
        UniformBuffer->Bind();
        UniformBuffer->SetRawBufferData(MATERIAL_ATTRIBUTES_SIZE, &Attributes);
    }

    void MaterialInstance::Unbind()
    {
        UniformBuffer->Unbind();
    }
}

