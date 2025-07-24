#include "CorePCH.hpp"

// Base Color
#define MATKEY_COLOR_BASE "$clr.base", 0, 0
#define MATKEY_BASE_COLOR_TEXTURE aiTextureType_BASE_COLOR

// Metallic-Roughness
#define MATKEY_METALLIC_FACTOR "$mat.metallicFactor", 0, 0
#define MATKEY_ROUGHNESS_FACTOR "$mat.roughnessFactor", 0, 0
#define MATKEY_METALLIC_TEXTURE aiTextureType_METALNESS
#define MATKEY_ROUGHNESS_TEXTURE aiTextureType_DIFFUSE_ROUGHNESS

// Ambient Occlusion
#define MATKEY_AMBIENT_OCCLUSION_TEXTURE aiTextureType_AMBIENT_OCCLUSION
#define MATKEY_AMBIENT_OCCLUISION_FACTOR "$mat.occlusionStrength", 0, 0

// Normal Map
#define MATKEY_NORMAL_TEXTURE aiTextureType_NORMALS

// Emissive
#define MATKEY_EMISSION_COLOR "$clr.emissive", 0, 0
#define MATKEY_EMISSION_TEXTURE aiTextureType_EMISSION_COLOR

// Clear Coat (KHR_materials_clearcoat)
#define MATKEY_CLEARCOAT_FACTOR "$mat.clearcoat.factor", 0, 0
#define MATKEY_CLEARCOAT_ROUGHNESS_FACTOR "$mat.clearcoat.roughnessFactor", 0, 0
#define MATKEY_CLEARCOAT_TEXTURE aiTextureType_CLEARCOAT

// Sheen (KHR_materials_sheen)
#define MATKEY_SHEEN_FACTOR "$mat.sheen.colorFactor", 0, 0
#define MATKEY_SHEEN_ROUGHNESS_FACTOR "$mat.sheen.roughnessFactor", 0, 0
#define MATKEY_SHEEN_TEXTURE aiTextureType_SHEEN

// Transmission (KHR_materials_transmission)
#define MATKEY_TRANSMISSION_FACTOR "$mat.transmission.factor", 0, 0
#define MATKEY_TRANSMISSION_TEXTURE aiTextureType_TRANSMISSION

// Index of Refraction (KHR_materials_ior)
#define MATKEY_IOR "$mat.ior", 0, 0

