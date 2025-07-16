#include "CorePCH.hpp"
#include "Importer.hpp"

#define AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR "$mat.clearcoat.roughnessFactor", 0, 0
#define AI_MATKEY_IOR "$mat.ior", 0, 0
#define AI_MATKEY_SHEEN_ROUGHNESS_FACTOR "$mat.sheen.roughnessFactor", 0, 0
#define AI_MATKEY_AMBIENT_OCCLUISION_FACTOR "$mat.occlusionStrength", 0, 0

namespace Motion::Core
{
    std::shared_ptr<StaticMesh> Importer::ImportModel(const std::string& modelName, const std::filesystem::path& path)
    {
        auto& assetManager = AssetManager::GetInstance();
        std::shared_ptr<StaticMesh> staticMeshPtr = assetManager.Create<StaticMesh>(modelName, path);

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            staticMeshPtr->m_MetaData.IsAssetInitialized = false;
            return staticMeshPtr;
        }
        else
        {
            MOTION_CORE_INFO("Assimp Importer: StaticMesh {0} loaded successfully from {1}", modelName, path.string());
            LoadNode(staticMeshPtr, scene->mRootNode, scene);

            staticMeshPtr->m_MetaData.IsAssetInitialized = true;
            return staticMeshPtr;
        }

        return nullptr;
    }


    std::shared_ptr<StaticMesh> Importer::ImportModel(UUID uuid, const std::string& modelName, const std::filesystem::path& path)
    {
        auto& assetManager = AssetManager::GetInstance();
        std::shared_ptr<StaticMesh> staticMeshPtr = assetManager.Create<StaticMesh>(uuid, modelName, path);

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            staticMeshPtr->m_MetaData.IsAssetInitialized = false;
            return staticMeshPtr;
        }
        else
        {
            MOTION_CORE_INFO("Assimp Importer: StaticMesh {0} loaded successfully from {1}", modelName, path.string());
            LoadNode(staticMeshPtr, scene->mRootNode, scene);

            staticMeshPtr->m_MetaData.IsAssetInitialized = true;
            return staticMeshPtr;
        }

        return nullptr;
    }

    static std::shared_ptr<ITexture> LoadTextures(aiTextureType aiTexType, aiMaterial* aiMaterial, TextureType textureType)
    {
        aiString property{};
        auto& assetManager = AssetManager::GetInstance();

        if ((aiMaterial->GetTextureCount(aiTexType) > 0))
        {
            if (AI_SUCCESS == aiMaterial->GetTexture(aiTexType, 0, &property))
            {
                if (property.data[0] != '*')
                {
                    MOTION_CORE_INFO("Loading Texture in {0} ", property.C_Str());
                    std::filesystem::path texturePath = std::filesystem::path(property.C_Str());
                    std::string textureFileName = texturePath.filename().string();

                    std::shared_ptr<ITexture> texture = assetManager.Create<ITexture>(textureFileName, texturePath, textureType);
                    if (texture != nullptr)
                    {
                        MOTION_CORE_INFO("{} Texture loading success!", textureFileName);
                        return texture;
                    }
                    else
                    {
                        texture.reset();
                        texture = assetManager.Create<ITexture>(textureFileName, 100, 100);

                        if (texture)
                        {
                            MOTION_CORE_WARN("Texture {0} could not be loaded, creating a default texture instead", textureFileName);
                            return texture;
                        }
                        else
                        {
                            MOTION_CORE_ERROR("Unable to load texture in {0}. Manual loading might required.", texturePath.string());
                            return nullptr;
                        }
                    }
                }
            }
        }

        return nullptr;
    }

    static float LoadMaterialFloatData(aiMaterial* currentMaterial, const char* dataType, uint32_t type, uint32_t idx)
    {
        MOTION_CORE_INFO("Looking for data type {0}", dataType);

        float data{ 0.0f };
        if (currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
        {
            MOTION_CORE_INFO("Found data type {0} with value {1}", dataType, data);
            return data;
        }
        else
        {
            MOTION_CORE_WARN("Data type {0} not found, returning default value 0.0f", dataType);
            return 0.0f;
        }
    }

    static glm::vec3 LoadMaterialVec3Data(aiMaterial* currentMaterial, const char* dataType, uint32_t type, uint32_t idx)
    {
        MOTION_CORE_INFO("Looking for data type {0}", dataType);

        glm::vec3 data{ 0.0f, 0.0f, 0.0f };
        if (currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
        {
            MOTION_CORE_INFO("Found data type {0} with value {1}, {2}, {3}", dataType, data.x, data.y, data.z);
            return data;
        }
        else
        {
            MOTION_CORE_WARN("Data type {0} not found, returning default value {1}, {2}, {3}", dataType, 0.0f, 0.0f, 0.0f);
            return { 0.0f, 0.0f, 0.0f };
        }
    }

    void Importer::LoadMesh(const std::shared_ptr<StaticMesh>& staticMeshPtr, aiMesh* mesh, const aiScene* scene)
    {
        static uint32_t meshIndex = 0;
        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        auto& assetManager = AssetManager::GetInstance();
        bool hasNormalizeTangents{ false };

        MOTION_CORE_INFO("Extracting StaticMesh SubMesh ({0}) Vertex and Indices data...", meshIndex);
        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
            // Position
            vertices.insert(vertices.end(), { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z });

            // TexCoords
            if (mesh->HasTextureCoords(0))
                vertices.insert(vertices.end(), { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y });
            else
                vertices.insert(vertices.end(), { 0.0f, 0.0f });

            // Normals
            vertices.insert(vertices.end(), { -mesh->mNormals[i].x, -mesh->mNormals[i].y, -mesh->mNormals[i].z });

            // Tangents and Bitangents
            if (mesh->HasTangentsAndBitangents())
            {
                glm::vec3 tangent = glm::normalize(glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));
                glm::vec3 bitangent = glm::normalize(glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z));

                vertices.insert(vertices.end(), { tangent.x, tangent.y, tangent.z });
                vertices.insert(vertices.end(), { bitangent.x, bitangent.y, bitangent.z });
                hasNormalizeTangents = true;
            }
            else
            {
                vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
                vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
                hasNormalizeTangents = false;
            }
        }

        // Extracting Indices
        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        std::shared_ptr<StaticMesh::MeshSegment> meshSegment = std::make_shared<StaticMesh::MeshSegment>();
        meshSegment->MeshIndex = meshIndex++;
        meshSegment->MaterialIndex = mesh->mMaterialIndex;
        meshSegment->MeshSelf = assetManager.Create<Mesh>(
            std::format("{}_SubMesh_{}", staticMeshPtr->GetName(), meshSegment->MeshIndex),
            vertices.data(), vertices.size(),
            indices.data(), indices.size(),
            BufferLayout({
                { UniformCache::VertexAttri_Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
                { UniformCache::VertexAttri_TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
                { UniformCache::VertexAttri_Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
                { UniformCache::VertexAttri_Tangents, BufferComponents::XYZ, BufferStride::F3, hasNormalizeTangents, offsetof(Vertex, Tangent) },
                { UniformCache::VertexAttri_Bitangents, BufferComponents::XYZ, BufferStride::F3, hasNormalizeTangents, offsetof(Vertex, Bitangent) }
                }),
            staticMeshPtr
        );

        LoadMaterials(meshSegment, scene);
        staticMeshPtr->m_Meshes.emplace_back(std::move(meshSegment));
    }

    void Importer::LoadNode(const std::shared_ptr<StaticMesh>& staticMeshPtr, aiNode* node, const aiScene* scene)
    {
        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            LoadMesh(staticMeshPtr, scene->mMeshes[node->mMeshes[i]], scene);
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            LoadNode(staticMeshPtr, node->mChildren[i], scene);
        }
    }

    void Motion::Core::Importer::LoadMaterials(const std::shared_ptr<StaticMesh::MeshSegment>& meshSegment, const aiScene* scene)
    {
        MOTION_CORE_INFO("Extracting StaticMesh - SubMesh {0} Materials", meshSegment->MeshIndex);
        auto& assetManager = AssetManager::GetInstance();
        aiMaterial* currentMaterial = scene->mMaterials[meshSegment->MaterialIndex];
        aiString property;

        if (currentMaterial->Get(AI_MATKEY_NAME, property) != AI_SUCCESS)
        {
            MOTION_CORE_WARN("Material without a name is not been handled. Skipping material load for SubMesh {0}", meshSegment->MeshIndex);
            return;
        }

        meshSegment->Materials = assetManager.Create<Material>(property.C_Str());
        if (!meshSegment->Materials)
        {
            MOTION_CORE_ERROR("Failed to create Material for SubMesh {0}", meshSegment->MeshIndex);
            meshSegment->MeshSelf.reset();
            meshSegment->Materials = nullptr;
            return;
        }

        // Surface Colors
        meshSegment->Materials->SetUniform(UniformCache::Color_AmbientColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_AMBIENT));
        meshSegment->Materials->SetUniform(UniformCache::Color_DiffuseColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_DIFFUSE));
        meshSegment->Materials->SetUniform(UniformCache::Color_SpecularColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_SPECULAR));
        meshSegment->Materials->SetUniform(UniformCache::Color_EmissiveColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_EMISSIVE));
        meshSegment->Materials->SetUniform(UniformCache::Color_ReflectiveColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_REFLECTIVE));
        meshSegment->Materials->SetUniform(UniformCache::Color_TransparentColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_TRANSPARENT));

        //Material properties
        meshSegment->Materials->SetUniform(UniformCache::Property_Shininess, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS));
        meshSegment->Materials->SetUniform(UniformCache::Property_ShininessStrength, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS_STRENGTH));
        meshSegment->Materials->SetUniform(UniformCache::Property_Opacity, LoadMaterialFloatData(currentMaterial, AI_MATKEY_OPACITY));
        meshSegment->Materials->SetUniform(UniformCache::Property_IndexOfRefraction, LoadMaterialFloatData(currentMaterial, AI_MATKEY_IOR));
        meshSegment->Materials->SetUniform(UniformCache::Property_BumpScaling, LoadMaterialFloatData(currentMaterial, AI_MATKEY_BUMPSCALING));
        meshSegment->Materials->SetUniform(UniformCache::Property_Reflectivity, LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFLECTIVITY));

        //Material Factors
        meshSegment->Materials->SetUniform(UniformCache::Factor_BaseColorFactor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_BASE_COLOR));
        meshSegment->Materials->SetUniform(UniformCache::Factor_MetallicFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_METALLIC_FACTOR));
        meshSegment->Materials->SetUniform(UniformCache::Factor_RoughnessFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_ROUGHNESS_FACTOR));
        meshSegment->Materials->SetUniform(UniformCache::Factor_TransmissionFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_TRANSMISSION_FACTOR));
        meshSegment->Materials->SetUniform(UniformCache::Factor_ClearCoatFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_CLEARCOAT_FACTOR));
        meshSegment->Materials->SetUniform(UniformCache::Factor_ClearCoatFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR));
        meshSegment->Materials->SetUniform(UniformCache::Factor_SheenFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHEEN_COLOR_FACTOR));
        meshSegment->Materials->SetUniform(UniformCache::Factor_SheenFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHEEN_ROUGHNESS_FACTOR));
        meshSegment->Materials->SetUniform(UniformCache::Factor_IndexOfRefraction, LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFRACTI));
        meshSegment->Materials->SetUniform(UniformCache::Factor_AmbientOcclusionFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_AMBIENT_OCCLUISION_FACTOR));

        // ********************************* legacy textures types ******************************************** //
        meshSegment->Materials->SetTexture(UniformCache::Texture_DiffuseTexture, LoadTextures(aiTextureType_DIFFUSE, currentMaterial, TextureType::DiffuseTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_SpecularTexture, LoadTextures(aiTextureType_SPECULAR, currentMaterial, TextureType::SpecularTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_AmbientTexture, LoadTextures(aiTextureType_AMBIENT, currentMaterial, TextureType::AmbientTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_EmissiveTexture, LoadTextures(aiTextureType_EMISSIVE, currentMaterial, TextureType::EmissiveTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_NormalMapsTexture, LoadTextures(aiTextureType_NORMALS, currentMaterial, TextureType::NormalMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_ShininessTexture, LoadTextures(aiTextureType_SHININESS, currentMaterial, TextureType::ShininessTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_OpacityMapsTexture, LoadTextures(aiTextureType_OPACITY, currentMaterial, TextureType::OpacityMapsTexture));

        // ********************************* Modern textures types ******************************************** //
        meshSegment->Materials->SetTexture(UniformCache::Texture_BaseColorTexture, LoadTextures(aiTextureType_BASE_COLOR, currentMaterial, TextureType::BaseColorMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_MetallicTexture, LoadTextures(aiTextureType_METALNESS, currentMaterial, TextureType::MetallicMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_RoughnessTexture, LoadTextures(aiTextureType_DIFFUSE_ROUGHNESS, currentMaterial, TextureType::RoughnessMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_AOMapTexture, LoadTextures(aiTextureType_AMBIENT_OCCLUSION, currentMaterial, TextureType::AOMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_EmissiveTexture, LoadTextures(aiTextureType_EMISSION_COLOR, currentMaterial, TextureType::EmissiveMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_ClearCoatTexture, LoadTextures(aiTextureType_CLEARCOAT, currentMaterial, TextureType::ClearCoatMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_SheenTexture, LoadTextures(aiTextureType_SHEEN, currentMaterial, TextureType::SheenMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_TransmissionTexture, LoadTextures(aiTextureType_TRANSMISSION, currentMaterial, TextureType::TransmissionMapsTexture));
    }
}