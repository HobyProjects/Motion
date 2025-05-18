#include "CorePCH.hpp"

namespace Motion::Core
{
    std::shared_ptr<Model> Importer::ImportModel(const std::string& shaderName, const std::filesystem::path& path) 
    {
        return nullptr;
    }

    static std::shared_ptr<ITexture> LoadTextures(aiTextureType aiTexType, aiMaterial* aiMaterial, TextureType textureType)
    {
        aiString property{};

        if ((aiMaterial->GetTextureCount(aiTexType) > 0)) 
        {
            if (AI_SUCCESS == aiMaterial->GetTexture(aiTexType, 0, &property)) 
            {
                if(property.data[0] != '*') 
                {
                    MOTION_CORE_INFO("Loading Texture in {0}  | Type : Diffure Texture", property.C_Str());
                    std::shared_ptr<ITexture> texture = TextureBuilder::CreateTextureFromFile(property.C_Str(), textureType);
                    if(texture != nullptr)
                    {
                        MOTION_CORE_INFO("Loading success!");
                        return texture;
                    }
                    else
                    {
                        MOTION_CORE_ERROR("Unable to load texture in {0}. Manual loading might required.", property.C_Str());
                        return nullptr;
                    }
                }
            } else 
            {
                MOTION_CORE_WARN("The model contained diffuse texture information, but texture loading failed. PATH: {0}", property.C_Str());
                return nullptr;
            }
        }

        return nullptr;
    }