// Opacity / Alpha Mode
#define MATKEY_OPACITY "$mat.opacity", 0, 0
#define MATKEY_OPACITY_TEXTURE aiTextureType_OPACITY

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
     * @param dataType The name of the material property to retrieve (e.g., MATKEY_COLOR_DIFFUSE).
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
                    { UniformCache::Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
                    { UniformCache::TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
                    { UniformCache::Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
                    { UniformCache::Tangents, BufferComponents::XYZ, BufferStride::F3, hasNormalizeTangents, offsetof(Vertex, Tangent) },
                    { UniformCache::Bitangents, BufferComponents::XYZ, BufferStride::F3, hasNormalizeTangents, offsetof(Vertex, Bitangent) }
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

        for (std::uint32_t i = 0; i < MAX_MATERIAL_LAYERS; i++)
        {
            MaterialLayer layer{};
            layer.Data.BaseColor = LoadMaterialVec3Data(currentMaterial, MATKEY_COLOR_BASE, MaterialDefaultValues::BaseColor);
            layer.Data.Metallic = LoadMaterialFloatData(currentMaterial, MATKEY_METALLIC_FACTOR, MaterialDefaultValues::Metallic);
            layer.Data.Roughness = LoadMaterialFloatData(currentMaterial, MATKEY_ROUGHNESS_FACTOR, MaterialDefaultValues::Roughness);
            layer.Data.Opacity = LoadMaterialFloatData(currentMaterial, MATKEY_OPACITY, MaterialDefaultValues::Opacity);
            layer.Data.AmbientOcclusion = LoadMaterialFloatData(currentMaterial, MATKEY_AMBIENT_OCCLUISION_FACTOR, MaterialDefaultValues::AmbientOcclusion);
            layer.Data.ClearCoat = LoadMaterialFloatData(currentMaterial, MATKEY_CLEARCOAT_FACTOR, MaterialDefaultValues::ClearCoat);
            layer.Data.ClearCoatRoughness = LoadMaterialFloatData(currentMaterial, MATKEY_CLEARCOAT_ROUGHNESS_FACTOR, MaterialDefaultValues::ClearCoatRoughness);
            layer.Data.SheenRoughness = LoadMaterialFloatData(currentMaterial, MATKEY_SHEEN_ROUGHNESS_FACTOR, MaterialDefaultValues::SheenRoughness);
            layer.Data.Transmission = LoadMaterialFloatData(currentMaterial, MATKEY_TRANSMISSION_FACTOR, MaterialDefaultValues::Transmission);
            layer.Data.IOR = LoadMaterialFloatData(currentMaterial, MATKEY_IOR, MaterialDefaultValues::IOR);
            layer.Data.Sheen = LoadMaterialFloatData(currentMaterial, MATKEY_SHEEN_FACTOR, MaterialDefaultValues::Sheen);
            layer.Data.Blend = MaterialDefaultValues::Blend;

            layer.EmissiveColor = LoadMaterialVec3Data(currentMaterial, MATKEY_EMISSION_COLOR, MaterialDefaultValues::EmissiveColor);
            meshSegment->Materials->InsertLayer(layer);
        }

        auto InsertTextures =
            [&](aiTextureType type, TextureType textureType, const std::string_view& semantic)
            {
                std::uint32_t textureCount = currentMaterial->GetTextureCount(type);

                if (textureCount <= 0)
                {
                    MOTION_CORE_WARN("Material {0} has no textures of type {1}.", meshSegment->Materials->GetName(), semantic);
                    for (std::uint32_t i = 0; i < MAX_MATERIAL_LAYERS; ++i)
                    {
                        meshSegment->Materials->InsertTexture(i, semantic, GetDefaultTexture(type));
                    }
                }
                else
                {
                    for (std::uint32_t i = 0; i < textureCount; ++i)
                    {
                        if (textureCount > MAX_MATERIAL_LAYERS)
                        {
                            MOTION_CORE_WARN("Material {0} has more textures than the maximum allowed layers ({1}). Skipping additional textures.", meshSegment->Materials->GetName(), MAX_MATERIAL_LAYERS);
                            break;
                        }
                        else
                        {
                            auto texture = LoadTextures(i, type, currentMaterial, textureType);
                            meshSegment->Materials->InsertTexture(i, semantic, texture);
                        }
                    }
                }
            };

        InsertTextures(MATKEY_BASE_COLOR_TEXTURE, TextureType::BaseColorTexture, UniformCache::BaseColorTextures);
        InsertTextures(MATKEY_METALLIC_TEXTURE, TextureType::MetallicTexture, UniformCache::MetallicTextures);
        InsertTextures(MATKEY_ROUGHNESS_TEXTURE, TextureType::RoughnessTexture, UniformCache::RoughnessTextures);
        InsertTextures(MATKEY_AMBIENT_OCCLUSION_TEXTURE, TextureType::AmbientOcclusionTexture, UniformCache::AmbientOcclusionTextures);
        InsertTextures(MATKEY_NORMAL_TEXTURE, TextureType::NormalTexture, UniformCache::NormalTextures);
        InsertTextures(MATKEY_OPACITY_TEXTURE, TextureType::OpacityTexture, UniformCache::OpacityTextures);

        InsertTextures(MATKEY_CLEARCOAT_TEXTURE, TextureType::ClearCoatTexture, UniformCache::ClearCoatTextures);
        InsertTextures(MATKEY_SHEEN_TEXTURE, TextureType::SheenTexture, UniformCache::SheenTextures);
        InsertTextures(MATKEY_TRANSMISSION_TEXTURE, TextureType::TransmissionTexture, UniformCache::TransmissionTextures);

        auto emissiveTexture = LoadTextures(0, MATKEY_EMISSION_TEXTURE, currentMaterial, TextureType::EmissiveTexture);
        if (emissiveTexture)
        {
            for (std::uint32_t i = 0; i < MAX_MATERIAL_LAYERS; i++)
            {
                meshSegment->Materials->GetLayer(i).EmissiveTexture = emissiveTexture;
                meshSegment->Materials->GetLayer(i).Textures[UniformCache::BlendMaskTextures] = CreateUnregisteredPlainTexture(10, 10, { 1.0f, 1.0f, 1.0f });
            }
        }
    }
}