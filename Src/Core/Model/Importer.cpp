#include "CorePCH.hpp"
#include "Importer.hpp"

static constexpr const char* MATKEY_CLEARCOAT_ROUGHNESS_FACTOR = "$mat.clearcoat.roughnessFactor";
static constexpr const char* MATKEY_IOR = "$mat.ior";
static constexpr const char* MATKEY_SHEEN_ROUGHNESS_FACTOR = "$mat.sheen.roughnessFactor";
static constexpr const char* MATKEY_AMBIENT_OCCLUISION_FACTOR = "$mat.occlusionStrength";

namespace Motion::Core
{
    std::shared_ptr<Model> Importer::ImportModel(const std::string& modelName, const std::filesystem::path& path) 
    {
        std::shared_ptr<Model> modelPtr = std::make_shared<Model>(modelName, path);
        modelPtr->Name = modelName;

        Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            return nullptr;
        }
        else
        {
            MOTION_CORE_INFO("Assimp Importer: Model {0} loaded successfully from {1}", modelName, path.string());
            modelPtr->m_MetaData.IsLoaded = true;
            LoadNode(modelPtr, scene->mRootNode, scene);
            LoadMaterials(modelPtr, scene);
            modelPtr->m_MetaData.IsLoaded = true;
            return modelPtr;
        }