    static float LoadMaterialFloatData(aiMaterial* currentMaterial, const char* dataType, uint32_t type, uint32_t idx)
    {
        float data{0.0f};
        if(currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
            return data;
        else
            return 0.0f;
    }

    static glm::vec3 LoadMaterialVec3Data(aiMaterial* currentMaterial, const char* dataType, uint32_t type, uint32_t idx)
    {
        glm::vec3 data{0.0f, 0.0f, 0.0f};
        if(currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
            return data;
        else
            return {0.0f, 0.0f, 0.0f};
    }

    void Importer::LoadMesh(const std::shared_ptr<Model>& modelPtr, aiMesh* mesh, const aiScene* scene)
    {
        static uint32_t meshIndex = 0;

        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        // Extracting vertex, TexCoords and Normals
        MOTION_CORE_INFO("Extracting Model Mesh ({0}) Vertex and Indices data...", meshIndex);
        for(uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            vertices.insert(vertices.end(), { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z});
            
            if(mesh->HasTextureCoords(0))
                vertices.insert(vertices.end(), { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y });
            else
                vertices.insert(vertices.end(), { 0.0f, 0.0f });

            vertices.insert(vertices.end(), { -mesh->mNormals [i].x, -mesh->mNormals [i].y, -mesh->mNormals [i].z });
        }

        // Extracting Normals
        for(uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for(uint32_t j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        std::shared_ptr<Model::SubMesh> subMesh = std::make_shared<Model::SubMesh>(meshIndex++, mesh->mMaterialIndex, std::make_shared<Mesh>(
            vertices.data(), 
            (uint32_t) vertices.size(), 
            indices.data(), 
            (uint32_t) indices.size(), 
            BufferLayout(
            {
                { UniformCache::PositionLayout, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Mesh::Vertex, Position) },
                { UniformCache::TextureCoordsLayout, BufferComponents::UV, BufferStride::F2, false, offsetof(Mesh::Vertex, TexCoords)},
                { UniformCache::NormalsLayout, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Mesh::Vertex, Normals) }
            }
        )));
        
        modelPtr->m_SubMeshes.emplace_back(std::move(subMesh));
    }

    void Importer::LoadNode(const std::shared_ptr<Model>& modelPtr, aiNode* node, const aiScene* scene)
    {
        for(uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            LoadMesh(modelPtr, scene->mMeshes[node->mMeshes[i]], scene);
        }

        for(uint32_t i = 0; i < node->mNumChildren; i++)
        {
            LoadNode(modelPtr, node->mChildren[i], scene);
        }
    }

    void Motion::Core::Importer::LoadMaterials(const std::shared_ptr<Model>& modelPtr, const aiScene* scene) 
    {
        //Extracting Materials 
        for(auto& mesh : modelPtr->m_SubMeshes)
        {
            MOTION_CORE_INFO("Extracting Model Mesh {{0}} Materials", mesh->MeshIndex);
            aiMaterial* currentMaterial = scene->mMaterials[mesh->MaterialIndex];
            aiString property;
            if(currentMaterial->Get(AI_MATKEY_NAME, property) != AI_SUCCESS)
            {
                MOTION_CORE_WARN("Material without a name is not handled");
                continue;
            }

            std::shared_ptr<Model::SubMeshMaterial> subMeshMaterials = std::make_shared<Model::SubMeshMaterial>(mesh->MaterialIndex, 
                std::make_shared<Material>(modelPtr->m_Shader->GetName()));

            subMeshMaterials->Materials->SetUniform(UniformCache::AmbientColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_AMBIENT));
            subMeshMaterials->Materials->SetUniform(UniformCache::DiffuseColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_DIFFUSE));
            subMeshMaterials->Materials->SetUniform(UniformCache::SpecularColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_SPECULAR));
            subMeshMaterials->Materials->SetUniform(UniformCache::EmissiveColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_EMISSIVE));
            subMeshMaterials->Materials->SetUniform(UniformCache::ReflectiveColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_REFLECTIVE));
            subMeshMaterials->Materials->SetUniform(UniformCache::TransparentColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_TRANSPARENT));
            subMeshMaterials->Materials->SetUniform(UniformCache::BaseColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_BASE_COLOR));

            subMeshMaterials->Materials->SetUniform(UniformCache::Shininess, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS));
            subMeshMaterials->Materials->SetUniform(UniformCache::ShininessStrenght, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS_STRENGTH));
            subMeshMaterials->Materials->SetUniform(UniformCache::Opacity, LoadMaterialFloatData(currentMaterial, AI_MATKEY_OPACITY));
            subMeshMaterials->Materials->SetUniform(UniformCache::ReflectiveIndex, LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFRACTI));
            subMeshMaterials->Materials->SetUniform(UniformCache::MetallicFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_METALLIC_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::RoughnessFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_ROUGHNESS_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::ClearcoatFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_CLEARCOAT_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::SheenFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHEEN_COLOR_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::TransmissionFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_TRANSMISSION_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::AmbientOcclusion, LoadMaterialFloatData(currentMaterial, "$ambient_occlusion", 0, 0));

            // ********************************* legacy textures types ******************************************** //

            subMeshMaterials->Materials->SetTexture(UniformCache::DiffuseTexture, LoadTextures(aiTextureType_DIFFUSE, currentMaterial, TextureType::DiffuseTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::SpecularTexture, LoadTextures(aiTextureType_SPECULAR, currentMaterial, TextureType::SpecularTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::AmbientTexture, LoadTextures(aiTextureType_AMBIENT, currentMaterial, TextureType::AmbientTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::EmissiveTexture, LoadTextures(aiTextureType_EMISSIVE, currentMaterial, TextureType::EmissiveTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::NormalsTexture, LoadTextures(aiTextureType_NORMALS, currentMaterial, TextureType::NormalsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::ShininessTexture, LoadTextures(aiTextureType_SHININESS, currentMaterial, TextureType::ShininessTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::OpacityTexture, LoadTextures(aiTextureType_OPACITY, currentMaterial, TextureType::OpacityTexture));

            // ********************************* Modern textures types ******************************************** //

            subMeshMaterials->Materials->SetTexture(UniformCache::BaseColorTexture, LoadTextures(aiTextureType_BASE_COLOR, currentMaterial, TextureType::BaseColorTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::MetalnessTexture, LoadTextures(aiTextureType_METALNESS, currentMaterial, TextureType::MetalnessTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::DiffuseRoughnessTexture, LoadTextures(aiTextureType_DIFFUSE_ROUGHNESS, currentMaterial, TextureType::DiffuseRoughnessTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::AmbientOcclusionTexture, LoadTextures(aiTextureType_AMBIENT_OCCLUSION, currentMaterial, TextureType::AmbientOcclusionTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::EmissiveColorTexture, LoadTextures(aiTextureType_EMISSION_COLOR, currentMaterial, TextureType::EmissiveColorTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::ClearCoatTexture, LoadTextures(aiTextureType_CLEARCOAT, currentMaterial,  TextureType::ClearCoatTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::SheenTexture, LoadTextures(aiTextureType_SHEEN, currentMaterial, TextureType::SheenTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::TransmissionTexture, LoadTextures(aiTextureType_TRANSMISSION, currentMaterial, TextureType::TransmissionTexture));
        }
    }
}