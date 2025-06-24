#include "CorePCH.hpp"

namespace Motion::Core
{
    bool MaterialSerializer::Serialize(const std::filesystem::path& filePath, const std::shared_ptr<Material>& material)
    {
        if(!material)
        {
            MOTION_CORE_ERROR("Failed to serialize material. Material is nullptr.");
            return false;
        }

        if(!std::filesystem::exists(filePath.parent_path()))
        {
            std::filesystem::create_directories(filePath.parent_path());
        }
        
        YAML::Emitter out;
        out << YAML::BeginMap;

        out << YAML::Key << "UUID" << YAML::Value << material->GetMetaData().AssetUUID;
        out << YAML::Key << "Name" << YAML::Value << material->GetMetaData().AssetName;
        out << YAML::Key << "ShadingMethod" << YAML::Value << (uint32_t)material->GetShadingMethod();

        out << YAML::Key << "Properties" << YAML::Value << YAML::BeginMap;
        for(const auto& [name, value] : material->GetFloatUniforms())
        {
            out << YAML::Key << name << YAML::Value << value;
        }

        for(const auto& [name, value] : material->GetVec3Uniforms())
        {
            out << YAML::Key << name << YAML::Value << YAML::Flow;
            out << YAML::BeginSeq << value.x << value.y << value.z;
            out << YAML::EndSeq;
        }

        for(const auto& [name, value] : material->GetVec4Uniforms())
        {
            out << YAML::Key << name << YAML::Value << YAML::Flow;
            out << YAML::BeginSeq << value.x << value.y << value.z << value.w;
            out << YAML::EndSeq;
        
        
        }
        out << YAML::EndMap;

        out << YAML::Key << "Textures" << YAML::Value << YAML::BeginMap;
        for(const auto& [name, value] : material->GetTextures())
        {
            out << YAML::Key << name << YAML::Value << YAML::BeginMap;            
            out << YAML::Key << "UUID" << YAML::Value << value->GetMetaData().AssetUUID;
            out << YAML::Key << "Name" << YAML::Value << value->GetMetaData().AssetName;
            out << YAML::Key << "Path" << YAML::Value << value->GetMetaData().FilePath;
            out << YAML::Key << "Type" << YAML::Value << (uint32_t)value->GetSpecification().Type;
            if(!value->IsFromFile())
            {
                out << YAML::Key << "IsFromFile" << YAML::Value << false;
                out << YAML::Key << "Width" << YAML::Value << value->GetSpecification().Width;
                out << YAML::Key << "Height" << YAML::Value << value->GetSpecification().Height;
            }
            else
            {
                out << YAML::Key << "IsFromFile" << YAML::Value << true;
                out << YAML::Key << "Width" << YAML::Value << value->GetSpecification().Width;
                out << YAML::Key << "Height" << YAML::Value << value->GetSpecification().Height;
            }

            out << YAML::EndMap;
        }
        out << YAML::EndMap;

        out << YAML::EndMap;
        std::ofstream fout(filePath);
        fout << out.c_str();
        return true;
    }

    static Material::ShadingMethod GetShadingMethod(uint32_t shadingMethod)
    {
        switch(shadingMethod)
        {
            case 0: return Material::ShadingMethod::Phong;
            case 1: return Material::ShadingMethod::PBR;
            case 2: return Material::ShadingMethod::Unlit;
            default: return Material::ShadingMethod::Unknown;
        };

        return Material::ShadingMethod::Unknown;
    }

    static TextureType GetTextureType(uint32_t textureType)
    {
        switch(textureType)
        {
            case 0: return TextureType::DiffuseTexture; 
            case 1: return TextureType::AmbientTexture; 
            case 2: return TextureType::SpecularTexture; 
            case 3: return TextureType::EmissiveTexture; 
            case 4: return TextureType::NormalMapsTexture; 
            case 5: return TextureType::HeightMaps; 
            case 6: return TextureType::ShininessTexture; 
            case 7: return TextureType::OpacityMapsTexture; 
            case 8: return TextureType::LightMapsTexture; 

            case 9: return TextureType::BaseColorMapsTexture; 
            case 10: return TextureType::MetallicMapsTexture; 
            case 11: return TextureType::RoughnessMapsTexture; 
            case 12: return TextureType::AOMapsTexture; 
            case 13: return TextureType::EmissiveMapsTexture; 
            case 14: return TextureType::ClearCoatMapsTexture; 
            case 15: return TextureType::SheenMapsTexture; 
            case 16: return TextureType::TransmissionMapsTexture; 
            default: return TextureType::UnknownTextureType;
        }

        return TextureType::UnknownTextureType;
    }

    std::shared_ptr<Material> MaterialSerializer::Deserialize(const std::filesystem::path& filePath)
    {
        if(!std::filesystem::exists(filePath))
        {
            MOTION_CORE_ERROR("Failed to deserialize material. File does not exist.");
            return nullptr;
        }

        YAML::Node node = YAML::LoadFile(filePath.string());
        UUID uuid = node["UUID"].as<UUID>();
        std::string name = node["Name"].as<std::string>();
        Material::ShadingMethod shadingMethod = GetShadingMethod(node["ShadingMethod"].as<uint32_t>());

        std::shared_ptr<Material> material = std::make_shared<Material>(uuid, name, shadingMethod, filePath.string());
        
        YAML::Node properties = node["Properties"];
        YAML::Node textures = node["Textures"];

        if(properties.IsNull() || textures.IsNull())
        {
            MOTION_CORE_WARN("Failed to deserialize material file. Properties and textures are null.");
            return nullptr;
        }

        for(const auto& property : properties)
        {
            if(property.IsScalar())
                material->SetUniform(property.first.as<std::string>(), property.second.as<float>());

            if(property.IsSequence())
            {
                if(property.second.size() == 3)
                    material->SetUniform(property.first.as<std::string>(), property.second.as<glm::vec3>());

                if(property.second.size() == 4)
                    material->SetUniform(property.first.as<std::string>(), property.second.as<glm::vec4>());
            }
        }

        for(const auto& texture : textures)
        {
            auto textureFirst = texture.first.as<std::string>();
            auto textureSecond = texture.second;

            UUID uuid = textureSecond["UUID"].as<UUID>();
            std::string name = textureSecond["Name"].as<std::string>();
            std::filesystem::path path = textureSecond["Path"].as<std::filesystem::path>();
            TextureType textureType = GetTextureType(textureSecond["Type"].as<uint32_t>());
            bool isFromFile = textureSecond["IsFromFile"].as<bool>();
            if(isFromFile)
            {
                std::shared_ptr<ITexture> texture = AssetManager::CreateTextureFromFile(uuid, name, path, textureType, true);
                if(texture != nullptr)
                {
                    MOTION_CORE_INFO("Texture loaded from file: {0} in {1}", name, path.string());
                    material->SetTexture(name, texture);
                }
                else
                {
                    MOTION_CORE_ERROR("Failed to create texture: {0}", name);
                }
            }

            if(!isFromFile)
            {
                uint32_t width = textureSecond["Width"].as<uint32_t>();
                uint32_t height = textureSecond["Height"].as<uint32_t>();

                std::shared_ptr<ITexture> texture = AssetManager::CreatePlainTexture(uuid, name, width, height);
                if(texture != nullptr)
                {
                    MOTION_CORE_INFO("Texture create in memory: {0} (width: {1}px, height: {2}px)", name, width, height);
                    material->SetTexture(name, texture);
                }
                else
                {
                    MOTION_CORE_ERROR("Failed to create texture: {0}", name);
                }
            }
        }

        return material;
    }

}