        return nullptr;
    }

    
    std::shared_ptr<Model> Importer::ImportModel(UUID uuid, const std::string & modelName, const std::filesystem::path & path)
    {
        std::shared_ptr<Model> modelPtr = std::make_shared<Model>(uuid, modelName, path);
        modelPtr->Name = modelName;

        Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) 
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            return nullptr;
        }
        else
        {
            MOTION_CORE_INFO("Assimp Importer: Model {0} loaded successfully from {1}", modelName, path.string());
            modelPtr->m_MetaData.IsLoaded = true;
            LoadNode(modelPtr, scene->mRootNode, scene);
            LoadMaterials(modelPtr, scene);
            modelPtr->m_MetaData.IsLoaded = true;
            return modelPtr;
        }

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
                    MOTION_CORE_INFO("Loading Texture in {0} ", property.C_Str());
                    std::shared_ptr<ITexture> texture = AssetManager::CreateTextureFromFile(property.C_Str(), std::filesystem::path(property.C_Str()), textureType);
                    if(texture != nullptr)
                    {
                        MOTION_CORE_INFO("Loading success!");
                        return texture;
                    }
                    else
                    {
                        texture.reset();
                        texture = AssetManager::CreatePlainTexture(property.C_Str(), 100, 100);
                        if(texture)
                        {
                            MOTION_CORE_WARN("Texture {0} could not be loaded, creating a default texture instead", property.C_Str());
                            return texture;
                        }

                        MOTION_CORE_ERROR("Unable to load texture in {0}. Manual loading might required.", property.C_Str());
                        return nullptr;
                    }
                }
            } 
            else 
            {
                MOTION_CORE_WARN("The model contained diffuse texture information, but texture loading failed. PATH: {0}", property.C_Str());
                return nullptr;
            }
        }

        return nullptr;
    }

    static float LoadMaterialFloatData(aiMaterial* currentMaterial, const char* dataType, uint32_t type, uint32_t idx)
    {
        MOTION_CORE_INFO(" >> Looking for data type {0}", dataType);

        float data{0.0f};
        if(currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
        {
            MOTION_CORE_INFO(" >> Found data type {0} with value {1}", dataType, data);
            return data;
        }
        else
        {
            MOTION_CORE_WARN(" >> Data type {0} not found, returning default value 0.0f", dataType);
            return 0.0f;
        }
    }

    static glm::vec3 LoadMaterialVec3Data(aiMaterial* currentMaterial, const char* dataType, uint32_t type, uint32_t idx)
    {
        MOTION_CORE_INFO(" >> Looking for data type {0}", dataType);

        glm::vec3 data{0.0f, 0.0f, 0.0f};
        if(currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
        {
            MOTION_CORE_INFO(" >> Found data type {0} with value {1}, {2}, {3}", dataType, data.x, data.y, data.z);
            return data;
        }
        else
        {
            MOTION_CORE_WARN(" >> Data type {0} not found, returning default value {1}, {2}, {3}", dataType, 0.0f, 0.0f, 0.0f);
            return {0.0f, 0.0f, 0.0f};
        }
    }

    void Importer::LoadMesh(const std::shared_ptr<Model>& modelPtr, aiMesh* mesh, const aiScene* scene)
    {
        static uint32_t meshIndex = 0;
        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        // Extracting vertex, TexCoords and Normals
        MOTION_CORE_INFO("Extracting Model SubMesh ({0}) Vertex and Indices data...", meshIndex);
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
                { "a_Position", BufferComponents::XYZ, BufferStride::F3, false, offsetof(Mesh::Vertex, Position) },
                { "a_TexCoords", BufferComponents::UV, BufferStride::F2, false, offsetof(Mesh::Vertex, TexCoords)},
                { "a_Normals", BufferComponents::XYZ, BufferStride::F3, false, offsetof(Mesh::Vertex, Normals) }
            }
        )));
        
        subMesh->ParentModel = modelPtr.get();
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
            MOTION_CORE_INFO("Extracting Model SubMesh {0} Materials", mesh->MeshIndex);
            aiMaterial* currentMaterial = scene->mMaterials[mesh->MaterialIndex];

            aiString property;
            if(currentMaterial->Get(AI_MATKEY_NAME, property) != AI_SUCCESS)
            {
                MOTION_CORE_WARN("Material without a name is not handled >> SKIPPING {0}", mesh->MaterialIndex);
                continue;
            }

            std::shared_ptr<Model::SubMeshMaterial> subMeshMaterials = std::make_shared<Model::SubMeshMaterial>(mesh->MaterialIndex, mesh->MeshIndex, std::format("SubMesh {0} Material {1} - {2} ", mesh->MeshIndex, mesh->MaterialIndex, property.C_Str()), property.C_Str());

            // Suface Colors
            subMeshMaterials->Materials->SetUniform(UniformCache::SurfaceColorsUniforms::AmbientColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_AMBIENT));
            subMeshMaterials->Materials->SetUniform(UniformCache::SurfaceColorsUniforms::DiffuseColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_DIFFUSE));
            subMeshMaterials->Materials->SetUniform(UniformCache::SurfaceColorsUniforms::SpecularColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_SPECULAR));
            subMeshMaterials->Materials->SetUniform(UniformCache::SurfaceColorsUniforms::EmissiveColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_EMISSIVE));
            subMeshMaterials->Materials->SetUniform(UniformCache::SurfaceColorsUniforms::ReflectiveColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_REFLECTIVE));
            subMeshMaterials->Materials->SetUniform(UniformCache::SurfaceColorsUniforms::TransparentColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_TRANSPARENT));

            //Material properties
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialPropertiesUniforms::Shininess, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialPropertiesUniforms::ShininessStrength, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS_STRENGTH));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialPropertiesUniforms::Opacity, LoadMaterialFloatData(currentMaterial, AI_MATKEY_OPACITY));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialPropertiesUniforms::IndexOfRefraction, LoadMaterialFloatData(currentMaterial, MATKEY_IOR, 0, 0));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialPropertiesUniforms::BumpScaling, LoadMaterialFloatData(currentMaterial, AI_MATKEY_BUMPSCALING));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialPropertiesUniforms::Reflectivity, LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFLECTIVITY));

            //Material Factors
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::BaseColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_BASE_COLOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::MetallicFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_METALLIC_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::RoughnessFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_ROUGHNESS_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::TransmissionFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_TRANSMISSION_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::ClearCoatFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_CLEARCOAT_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::ClearCoatFactor, LoadMaterialFloatData(currentMaterial, MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, 0, 0));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::SheenFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHEEN_COLOR_FACTOR));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::SheenFactor, LoadMaterialFloatData(currentMaterial, MATKEY_SHEEN_ROUGHNESS_FACTOR, 0, 0));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::IndexOfRefraction, LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFRACTI));
            subMeshMaterials->Materials->SetUniform(UniformCache::MaterialFactorsUniforms::AmbientOcclusionFactor, LoadMaterialFloatData(currentMaterial, MATKEY_AMBIENT_OCCLUISION_FACTOR, 0, 0));

            // ********************************* legacy textures types ******************************************** //

            subMeshMaterials->Materials->SetTexture(UniformCache::LegacyTextureUniforms::DiffuseTexture, LoadTextures(aiTextureType_DIFFUSE, currentMaterial, TextureType::DiffuseTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::LegacyTextureUniforms::SpecularTexture, LoadTextures(aiTextureType_SPECULAR, currentMaterial, TextureType::SpecularTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::LegacyTextureUniforms::AmbientTexture, LoadTextures(aiTextureType_AMBIENT, currentMaterial, TextureType::AmbientTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::LegacyTextureUniforms::EmissiveTexture, LoadTextures(aiTextureType_EMISSIVE, currentMaterial, TextureType::EmissiveTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::LegacyTextureUniforms::NormalMapsTexture, LoadTextures(aiTextureType_NORMALS, currentMaterial, TextureType::NormalMapsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::LegacyTextureUniforms::ShininessTexture, LoadTextures(aiTextureType_SHININESS, currentMaterial, TextureType::ShininessTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::LegacyTextureUniforms::OpacityMapsTexture, LoadTextures(aiTextureType_OPACITY, currentMaterial, TextureType::OpacityMapsTexture));

            // ********************************* Modern textures types ******************************************** //

            subMeshMaterials->Materials->SetTexture(UniformCache::PBRTextureUniforms::BaseColorTexture, LoadTextures(aiTextureType_BASE_COLOR, currentMaterial, TextureType::BaseColorMapsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::PBRTextureUniforms::MetallicTexture, LoadTextures(aiTextureType_METALNESS, currentMaterial, TextureType::MetallicMapsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::PBRTextureUniforms::RoughnessTexture, LoadTextures(aiTextureType_DIFFUSE_ROUGHNESS, currentMaterial, TextureType::RoughnessMapsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::PBRTextureUniforms::AOMapTexture, LoadTextures(aiTextureType_AMBIENT_OCCLUSION, currentMaterial, TextureType::AOMapsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::PBRTextureUniforms::EmissiveTexture, LoadTextures(aiTextureType_EMISSION_COLOR, currentMaterial, TextureType::EmissiveMapsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::PBRTextureUniforms::ClearCoatTexture, LoadTextures(aiTextureType_CLEARCOAT, currentMaterial,  TextureType::ClearCoatMapsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::PBRTextureUniforms::SheenTexture, LoadTextures(aiTextureType_SHEEN, currentMaterial, TextureType::SheenMapsTexture));
            subMeshMaterials->Materials->SetTexture(UniformCache::PBRTextureUniforms::TransmissionTexture, LoadTextures(aiTextureType_TRANSMISSION, currentMaterial, TextureType::TransmissionMapsTexture));

            subMeshMaterials->ParentModel = modelPtr.get();
            modelPtr->m_SubMeshMaterialMapping[mesh->MeshIndex] = subMeshMaterials;
        }
    }
}