#include "CorePCH.hpp"

#define AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR "$mat.clearcoat.roughnessFactor", 0, 0
#define AI_MATKEY_IOR "$mat.ior", 0, 0
#define AI_MATKEY_SHEEN_ROUGHNESS_FACTOR "$mat.sheen.roughnessFactor", 0, 0
#define AI_MATKEY_AMBIENT_OCCLUISION_FACTOR "$mat.occlusionStrength", 0, 0

namespace Motion::Core
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
        return ImportModel(UniqueIdentity::GetUniqueID(), modelName, path);
    }

    /**
     * @brief Imports a 3D model from the specified file path and creates a StaticMesh asset with a unique UUID.
     *
     * This function uses the Assimp library to read and process the 3D model file located at the given path.
     * It creates a StaticMesh asset using the AssetManager and populates it with the imported mesh data.
     * If the import fails, an error is logged and the returned StaticMesh asset is marked as uninitialized.
     * On success, the mesh is loaded and the asset is marked as initialized.
     *
     * @param uuid The unique identifier for the imported StaticMesh asset.
     * @param modelName The name to assign to the imported StaticMesh asset.
     * @param path The filesystem path to the 3D model file to import.
     * @return std::shared_ptr<StaticMesh> A shared pointer to the created StaticMesh asset. The asset's
     *         initialization status can be checked via its metadata.
     */
    std::shared_ptr<StaticMesh> Importer::ImportModel(UUID uuid, const std::string& modelName, const std::filesystem::path& path)
    {
        auto& assetManager = AssetManager::GetInstance();
        std::shared_ptr<StaticMesh> staticMeshPtr = assetManager.Create<StaticMesh>(uuid, modelName, path);

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
        if (aiTexType == aiTextureType_BASE_COLOR) return MaterialFallbackTextures::White;
        if (aiTexType == aiTextureType_METALNESS)   return MaterialFallbackTextures::Black;
        if (aiTexType == aiTextureType_DIFFUSE_ROUGHNESS) return MaterialFallbackTextures::Grey;
        if (aiTexType == aiTextureType_NORMALS)     return MaterialFallbackTextures::Normal;
        if (aiTexType == aiTextureType_AMBIENT_OCCLUSION) return MaterialFallbackTextures::White;
        if (aiTexType == aiTextureType_EMISSION_COLOR) return MaterialFallbackTextures::Black;
        if (aiTexType == aiTextureType_CLEARCOAT) return MaterialFallbackTextures::Black;
        if (aiTexType == aiTextureType_SHEEN) return MaterialFallbackTextures::Black;
        if (aiTexType == aiTextureType_TRANSMISSION) return MaterialFallbackTextures::Black;

        if (aiTexType == aiTextureType_DIFFUSE) return MaterialFallbackTextures::White;
        if (aiTexType == aiTextureType_SPECULAR) return MaterialFallbackTextures::Black;
        if (aiTexType == aiTextureType_SHININESS) return MaterialFallbackTextures::Grey;
        if (aiTexType == aiTextureType_OPACITY) return MaterialFallbackTextures::White;

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
    static std::shared_ptr<ITexture> LoadTextures(aiTextureType aiTexType, aiMaterial* aiMaterial, TextureType textureType)
    {
        aiString property{};
        auto& assetManager = AssetManager::GetInstance();

        if ((aiMaterial->GetTextureCount(aiTexType) > 0))
        {
            if (aiMaterial->GetTexture(aiTexType, 0, &property) == AI_SUCCESS)
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
                        MOTION_CORE_ERROR("Failed to load texture from material for type: {0}", static_cast<int>(aiTexType));
                        return GetDefaultTexture(aiTexType);
                    }
                }
                else
                {
                    MOTION_CORE_WARN("Texture property {0} is a placeholder, skipping loading.", property.C_Str());
                }
            }
            else
            {
                MOTION_CORE_ERROR("Failed to load texture from material for type: {0}", static_cast<int>(aiTexType));
                return GetDefaultTexture(aiTexType);
            }
        }
        else
        {
            MOTION_CORE_WARN("No texture found for type: {0}, using default texture.", static_cast<int>(aiTexType));
            return GetDefaultTexture(aiTexType);
        }

        return nullptr;
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
        meshSegment->Materials->SetUniform(UniformCache::Color_AmbientColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_AMBIENT, StandardMaterialConfig::AmbientColor));
        meshSegment->Materials->SetUniform(UniformCache::Color_DiffuseColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_DIFFUSE, StandardMaterialConfig::DiffuseColor));
        meshSegment->Materials->SetUniform(UniformCache::Color_SpecularColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_SPECULAR, StandardMaterialConfig::SpecularColor));
        meshSegment->Materials->SetUniform(UniformCache::Color_EmissiveColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_EMISSIVE, StandardMaterialConfig::EmissiveColor));
        meshSegment->Materials->SetUniform(UniformCache::Color_ReflectiveColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_REFLECTIVE, StandardMaterialConfig::ReflectiveColor));
        meshSegment->Materials->SetUniform(UniformCache::Color_TransparentColor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_TRANSPARENT, StandardMaterialConfig::TransparentColor));

        //Material properties
        meshSegment->Materials->SetUniform(UniformCache::Property_Shininess, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS, StandardMaterialConfig::Shininess));
        meshSegment->Materials->SetUniform(UniformCache::Property_ShininessStrength, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHININESS_STRENGTH, StandardMaterialConfig::ShininessStrength));
        meshSegment->Materials->SetUniform(UniformCache::Property_Opacity, LoadMaterialFloatData(currentMaterial, AI_MATKEY_OPACITY, StandardMaterialConfig::Opacity));
        meshSegment->Materials->SetUniform(UniformCache::Property_IndexOfRefraction, LoadMaterialFloatData(currentMaterial, AI_MATKEY_IOR, StandardMaterialConfig::IndexOfRefraction));
        meshSegment->Materials->SetUniform(UniformCache::Property_BumpScaling, LoadMaterialFloatData(currentMaterial, AI_MATKEY_BUMPSCALING, StandardMaterialConfig::BumpScaling));
        meshSegment->Materials->SetUniform(UniformCache::Property_Reflectivity, LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFLECTIVITY, StandardMaterialConfig::Reflectivity));

        //Material Factors
        meshSegment->Materials->SetUniform(UniformCache::Factor_BaseColorFactor, LoadMaterialVec3Data(currentMaterial, AI_MATKEY_BASE_COLOR, StandardMaterialConfig::BaseColorFactor));
        meshSegment->Materials->SetUniform(UniformCache::Factor_MetallicFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_METALLIC_FACTOR, StandardMaterialConfig::MetallicFactor));
        meshSegment->Materials->SetUniform(UniformCache::Factor_RoughnessFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_ROUGHNESS_FACTOR, StandardMaterialConfig::RoughnessFactor));
        meshSegment->Materials->SetUniform(UniformCache::Factor_TransmissionFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_TRANSMISSION_FACTOR, StandardMaterialConfig::TransmissionFactor));
        meshSegment->Materials->SetUniform(UniformCache::Factor_ClearCoatFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_CLEARCOAT_FACTOR, StandardMaterialConfig::ClearCoatFactor));
        meshSegment->Materials->SetUniform(UniformCache::Factor_ClearCoatRoughnessFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, StandardMaterialConfig::ClearCoatRoughnessFactor));
        meshSegment->Materials->SetUniform(UniformCache::Factor_SheenFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHEEN_COLOR_FACTOR, StandardMaterialConfig::SheenFactor));
        meshSegment->Materials->SetUniform(UniformCache::Factor_SheenRoughnessFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_SHEEN_ROUGHNESS_FACTOR, StandardMaterialConfig::SheenRoughnessFactor));
        meshSegment->Materials->SetUniform(UniformCache::Factor_IndexOfRefraction, LoadMaterialFloatData(currentMaterial, AI_MATKEY_REFRACTI, StandardMaterialConfig::IndexOfRefraction));
        meshSegment->Materials->SetUniform(UniformCache::Factor_AmbientOcclusionFactor, LoadMaterialFloatData(currentMaterial, AI_MATKEY_AMBIENT_OCCLUISION_FACTOR, StandardMaterialConfig::AmbientOcclusionFactor));

        // ********************************* legacy textures types ******************************************** //
        meshSegment->Materials->SetTexture(UniformCache::Texture_DiffuseTexture, LoadTextures(aiTextureType_DIFFUSE, currentMaterial, TextureType::DiffuseTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_SpecularTexture, LoadTextures(aiTextureType_SPECULAR, currentMaterial, TextureType::SpecularTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_AmbientTexture, LoadTextures(aiTextureType_AMBIENT, currentMaterial, TextureType::AmbientTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_EmissiveTexture, LoadTextures(aiTextureType_EMISSIVE, currentMaterial, TextureType::EmissiveTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_NormalMapTexture, LoadTextures(aiTextureType_NORMALS, currentMaterial, TextureType::NormalMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_ShininessTexture, LoadTextures(aiTextureType_SHININESS, currentMaterial, TextureType::ShininessTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_OpacityTexture, LoadTextures(aiTextureType_OPACITY, currentMaterial, TextureType::OpacityMapsTexture));

        // ********************************* Modern textures types ******************************************** //
        meshSegment->Materials->SetTexture(UniformCache::Texture_BaseColorTexture, LoadTextures(aiTextureType_BASE_COLOR, currentMaterial, TextureType::BaseColorMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_MetallicTexture, LoadTextures(aiTextureType_METALNESS, currentMaterial, TextureType::MetallicMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_RoughnessTexture, LoadTextures(aiTextureType_DIFFUSE_ROUGHNESS, currentMaterial, TextureType::RoughnessMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_AmbientOcclusionTexture, LoadTextures(aiTextureType_AMBIENT_OCCLUSION, currentMaterial, TextureType::AOMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_EmissiveTexture, LoadTextures(aiTextureType_EMISSION_COLOR, currentMaterial, TextureType::EmissiveMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_ClearCoatTexture, LoadTextures(aiTextureType_CLEARCOAT, currentMaterial, TextureType::ClearCoatMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_SheenTexture, LoadTextures(aiTextureType_SHEEN, currentMaterial, TextureType::SheenMapsTexture));
        meshSegment->Materials->SetTexture(UniformCache::Texture_TransmissionTexture, LoadTextures(aiTextureType_TRANSMISSION, currentMaterial, TextureType::TransmissionMapsTexture));

        // Setup texture parameters for the material if textures are default
        meshSegment->Materials->SetupTextureParameters();

        // Determine the shading method based on the material properties
        meshSegment->Materials->DetermineShadingMethod();
    }
}