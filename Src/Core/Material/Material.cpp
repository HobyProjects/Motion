#include "CorePCH.hpp"
#include "Material.hpp"

namespace Motion
{
    /**
     * @brief Constructs a PhysicalBasedMaterial object with the specified unique ID, name, and file path.
     *
     * @param uniqueID The unique identifier for the material asset.
     * @param materialName The name of the material.
     * @param materialFile The filesystem path to the material file.
     */
    PhysicalBasedMaterial::PhysicalBasedMaterial(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile) :
        AssetBase<IAsset>(uniqueID, materialName, AssetType::Material, materialFile.string()) {
    }

    /**
     * @brief Constructs a PhysicalBasedMaterialInstance with the specified material ID and base material.
     *
     * Initializes the instance with a unique material identifier and a shared pointer to the base physical-based material.
     * Also creates a uniform buffer for storing material attributes.
     *
     * @param materialID      A unique string identifier for this material instance.
     * @param baseMaterial    Shared pointer to the base PhysicalBasedMaterial from which this instance derives.
     */
    PhysicalBasedMaterialInstance::PhysicalBasedMaterialInstance(const std::string& materialID, const std::shared_ptr<PhysicalBasedMaterial>& baseMaterial)
        : BaseMaterial(baseMaterial), m_MaterialID(materialID), m_UniformBuffer(IShaderBuffer::Create(MATERIAL_PBR_ATTRIBUTES_SIZE, 0)) {
    }

    /**
     * @brief Uploads the material attribute data to the GPU.
     *
     * This method binds the uniform buffer associated with the material instance
     * and uploads the current set of physical-based rendering (PBR) attributes.
     * The attributes are transferred as raw buffer data, ensuring that the shader
     * has access to the latest material properties for rendering.
     *
     * @warning This method assumes that you have already bind with the shader program
     *          that uses this material instance. It does not handle shader binding.
     */
    void PhysicalBasedMaterialInstance::UploadAttributes()
    {
        m_UniformBuffer->Bind();
        m_UniformBuffer->SetRawBufferData(MATERIAL_PBR_ATTRIBUTES_SIZE, &Attributes);
    }

    /**
     * @brief Constructs a StandardMaterial object with the specified unique ID, name, and file path.
     *
     * @param uniqueID The unique identifier for the material asset.
     * @param materialName The name of the material.
     * @param materialFile The filesystem path to the material file.
     */
    StandardMaterial::StandardMaterial(UUID uniqueID, const std::string& materialName, const std::filesystem::path& materialFile) :
        AssetBase<IAsset>(uniqueID, materialName, AssetType::Material, materialFile.string()) {
    }

    /**
     * @brief Constructs a StandardMaterialInstance with the specified unique ID, material ID, and base material.
     *
     * Initializes the instance with a unique material identifier and a shared pointer to the base standard material.
     * Also creates a uniform buffer for storing material attributes.
     *
     * @param materialID    A string identifier for the material instance.
     * @param baseMaterial  Shared pointer to the base StandardMaterial from which this instance derives.
     */
    StandardMaterialInstance::StandardMaterialInstance(const std::string& materialID, const std::shared_ptr<StandardMaterial>& baseMaterial)
        : BaseMaterial(baseMaterial), m_MaterialID(materialID), m_UniformBuffer(IShaderBuffer::Create(MATERIAL_STANDARD_ATTRIBUTES_SIZE, 1)) {
    }


    /**
     * @brief Uploads the material attribute data to the GPU.
     *
     * This method binds the uniform buffer associated with the material instance
     * and uploads the current set of standard material attributes. The attributes
     * are transferred as raw buffer data, ensuring that the shader has access to
     * the latest material properties for rendering.
     *
     * @warning This method assumes that you have already bind with the shader program
     *          that uses this material instance. It does not handle shader binding.
     */
    void StandardMaterialInstance::UploadAttributes()
    {
        m_UniformBuffer->Bind();
        m_UniformBuffer->SetRawBufferData(MATERIAL_STANDARD_ATTRIBUTES_SIZE, &Attributes);
    }


    /**
     * @brief Imports a PhysicalBasedMaterial from a YAML file.
     *
     * This function reads the material properties from the specified YAML file,
     * constructs a PhysicalBasedMaterial object, and registers it with the AssetManager.
     *
     * @param materialYAML The filesystem path to the YAML file containing the material definition.
     */
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
                if (params["Opacity"])
                    data.Opacity = params["Opacity"].as<float>();
            }

            // Load textures
            auto textures = materialNode["Textures"];
            if (textures)
            {
                auto tryLoad =
                    [&](const std::string& key, TextureType type)
                    {
                        if (textures[key])
                        {
                            std::filesystem::path texPath = textures[key].as<std::string>();
                            material->Texture[type] = ITexture::Create(texPath, type, true);
                        }
                    };

                tryLoad("BaseColor", TextureType::BaseColorTexture);
                tryLoad("Normal", TextureType::NormalTexture);
                tryLoad("Roughness", TextureType::RoughnessTexture);
                tryLoad("AmbientOcclusion", TextureType::AmbientOcclusionTexture);
                tryLoad("Metallic", TextureType::MetallicTexture);
                tryLoad("Displacement", TextureType::DisplacementTexture);
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to import material from '{}': {}", materialYAML.filename().string(), e.what());
            return;
        }
    }


    /**
     * @brief Imports a StandardMaterial from a YAML file.
     *
     * This function reads the material properties from the specified YAML file,
     * constructs a StandardMaterial object, and registers it with the AssetManager.
     *
     * @param materialYAML The filesystem path to the YAML file containing the material definition.
     */
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

            // Load parameters
            auto& data = material->Attributes;
            auto params = materialNode["Parameters"];
            if (params) {
                if (params["DiffuseColor"])
                    data.DiffuseColor = glm::vec3(params["DiffuseColor"][0].as<float>(), params["DiffuseColor"][1].as<float>(), params["DiffuseColor"][2].as<float>());
                if (params["SpecularColor"])
                    data.SpecularColor = glm::vec3(params["SpecularColor"][0].as<float>(), params["SpecularColor"][1].as<float>(), params["SpecularColor"][2].as<float>());
                if (params["AmbientColor"])
                    data.AmbientColor = glm::vec3(params["AmbientColor"][0].as<float>(), params["AmbientColor"][1].as<float>(), params["AmbientColor"][2].as<float>());
                if (params["EmissiveColor"])
                    data.EmissiveColor = glm::vec3(params["EmissiveColor"][0].as<float>(), params["EmissiveColor"][1].as<float>(), params["EmissiveColor"][2].as<float>());
                if (params["Shininess"])
                    data.Shininess = params["Shininess"].as<float>();
                if (params["Opacity"])
                    data.Opacity = params["Opacity"].as<float>();
            }

            // Load textures
            auto textures = materialNode["Textures"];
            if (textures)
            {
                auto tryLoad =
                    [&](const std::string& key, TextureType type)
                    {
                        if (textures[key])
                        {
                            std::filesystem::path texPath = textures[key].as<std::string>();
                            material->Texture[type] = ITexture::Create(texPath, type, true);
                        }
                    };

                tryLoad("Diffuse", TextureType::DiffuseTexture);
                tryLoad("Specular", TextureType::SpecularTexture);
                tryLoad("Emissive", TextureType::EmissiveTexture);
                tryLoad("Opacity", TextureType::OpacityTexture);
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to import material from '{}': {}", materialYAML.filename().string(), e.what());
            return;
        }
    }
}