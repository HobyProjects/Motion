#include "CorePCH.hpp"

#define AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR "$mat.clearcoat.roughnessFactor", 0, 0
#define AI_MATKEY_IOR "$mat.ior", 0, 0
#define AI_MATKEY_SHEEN_ROUGHNESS_FACTOR "$mat.sheen.roughnessFactor", 0, 0
#define AI_MATKEY_AMBIENT_OCCLUISION_FACTOR "$mat.occlusionStrength", 0, 0

namespace Motion
{
    /**
     * @brief Imports a 3D model from the specified file path and creates a StaticMesh asset.
     *
     * This function uses the Assimp library to read and process the 3D model file located at the given path.
     * It creates a StaticMesh asset using the AssetManager and populates it with the imported mesh data.
     * If the import fails, an error is logged and the returned StaticMesh asset is marked as uninitialized.
     * On success, the mesh is loaded and the asset is marked as initialized.
     *
     * @param modelName The name to assign to the imported StaticMesh asset.
     * @param path The filesystem path to the 3D model file to import.
     * @return std::shared_ptr<StaticMesh> A shared pointer to the created StaticMesh asset. The asset's
     *         initialization status can be checked via its metadata.
     */
    std::shared_ptr<StaticMesh> Importer::ImportModel(const std::string& modelName, const std::filesystem::path& path)
    {
        auto& assetManager = AssetManager::GetInstance();
        std::shared_ptr<StaticMesh> staticMeshPtr = assetManager.Create<StaticMesh>(modelName, path);

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            staticMeshPtr->AssetInfo.IsInitialized = false;
            return staticMeshPtr;
        }
        else
        {
            MOTION_CORE_INFO("Assimp Importer: StaticMesh {0} loaded successfully from {1}", modelName, path.string());
            LoadNode(staticMeshPtr, scene->mRootNode, scene);

            staticMeshPtr->AssetInfo.IsInitialized = true;
            return staticMeshPtr;
        }

