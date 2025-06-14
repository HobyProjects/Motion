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
            out << YAML::Key << "UUID" << YAML::Value << value->GetMetaData().AssetUUID;
            out << YAML::Key << "Name" << YAML::Value << value->GetMetaData().AssetName;
            out << YAML::Key << "Path" << YAML::Value << value->GetMetaData().FilePath;
            out << YAML::Key << "Type" << YAML::Value << (uint32_t)value->GetSpecification().Type;
            out << YAML::Newline;
        }
        out << YAML::EndMap;

        out << YAML::EndMap;
        std::ofstream fout(filePath);
        fout << out.c_str();
        return true;
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
        uint32_t shading_method = node["ShadingMethod"].as<uint32_t>();

        Material::ShadingMethod shadingMethod;
        switch(shading_method)
        {
            case 0: shadingMethod = Material::ShadingMethod::Phong; break;
            case 1: shadingMethod = Material::ShadingMethod::PBR; break;
            case 2: shadingMethod = Material::ShadingMethod::Unlit; break;
        };

        std::shared_ptr<Material> material = std::make_shared<Material>(uuid, name, shadingMethod, filePath.string());

        YAML::Node properties = node["Properties"];
        for(const auto& property : properties)
        {
            std::string name = property["Name"].as<std::string>();
            float value = property["Value"].as<float>();
            material->SetUniform(name, value);
        }

        YAML::Node textures = node["Textures"];
        for(const auto& texture : textures)
        {
            UUID uuid = texture["UUID"].as<UUID>();
            std::string name = texture["Name"].as<std::string>();
            std::filesystem::path path = texture["Path"].as<std::filesystem::path>();
            uint32_t type = texture["Type"].as<uint32_t>();
            TextureType textureType{TextureType::BaseColorMapsTexture};
            switch(type)
            {
                case 0: textureType = TextureType::DiffuseTexture; break;
                case 1: textureType = TextureType::AmbientTexture; break;
                case 2: textureType = TextureType::SpecularTexture; break;
                case 3: textureType = TextureType::EmissiveTexture; break;
                case 4: textureType = TextureType::NormalMapsTexture; break;
                case 5: textureType = TextureType::HeightMaps; break;
                case 6: textureType = TextureType::ShininessTexture; break;
                case 7: textureType = TextureType::OpacityMapsTexture; break;
                case 8: textureType = TextureType::LightMapsTexture; break;

                case 9: textureType = TextureType::BaseColorMapsTexture; break;
                case 10: textureType = TextureType::MetallicMapsTexture; break;
                case 11: textureType = TextureType::RoughnessMapsTexture; break;
                case 12: textureType = TextureType::AOMapsTexture; break;
                case 13: textureType = TextureType::EmissiveMapsTexture; break;
                case 14: textureType = TextureType::ClearCoatMapsTexture; break;
                case 15: textureType = TextureType::SheenMapsTexture; break;
                case 16: textureType = TextureType::TransmissionMapsTexture; break;
            }

            std::shared_ptr<ITexture> texture = AssetManager::CreateTextureFromFile(uuid, name, std::filesystem::path(path), textureType, true);
            if(texture != nullptr)
            {
                MOTION_CORE_INFO("Texture loaded: {0}", name);
                material->SetTexture(name, texture);
            }
            else
            {
                MOTION_CORE_ERROR("Failed to create texture: {0}", name);
            }
        }


        return material;
    }

}


