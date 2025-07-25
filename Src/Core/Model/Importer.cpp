#include "CorePCH.hpp"

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
        std::shared_ptr<StaticMesh> staticMesh = assetManager.Create<StaticMesh>(modelName, path);

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            staticMesh->AssetInfo.IsInitialized = false;
            return staticMesh;
        }
        else
        {
            MOTION_CORE_INFO("Assimp Importer: StaticMesh {0} successfully loaded to memory from {1}. Extracting mesh data...", modelName, path.string());
            LoadNode(staticMesh, scene->mRootNode, scene);
            staticMesh->AssetInfo.IsInitialized = true;
            return staticMesh;
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
            std::string textureFileName = textureFilePath.filename().stem().string();

            std::shared_ptr<ITexture> texture = assetManager.Create<ITexture>(textureFileName, textureFilePath, textureType, true);
            if (texture != nullptr)
            {
                MOTION_CORE_INFO("{0} Texture loading success from {1}", textureFileName, textureFilePath.string());
                return texture;
            }
            else
            {
                MOTION_CORE_ERROR("Failed to load texture from {0}. Getting default texture.", textureFilePath.string());
            }
        }
        else
        {
            MOTION_CORE_ERROR("Invalid texture property detected! Getting default texture for type: {0}", static_cast<int>(aiTexType));
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
        glm::vec3 data{ defaultValue };
        if (currentMaterial->Get(dataType, type, idx, data) == AI_SUCCESS)
        {
            MOTION_CORE_INFO("Found data type {0} with value ({1},{2},{3})", dataType, data.x, data.y, data.z);
            return data;
        }
        else
        {
            MOTION_CORE_WARN("Data type {0} not found, returning default value ({1},{2},{3})", dataType, defaultValue.x, defaultValue.y, defaultValue.z);
            return defaultValue;
        }
    }


    /**
     * @brief Loads mesh data from the specified node in the scene and constructs a mesh segment.
     *
     * Extracts vertex attributes (positions, texture coordinates, normals, tangents, bitangents) and indices
     * from the given aiMesh, normalizes tangents and bitangents if present, and creates a MeshSegment object.
     * The mesh segment is then associated with the provided StaticMesh and its materials are loaded.
     *
     * @param currentScene Pointer to the Assimp scene containing the mesh.
     * @param meshIndex Index of the mesh within the node.
     * @param currentMesh Pointer to the aiMesh to be processed.
     * @param staticMesh Shared pointer to the StaticMesh to which the mesh segment will be added.
     */
    void Importer::LoadCurrentNodeMeshes(const aiScene* currentScene, std::uint32_t meshIndex, aiMesh* currentMesh, const std::shared_ptr<StaticMesh>& staticMesh)
    {
        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        auto& assetManager = AssetManager::GetInstance();
        bool hasNormalizeTangents{ false };

        MOTION_CORE_INFO("Extracting StaticMesh SubMesh ({0}) Vertex and Indices data...", meshIndex);
        for (uint32_t i = 0; i < currentMesh->mNumVertices; i++)
        {
            // Position
            vertices.insert(vertices.end(), { currentMesh->mVertices[i].x, currentMesh->mVertices[i].y, currentMesh->mVertices[i].z });

            // TexCoords
            if (currentMesh->HasTextureCoords(0))
                vertices.insert(vertices.end(), { currentMesh->mTextureCoords[0][i].x, currentMesh->mTextureCoords[0][i].y });
            else
                vertices.insert(vertices.end(), { 0.0f, 0.0f });

            // Normals
            vertices.insert(vertices.end(), { -currentMesh->mNormals[i].x, -currentMesh->mNormals[i].y, -currentMesh->mNormals[i].z });

            // Tangents and Bitangents
            if (currentMesh->HasTangentsAndBitangents())
            {
                glm::vec3 tangent = glm::vec3(currentMesh->mTangents[i].x, currentMesh->mTangents[i].y, currentMesh->mTangents[i].z);
                glm::vec3 bitangent = glm::vec3(currentMesh->mBitangents[i].x, currentMesh->mBitangents[i].y, currentMesh->mBitangents[i].z);

                vertices.insert(vertices.end(), { tangent.x, tangent.y, tangent.z });
                vertices.insert(vertices.end(), { bitangent.x, bitangent.y, bitangent.z });
                hasNormalizeTangents = true;
            }
            else
            {
                // Calculate tangent and bitangent manually if not present
                // We'll use the previous and next vertex to estimate, or fallback to (1,0,0) and (0,1,0)
                glm::vec3 tangent(1.0f, 0.0f, 0.0f);
                glm::vec3 bitangent(0.0f, 1.0f, 0.0f);

                if (currentMesh->mNumFaces > 0 && currentMesh->mNumVertices >= 3) {
                    // Find a face this vertex belongs to
                    for (uint32_t f = 0; f < currentMesh->mNumFaces; ++f) {
                        const aiFace& face = currentMesh->mFaces[f];
                        for (uint32_t vi = 0; vi < face.mNumIndices; ++vi) {
                            if (face.mIndices[vi] == i && face.mNumIndices >= 3) {
                                // Get the three vertices of the triangle
                                uint32_t i0 = face.mIndices[0];
                                uint32_t i1 = face.mIndices[1];
                                uint32_t i2 = face.mIndices[2];

                                glm::vec3 v0(currentMesh->mVertices[i0].x, currentMesh->mVertices[i0].y, currentMesh->mVertices[i0].z);
                                glm::vec3 v1(currentMesh->mVertices[i1].x, currentMesh->mVertices[i1].y, currentMesh->mVertices[i1].z);
                                glm::vec3 v2(currentMesh->mVertices[i2].x, currentMesh->mVertices[i2].y, currentMesh->mVertices[i2].z);

                                glm::vec2 uv0(0.0f), uv1(0.0f), uv2(0.0f);
                                if (currentMesh->HasTextureCoords(0)) {
                                    uv0 = glm::vec2(currentMesh->mTextureCoords[0][i0].x, currentMesh->mTextureCoords[0][i0].y);
                                    uv1 = glm::vec2(currentMesh->mTextureCoords[0][i1].x, currentMesh->mTextureCoords[0][i1].y);
                                    uv2 = glm::vec2(currentMesh->mTextureCoords[0][i2].x, currentMesh->mTextureCoords[0][i2].y);
                                }

                                glm::vec3 edge1 = v1 - v0;
                                glm::vec3 edge2 = v2 - v0;
                                glm::vec2 deltaUV1 = uv1 - uv0;
                                glm::vec2 deltaUV2 = uv2 - uv0;

                                float f = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
                                if (fabs(f) < 1e-6f) f = 1.0f; // Prevent division by zero

                                float invF = 1.0f / f;

                                tangent = invF * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
                                bitangent = invF * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

                                vertices.insert(vertices.end(), { tangent.x, tangent.y, tangent.z });
                                vertices.insert(vertices.end(), { bitangent.x, bitangent.y, bitangent.z });
                            }
                        }
                    }
                }
            }
        }

        // Extracting Indices
        for (uint32_t i = 0; i < currentMesh->mNumFaces; i++)
        {
            aiFace face = currentMesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);
            }
        }

        std::shared_ptr<StaticMesh::MeshSegment> meshSegment = std::make_shared<StaticMesh::MeshSegment>();
        meshSegment->MeshSelf = assetManager.Create<Mesh>(
            std::format("{}-{}-{}", staticMesh->GetName(), currentMesh->mName.C_Str(), meshIndex),
            vertices.data(), static_cast<std::int32_t>(vertices.size()),
            indices.data(), static_cast<std::int32_t>(indices.size()),
            BufferLayout(
                {
                    { UniformCache::Position,   BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
                    { UniformCache::TexCoords,  BufferComponents::UV,  BufferStride::F2, false, offsetof(Vertex, TexCoord) },
                    { UniformCache::Normals,    BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
                    { UniformCache::Tangents,   BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent) },
                    { UniformCache::Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent) }
                }
            ),
            staticMesh
        );

        LoadMaterials(meshIndex, currentMesh->mMaterialIndex, meshSegment, currentScene);
        staticMesh->m_Meshes.emplace_back(std::move(meshSegment));
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
        for (std::uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; ++meshIndex)
        {
            MOTION_CORE_INFO("Loading MeshSegment - {0} from Node ({1})", meshIndex, node->mName.C_Str());
            LoadCurrentNodeMeshes(scene, meshIndex, scene->mMeshes[node->mMeshes[meshIndex]], staticMeshPtr);
        }

        for (std::uint32_t currentNode = 0; currentNode < node->mNumChildren; ++currentNode)
        {
            if (!node->mChildren[currentNode])
            {
                MOTION_CORE_WARN("Node {0} has a no child node. Skipping...", currentNode);
                continue;
            }

            LoadNode(staticMeshPtr, node->mChildren[currentNode], scene);
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
    void Importer::LoadMaterials(std::uint32_t meshIndex, std::uint32_t materialIndex, const std::shared_ptr<StaticMesh::MeshSegment>& meshSegment, const aiScene* scene)
    {
        MOTION_CORE_INFO("Extracting MeshSegment {0} Materials...", meshIndex);
        auto& assetManager = AssetManager::GetInstance();
        aiMaterial* currentMaterial = scene->mMaterials[materialIndex];
        aiString property;

        if (currentMaterial->Get(AI_MATKEY_NAME, property) != AI_SUCCESS)
        {
            MOTION_CORE_WARN("Material without a name is not been handled. Skipping material load for Mesh Segment {0}", meshIndex);
            return;
        }

        std::shared_ptr<Material> baseMaterial = assetManager.Get<Material>("M53A"); // <-- Change it later to plain material
        if (!baseMaterial)
        {
            MOTION_CORE_ERROR("Failed to create base Material for Mesh Segment {0} - Material Name ({1})", meshIndex, property.C_Str());
            meshSegment->MeshSelf.reset();
            meshSegment->Materials = nullptr;
            return;
        }

        meshSegment->Materials = assetManager.Create<MaterialInstance>(property.C_Str(), baseMaterial);
        if (!meshSegment->Materials)
        {
            MOTION_CORE_ERROR("Failed to create Material for Mesh Segment {0} - Material Name ({1})", meshIndex, property.C_Str());
            meshSegment->MeshSelf.reset();
            meshSegment->Materials = nullptr;
            return;
        }

        meshSegment->Materials->Attributes.BaseColor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_BASE_COLOR, { 1.0f, 1.0f, 1.0f });
        meshSegment->Materials->Attributes.Metallic = LoadMaterialFloatData(currentMaterial, AI_MATKEY_METALLIC_FACTOR, 0.0f);
        meshSegment->Materials->Attributes.Roughness = LoadMaterialFloatData(currentMaterial, AI_MATKEY_ROUGHNESS_FACTOR, 1.0f);
        meshSegment->Materials->Attributes.Opacity = LoadMaterialFloatData(currentMaterial, AI_MATKEY_OPACITY, 1.0f);
        meshSegment->Materials->Attributes.DisplacementScale = LoadMaterialFloatData(currentMaterial, AI_MATKEY_BUMPSCALING, 0.0f);
        meshSegment->Materials->Attributes.PADDING1 = 0.0f; // Ensure proper alignment
        meshSegment->Materials->Attributes.PADDING2 = 0.0f; // Ensure proper alignment

        auto loadTexture =
            [&](std::string_view name, std::uint32_t index, aiTextureType type, TextureType textureType)
            {
                std::shared_ptr<ITexture> texture = LoadTextures(index, type, currentMaterial, textureType);
                if (texture)
                    meshSegment->Materials->Texture[name] = texture;
            };

        loadTexture(UniformCache::BaseColorTextures, materialIndex, aiTextureType_BASE_COLOR, TextureType::BaseColorTexture);
        loadTexture(UniformCache::MetallicTextures, materialIndex, aiTextureType_METALNESS, TextureType::MetallicTexture);
        loadTexture(UniformCache::RoughnessTextures, materialIndex, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::RoughnessTexture);
        loadTexture(UniformCache::AmbientOcclusionTextures, materialIndex, aiTextureType_AMBIENT_OCCLUSION, TextureType::AmbientOcclusionTexture);
        loadTexture(UniformCache::NormalTextures, materialIndex, aiTextureType_NORMALS, TextureType::NormalTexture);
        loadTexture(UniformCache::DisplacementTextures, materialIndex, aiTextureType_DISPLACEMENT, TextureType::DisplacementTexture);
    }
}