        return nullptr;
    }

    /**
     * @brief Retrieves a default texture based on the specified Assimp texture type.
     *
     * This function returns a shared pointer to a default texture corresponding to the given Assimp texture type.
     * If the texture type is not recognized, it logs an error and returns nullptr.
     *
     * @param aiTexType The Assimp texture type for which to retrieve the default texture.
     * @return std::shared_ptr<ITexture> A shared pointer to the default texture, or nullptr if the type is unsupported.
     */
    static std::shared_ptr<ITexture> GetDefaultTexture(aiTextureType aiTexType)
    {

        switch (aiTexType)
        {
        case aiTextureType_BASE_COLOR: return CreateUnregisteredPlainTexture(10, 10, { 1.0f, 1.0f, 1.0f });
        case aiTextureType_METALNESS: return CreateUnregisteredPlainTexture(10, 10, { 0.0f, 0.0f, 0.0f });
        case aiTextureType_DIFFUSE_ROUGHNESS: return CreateUnregisteredPlainTexture(10, 10, { 0.8f, 0.8f, 0.8f });
        case aiTextureType_NORMALS: return CreateUnregisteredPlainTexture(10, 10, { 0.5f, 0.5f, 1.0f });
        case aiTextureType_AMBIENT_OCCLUSION: return CreateUnregisteredPlainTexture(10, 10, { 1.0f, 1.0f, 1.0f });
        case aiTextureType_EMISSION_COLOR: return CreateUnregisteredPlainTexture(10, 10, { 0.0f, 0.0f, 0.0f });
        case aiTextureType_CLEARCOAT: return CreateUnregisteredPlainTexture(10, 10, { 0.0f, 0.0f, 0.0f });
        case aiTextureType_SHEEN: return CreateUnregisteredPlainTexture(10, 10, { 0.0f, 0.0f, 0.0f });
        case aiTextureType_TRANSMISSION: return CreateUnregisteredPlainTexture(10, 10, { 0.0f, 0.0f, 0.0f });
        case aiTextureType_DIFFUSE: return CreateUnregisteredPlainTexture(10, 10, { 1.0f, 1.0f, 1.0f });
        case aiTextureType_SPECULAR: return CreateUnregisteredPlainTexture(10, 10, { 0.0f, 0.0f, 0.0f });
        case aiTextureType_SHININESS: return CreateUnregisteredPlainTexture(10, 10, { 0.8f, 0.8f, 0.8f });
        case aiTextureType_OPACITY: return CreateUnregisteredPlainTexture(10, 10, { 1.0f, 1.0f, 1.0f });
        default: break;
        }

        MOTION_ASSERT(false, "Unsupported aiTextureType for default texture: {0}", static_cast<int>(aiTexType));
        return nullptr;
    }

    /**
     * @brief Loads a texture from the given aiMaterial based on the specified aiTextureType.
     *
     * This function attempts to load a texture from the provided Assimp material. If the texture is found and loaded successfully,
     * it returns a shared pointer to the loaded ITexture. If the texture cannot be loaded, it attempts to create a default texture.
     * If both loading and creation fail, it returns nullptr.
     *
     * @param aiTexType The Assimp texture type to look for (e.g., aiTextureType_DIFFUSE).
     * @param aiMaterial Pointer to the Assimp material from which to load the texture.
     * @param textureType The application's texture type to be associated with the loaded texture.
     * @return std::shared_ptr<ITexture> Shared pointer to the loaded or default texture, or nullptr if loading fails.
     */
    static std::shared_ptr<ITexture> LoadTextures(std::uint32_t index, aiTextureType aiTexType, aiMaterial* aiMaterial, TextureType textureType)
    {
        aiString property{};
        auto& assetManager = AssetManager::GetInstance();

        if (aiMaterial->GetTexture(aiTexType, index, &property) == AI_SUCCESS)
        {
            std::filesystem::path textureFilePath(property.C_Str());
            std::string textureFileName = textureFilePath.filename().string();

            std::shared_ptr<ITexture> texture = assetManager.Create<ITexture>(textureFileName, textureFilePath, textureType, true);
            if (texture != nullptr)
            {
                MOTION_CORE_INFO("{} Texture loading success!", textureFileName);
                return texture;
            }
            else
            {
                MOTION_CORE_ERROR("Failed to load texture from material for type: {0}", static_cast<int>(aiTexType));
                return GetDefaultTexture(aiTexType);
            }
        }
        else
        {
            MOTION_CORE_ERROR("Failed to retrieve texture property for type: {0}", static_cast<int>(aiTexType));
            return GetDefaultTexture(aiTexType);
        }
    }

    /**
     * @brief Loads a float value from an Assimp material property.
     *
     * This function attempts to retrieve a float value from the specified material property
     * using the provided data type, type, and index. If the property is found, its value is returned.
     * Otherwise, a default value of 0.0f is returned. Logging is performed to indicate whether
     * the property was found or not.
     *
     * @param currentMaterial Pointer to the aiMaterial from which to load the data.
     * @param dataType The name of the material property to retrieve.
     * @param type The type of the material property (e.g., aiTextureType).
     * @param idx The index of the property if there are multiple entries.
     * @return The float value of the requested material property, or 0.0f if not found.
     */
    static float LoadMaterialFloatData(aiMaterial* currentMaterial, const char* dataType, uint32_t type, uint32_t idx, float defaultValue = 0.0f)
    {
        MOTION_CORE_INFO("Looking for data type {0}", dataType);

        float data{ defaultValue };
        if (currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
        {
            MOTION_CORE_INFO("Found data type {0} with value {1}", dataType, data);
            return data;
        }
        else
        {
            MOTION_CORE_WARN("Data type {0} not found, returning default value {1}", dataType, defaultValue);
            return defaultValue;
        }
    }

    /**
     * @brief Loads a glm::vec3 value from an Assimp material property.
     *
     * This function attempts to retrieve a 3-component vector (vec3) from the specified
     * material property of an Assimp material. If the property is found, its value is returned.
     * Otherwise, a default vector (0.0f, 0.0f, 0.0f) is returned.
     *
     * @param currentMaterial Pointer to the aiMaterial from which to load the property.
     * @param dataType The name of the material property to retrieve (e.g., AI_MATKEY_COLOR_DIFFUSE).
     * @param type The type of the property (usually aiTextureType_NONE for colors).
     * @param idx The index of the property (usually 0).
     * @return glm::vec3 The loaded vector value, or (0.0f, 0.0f, 0.0f) if not found.
     */
    static glm::vec3 LoadMaterialVec3Data(aiMaterial* currentMaterial, const char* dataType, uint32_t type, uint32_t idx, glm::vec3 defaultValue = { 0.0f, 0.0f, 0.0f })
    {
        MOTION_CORE_INFO("Looking for data type {0}", dataType);

        glm::vec3 data{ defaultValue };
        if (currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
        {
            MOTION_CORE_INFO("Found data type {0} with value {1}, {2}, {3}", dataType, data.x, data.y, data.z);
            return data;
        }
        else
        {
            MOTION_CORE_WARN("Data type {0} not found, returning default value {1}, {2}, {3}", dataType, defaultValue.x, defaultValue.y, defaultValue.z);
            return defaultValue;
        }
    }

    /**
     * @brief Loads mesh data from an Assimp aiMesh into a StaticMesh object.
     *
     * This function extracts vertex attributes (positions, texture coordinates, normals, tangents, and bitangents)
     * and face indices from the provided aiMesh, and constructs a MeshSegment that is added to the given StaticMesh.
     * It also handles the creation of the underlying Mesh resource and associates it with the appropriate material.
     *
     * @param staticMeshPtr Shared pointer to the StaticMesh object to which the mesh data will be added.
     * @param mesh Pointer to the aiMesh structure containing the mesh data to import.
     * @param scene Pointer to the aiScene containing the mesh and its associated materials.
     */
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
            BufferLayout(
                {
                    { UniformCache::VertexAttri_Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
                    { UniformCache::VertexAttri_TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
                    { UniformCache::VertexAttri_Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
                    { UniformCache::VertexAttri_Tangents, BufferComponents::XYZ, BufferStride::F3, hasNormalizeTangents, offsetof(Vertex, Tangent) },
                    { UniformCache::VertexAttri_Bitangents, BufferComponents::XYZ, BufferStride::F3, hasNormalizeTangents, offsetof(Vertex, Bitangent) }
                }
            ),
            staticMeshPtr
        );

        LoadMaterials(meshSegment, scene);
        staticMeshPtr->m_Meshes.emplace_back(std::move(meshSegment));
    }

    /**
     * @brief Recursively loads meshes from an Assimp node and its children into a StaticMesh object.
     *
     * This function traverses the given Assimp node, loads all meshes associated with the node,
     * and then recursively processes all child nodes. Each mesh is loaded into the provided
     * StaticMesh instance using the LoadMesh function.
     *
     * @param staticMeshPtr Shared pointer to the StaticMesh object where the meshes will be loaded.
     * @param node Pointer to the current Assimp node (aiNode) to process.
     * @param scene Pointer to the Assimp scene (aiScene) containing the node and mesh data.
     */
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



    /**
     * @brief Loads and assigns material properties and textures to a given mesh segment from an Assimp scene.
     *
     * This function extracts material information from the specified mesh segment's material index in the provided
     * Assimp scene. It creates a Material instance, sets various color and property uniforms, and loads both legacy
     * and modern texture types into the material. If the material cannot be found or created, the mesh segment is
     * reset and an error is logged.
     *
     * @param meshSegment Shared pointer to the StaticMesh::MeshSegment to which materials will be assigned.
     * @param scene Pointer to the Assimp aiScene containing the material data.
     */
    void Importer::LoadMaterials(const std::shared_ptr<StaticMesh::MeshSegment>& meshSegment, const aiScene* scene)
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

        auto& attributes = meshSegment->Materials->RetrieveAttributes();

        // Surface Colors
        attributes.AmbientColor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_AMBIENT, StandardMaterialConfig::AmbientColor);
        attributes.DiffuseColor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_DIFFUSE, StandardMaterialConfig::DiffuseColor);
        attributes.SpecularColor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_SPECULAR, StandardMaterialConfig::SpecularColor);
        attributes.EmissiveColor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_EMISSIVE, StandardMaterialConfig::EmissiveColor);
        attributes.ReflectiveColor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_REFLECTIVE, StandardMaterialConfig::ReflectiveColor);
        attributes.TransparentColor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_TRANSPARENT, StandardMaterialConfig::TransparentColor);

        //Material properties
        attributes.Shininess = LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS, StandardMaterialConfig::Shininess);
        attributes.ShininessStrength = LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS_STRENGTH, StandardMaterialConfig::ShininessStrength);
        attributes.Opacity = LoadMaterialFloatData(currentMaterial, AI_MATKEY_OPACITY, StandardMaterialConfig::Opacity);
        attributes.IndexOfRefraction = LoadMaterialFloatData(currentMaterial, AI_MATKEY_IOR, StandardMaterialConfig::IndexOfRefraction);
        attributes.BumpScaling = LoadMaterialFloatData(currentMaterial, AI_MATKEY_BUMPSCALING, StandardMaterialConfig::BumpScaling);
        attributes.Reflectivity = LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFLECTIVITY, StandardMaterialConfig::Reflectivity);

        //Material Factors
        attributes.BaseColorFactor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_BASE_COLOR, StandardMaterialConfig::BaseColorFactor);
        attributes.MetallicFactor = LoadMaterialFloatData(currentMaterial, AI_MATKEY_METALLIC_FACTOR, StandardMaterialConfig::MetallicFactor);
        attributes.RoughnessFactor = LoadMaterialFloatData(currentMaterial, AI_MATKEY_ROUGHNESS_FACTOR, StandardMaterialConfig::RoughnessFactor);
        attributes.TransmissionFactor = LoadMaterialFloatData(currentMaterial, AI_MATKEY_TRANSMISSION_FACTOR, StandardMaterialConfig::TransmissionFactor);
        attributes.ClearCoatFactor = LoadMaterialFloatData(currentMaterial, AI_MATKEY_CLEARCOAT_FACTOR, StandardMaterialConfig::ClearCoatFactor);
        attributes.ClearCoatRoughnessFactor = LoadMaterialFloatData(currentMaterial, AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, StandardMaterialConfig::ClearCoatRoughnessFactor);
        attributes.SheenFactor = LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHEEN_COLOR_FACTOR, StandardMaterialConfig::SheenFactor);
        attributes.SheenRoughnessFactor = LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHEEN_ROUGHNESS_FACTOR, StandardMaterialConfig::SheenRoughnessFactor);
        attributes.IndexOfRefraction = LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFRACTI, StandardMaterialConfig::IndexOfRefraction);
        attributes.AmbientOcclusionFactor = LoadMaterialFloatData(currentMaterial, AI_MATKEY_AMBIENT_OCCLUISION_FACTOR, StandardMaterialConfig::AmbientOcclusionFactor);

        aiTextureType assimpTextureTypes[] = {
            aiTextureType_DIFFUSE,
            aiTextureType_SPECULAR,
            aiTextureType_AMBIENT,
            aiTextureType_EMISSIVE,
            aiTextureType_NORMALS,
            aiTextureType_SHININESS,
            aiTextureType_OPACITY,

            aiTextureType_BASE_COLOR,
            aiTextureType_METALNESS,
            aiTextureType_DIFFUSE_ROUGHNESS,
            aiTextureType_AMBIENT_OCCLUSION,
            aiTextureType_EMISSION_COLOR,
            aiTextureType_CLEARCOAT,
            aiTextureType_SHEEN,
            aiTextureType_TRANSMISSION
        };

        TextureType textureTypes[] = {
            TextureType::DiffuseTexture,
            TextureType::SpecularTexture,
            TextureType::AmbientTexture,
            TextureType::EmissiveTexture,
            TextureType::NormalMapsTexture,
            TextureType::ShininessTexture,
            TextureType::OpacityMapsTexture,

            TextureType::BaseColorMapsTexture,
            TextureType::MetallicMapsTexture,
            TextureType::RoughnessMapsTexture,
            TextureType::AOMapsTexture,
            TextureType::EmissiveMapsTexture,
            TextureType::ClearCoatMapsTexture,
            TextureType::SheenMapsTexture,
            TextureType::TransmissionMapsTexture
        };

        std::string_view textureUniforms[] = {
            UniformCache::Texture_DiffuseTexture,
            UniformCache::Texture_SpecularTexture,
            UniformCache::Texture_AmbientTexture,
            UniformCache::Texture_EmissiveTexture,
            UniformCache::Texture_NormalMapTexture,
            UniformCache::Texture_ShininessTexture,
            UniformCache::Texture_OpacityTexture,

            UniformCache::Texture_BaseColorTexture,
            UniformCache::Texture_MetallicTexture,
            UniformCache::Texture_RoughnessTexture,
            UniformCache::Texture_AmbientOcclusionTexture,
            UniformCache::Texture_EmissiveTexture,
            UniformCache::Texture_ClearCoatTexture,
            UniformCache::Texture_SheenTexture,
            UniformCache::Texture_TransmissionTexture
        };


        for (uint32_t i = 0; i < std::size(assimpTextureTypes); ++i)
        {
            std::uint32_t textureCount = currentMaterial->GetTextureCount(assimpTextureTypes[i]);
            if (textureCount > 0)
            {
                for (std::uint32_t j = 0; j < textureCount; j++)
                    meshSegment->Materials->SetTexture(textureUniforms[i], LoadTextures(j, assimpTextureTypes[i], currentMaterial, textureTypes[i]));
            }

        }

        meshSegment->Materials->SetupTextureParameters();   // Setup texture parameters for the material if textures are default
        meshSegment->Materials->DetermineShadingMethod();   // Determine the shading method based on the material properties
    }
}