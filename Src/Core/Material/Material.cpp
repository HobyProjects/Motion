#include "CorePCH.hpp"
#include "Material.hpp"

namespace Motion
{
    BaseMaterial::BaseMaterial(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile)
        :AssetBase<IAsset>(uniqueID, materialName, AssetType::Material, materialFile.string())
    {
    }

    static bool YAML_GetVec3(const YAML::Node& n, glm::vec3& out)
    {
        if (!n || !n.IsSequence() || n.size() < 3) return false;
        out.x = n[0].as<float>();
        out.y = n[1].as<float>();
        out.z = n[2].as<float>();
        return true;
    }

    static void TryLoadTexture(std::unordered_map<TextureType, std::shared_ptr<ITexture>>& map, const YAML::Node& texturesNode, const char* key, TextureType type)
    {
        if (!texturesNode || !texturesNode[key]) return;
        std::filesystem::path texPath = texturesNode[key].as<std::string>();
        map[type] = ITexture::Create(texPath, type);
    }

    void BaseMaterial::Import(const std::filesystem::path& materialYAML)
    {
        if (!std::filesystem::exists(materialYAML)) {
            MOTION_CORE_ERROR("Physical Based Material file: '{}' does not exist!", materialYAML.string());
            return;
        }
        if (materialYAML.extension() != ".yaml" && materialYAML.extension() != ".yml") {
            MOTION_CORE_ERROR("Physical Based Material file: '{}' is not a valid YAML file!", materialYAML.string());
            return;
        }

        try
        {
            YAML::Node root = YAML::LoadFile(std::filesystem::absolute(materialYAML).string());
            YAML::Node materialNode = root["Material"];
            std::string name = materialNode["Name"].as<std::string>();

            auto& assetManager = AssetManager::GetInstance();
            auto material = assetManager.Create<BaseMaterial>(name, materialYAML);

            if (auto params = materialNode["Parameters"])
            {
                if (params["BaseColor"])      YAML_GetVec3(params["BaseColor"], material->BaseColor);
                if (params["Metallic"])       material->MetallicFactor = params["Metallic"].as<float>();
                if (params["Roughness"])      material->RoughnessFactor = params["Roughness"].as<float>();
                if (params["Opacity"])        material->OpacityFactor = params["Opacity"].as<float>();
            }

            if (auto textures = materialNode["Textures"])
            {
                auto& T = material->Textures;
                TryLoadTexture(T, textures, "BaseColor", TextureType::BaseColorTexture);
                TryLoadTexture(T, textures, "Normal", TextureType::NormalTexture);
                TryLoadTexture(T, textures, "Roughness", TextureType::RoughnessTexture);
                TryLoadTexture(T, textures, "AmbientOcclusion", TextureType::AmbientOcclusionTexture);
                TryLoadTexture(T, textures, "Metallic", TextureType::MetallicTexture);
                TryLoadTexture(T, textures, "Opacity", TextureType::OpacityTexture);
                TryLoadTexture(T, textures, "Emissive", TextureType::EmissiveTexture);
                TryLoadTexture(T, textures, "Displacement", TextureType::DisplacementTexture);
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to import material from '{}': {}", materialYAML.filename().string(), e.what());
            return;
        }
    }

    std::shared_ptr<Material> MaterialBuilder::Create(const std::shared_ptr<BaseMaterial>& baseMaterial)
    {
        std::shared_ptr<Material> material = std::make_shared<Material>(_Registry.create(), baseMaterial);
        material->AddTexture<CorePBR>();
        return material;
    }

    void MaterialBuilder::Destroy(const std::shared_ptr<Material>& material)
    {
        auto handle = material->GetHandle();
        _Registry.destroy(handle);
        material->Destroy();
    }

    void BaseMaterial::SerializeYAML(const std::filesystem::path& outFile) const
    {
        YAML::Emitter out;
        out << YAML::BeginMap;
        out << YAML::Key << "Material" << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "Name" << YAML::Value << outFile.filename().stem().string();

        out << YAML::Key << "Parameters" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "BaseColor" << YAML::Value << YAML::Flow << YAML::BeginSeq << BaseColor.x << BaseColor.y << BaseColor.z << YAML::EndSeq;
        out << YAML::Key << "MetallicFactor" << YAML::Value << MetallicFactor;
        out << YAML::Key << "RoughnessFactor" << YAML::Value << RoughnessFactor;
        out << YAML::Key << "OpacityFactor" << YAML::Value << OpacityFactor;
        out << YAML::EndMap;

        out << YAML::Key << "Textures" << YAML::Value << YAML::BeginMap;
        auto writeTex =
            [&](TextureType t, const char* key)
            {
                auto it = Textures.find(t);
                if (it != Textures.end() && it->second)
                {
                    const auto& spec = it->second->GetSpecification();
                    std::string path = spec.TextureFile.empty() ? fmt::format("<generated:{}:{}x{}>", GetTextureTypeString(t), spec.Width, spec.Height) : spec.TextureFile;
                    out << YAML::Key << key << YAML::Value << path;
                }
            };

        writeTex(TextureType::BaseColorTexture, "BaseColor");
        writeTex(TextureType::MetallicTexture, "Metallic");
        writeTex(TextureType::RoughnessTexture, "Roughness");
        writeTex(TextureType::NormalTexture, "Normal");
        writeTex(TextureType::AmbientOcclusionTexture, "AO");
        writeTex(TextureType::EmissiveTexture, "Emissive");
        writeTex(TextureType::OpacityTexture, "Opacity");
        writeTex(TextureType::ORMTexture, "ORM");
        writeTex(TextureType::DisplacementTexture, "Displacement");
        out << YAML::EndMap;

        out << YAML::EndMap; // Material
        out << YAML::EndMap;

        std::filesystem::create_directories(outFile.parent_path());
        std::ofstream fout(outFile);
        MOTION_ASSERT(fout.good(), "Failed to write material YAML '{}'", outFile.string());
        fout << out.c_str();
    }
}

