#include "CorePCH.hpp"
#include "Material.hpp"

namespace Motion
{

    PhysicalBasedMaterial::PhysicalBasedMaterial(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile) :
        AssetBase<IAsset>(uniqueID, materialName, AssetType::Material, materialFile.string()) {
    }

    PhysicalBasedMaterialInstance::PhysicalBasedMaterialInstance(const std::string& materialID, const std::shared_ptr<PhysicalBasedMaterial>& baseMaterial)
        : BaseMaterial(baseMaterial), m_MaterialID(materialID), m_UniformBuffer(IShaderBuffer::Create(MATERIAL_PBR_ATTRIBUTES_SIZE, 0)) {
    }

    void PhysicalBasedMaterialInstance::UploadAttributes()
    {
        m_UniformBuffer->Bind();
        m_UniformBuffer->SetRawBufferData(MATERIAL_PBR_ATTRIBUTES_SIZE, &Attributes);
    }

    StandardMaterial::StandardMaterial(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile) :
        AssetBase<IAsset>(uniqueID, materialName, AssetType::Material, materialFile.string()) {
    }

    StandardMaterialInstance::StandardMaterialInstance(const std::string& materialID, const std::shared_ptr<StandardMaterial>& baseMaterial)
        : BaseMaterial(baseMaterial), m_MaterialID(materialID), m_UniformBuffer(IShaderBuffer::Create(MATERIAL_STANDARD_ATTRIBUTES_SIZE, 1)) {
    }

    void StandardMaterialInstance::UploadAttributes()
    {
        m_UniformBuffer->Bind();
        m_UniformBuffer->SetRawBufferData(MATERIAL_STANDARD_ATTRIBUTES_SIZE, &Attributes);
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
        map[type] = ITexture::Create(texPath, type, true);
    }

    void PhysicalBasedMaterial::Import(const std::filesystem::path& materialYAML)
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
            auto material = assetManager.Create<PhysicalBasedMaterial>(name, materialYAML);

            // Parameters
            auto& A = material->Attributes;
            if (auto params = materialNode["Parameters"])
            {
                if (params["BaseColor"])      YAML_GetVec3(params["BaseColor"], A.BaseColor);
                if (params["Metallic"])       A.Metallic = params["Metallic"].as<float>();
                if (params["Roughness"])      A.Roughness = params["Roughness"].as<float>();
                if (params["Opacity"])        A.Opacity = params["Opacity"].as<float>();

                // Extended
                // if (params["ClearcoatFactor"])      A.ClearcoatFactor = params["ClearcoatFactor"].as<float>();
                // if (params["ClearcoatRoughness"])   A.ClearcoatRoughness = params["ClearcoatRoughness"].as<float>();
                // if (params["SpecularColor"])        YAML_GetVec3(params["SpecularColor"], A.SpecularColor);
                // if (params["SpecularLevel"])        A.SpecularLevel = params["SpecularLevel"].as<float>();
                // if (params["SheenColor"])           YAML_GetVec3(params["SheenColor"], A.SheenColor);
                // if (params["SheenRoughness"])       A.SheenRoughness = params["SheenRoughness"].as<float>();
                // if (params["Transmission"])         A.Transmission = params["Transmission"].as<float>();
                // if (params["Thickness"])            A.Thickness = params["Thickness"].as<float>();
                // if (params["AttenuationColor"])     YAML_GetVec3(params["AttenuationColor"], A.AttenuationColor);
                // if (params["AttenuationDistance"])  A.AttenuationDistance = params["AttenuationDistance"].as<float>();
                // if (params["IOR"])                  A.IOR = params["IOR"].as<float>();
            }

            // Textures
            if (auto textures = materialNode["Textures"])
            {
                auto& T = material->Texture;

                // Core PBR
                TryLoadTexture(T, textures, "BaseColor", TextureType::BaseColorTexture);
                TryLoadTexture(T, textures, "Normal", TextureType::NormalTexture);
                TryLoadTexture(T, textures, "Roughness", TextureType::RoughnessTexture);
                TryLoadTexture(T, textures, "AmbientOcclusion", TextureType::AmbientOcclusionTexture);
                TryLoadTexture(T, textures, "Metallic", TextureType::MetallicTexture);
                TryLoadTexture(T, textures, "Emissive", TextureType::EmissiveTexture);
                TryLoadTexture(T, textures, "Opacity", TextureType::OpacityTexture);
                TryLoadTexture(T, textures, "ORM", TextureType::ORMTexture);

                // Extended
                // TryLoadTexture(T, textures, "Clearcoat", TextureType::ClearcoatTexture);
                // TryLoadTexture(T, textures, "ClearcoatRoughness", TextureType::ClearcoatRoughnessTexture);
                // TryLoadTexture(T, textures, "SpecularColor", TextureType::SpecularColorTexture);
                // TryLoadTexture(T, textures, "Specular", TextureType::SpecularTexture);
                // TryLoadTexture(T, textures, "SheenColor", TextureType::SheenColorTexture);
                // TryLoadTexture(T, textures, "SheenRoughness", TextureType::SheenRoughnessTexture);
                // TryLoadTexture(T, textures, "Transmission", TextureType::TransmissionTexture);
                // TryLoadTexture(T, textures, "Thickness", TextureType::ThicknessTexture);
                // (Optional future) Displacement, ClearcoatNormal, etc.
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to import material from '{}': {}", materialYAML.filename().string(), e.what());
            return;
        }
    }

    void StandardMaterial::Import(const std::filesystem::path& materialYAML)
    {
        if (!std::filesystem::exists(materialYAML)) {
            MOTION_CORE_ERROR("Standard Material file: '{}' does not exist!", materialYAML.string());
            return;
        }
        if (materialYAML.extension() != ".yaml" && materialYAML.extension() != ".yml") {
            MOTION_CORE_ERROR("Standard Material file: '{}' is not a valid YAML file!", materialYAML.string());
            return;
        }

        try
        {
            YAML::Node root = YAML::LoadFile(std::filesystem::absolute(materialYAML).string());
            YAML::Node materialNode = root["Material"];
            std::string name = materialNode["Name"].as<std::string>();

            auto& assetManager = AssetManager::GetInstance();
            auto material = assetManager.Create<StandardMaterial>(name, materialYAML);

            // Parameters
            auto& A = material->Attributes;
            if (auto params = materialNode["Parameters"])
            {
                if (params["DiffuseColor"])  YAML_GetVec3(params["DiffuseColor"], A.DiffuseColor);
                if (params["SpecularColor"]) YAML_GetVec3(params["SpecularColor"], A.SpecularColor);
                if (params["AmbientColor"])  YAML_GetVec3(params["AmbientColor"], A.AmbientColor);
                if (params["EmissiveColor"]) YAML_GetVec3(params["EmissiveColor"], A.EmissiveColor);
                if (params["Shininess"])     A.Shininess = params["Shininess"].as<float>();
                if (params["Opacity"])       A.Opacity = params["Opacity"].as<float>();

            }

            // Textures
            if (auto textures = materialNode["Textures"])
            {
                auto& T = material->Texture;
                TryLoadTexture(T, textures, "Diffuse", TextureType::DiffuseTexture);
                TryLoadTexture(T, textures, "Specular", TextureType::SpecularTexture);
                TryLoadTexture(T, textures, "Emissive", TextureType::EmissiveTexture);
                TryLoadTexture(T, textures, "Opacity", TextureType::OpacityTexture);
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to import material from '{}': {}", materialYAML.filename().string(), e.what());
            return;
        }
    }
}