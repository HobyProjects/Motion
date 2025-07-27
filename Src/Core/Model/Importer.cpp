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

    static const char* GetTextureTypeString(aiTextureType aiTexture)
    {
        switch (aiTexture)
        {
        case aiTextureType_BASE_COLOR: return "Base Color Texture";
        case aiTextureType_METALNESS: return "Metallic Texture";
        case aiTextureType_DIFFUSE_ROUGHNESS: return "Roughness Texture";
        case aiTextureType_AMBIENT_OCCLUSION: return "Ambient Occlusion Texture";
        case aiTextureType_NORMALS: return "Normal Texture";
        case aiTextureType_DISPLACEMENT: return "Displacement Texture";
        default: return "Unknown Texture Type";
        }
    }

    /**
     * @brief Loads a texture from an Assimp material property.
     *
     * This function attempts to retrieve a texture from the specified material property
     * using the provided data type, type, and index. If the property is found, a shared pointer
     * to the loaded texture is returned. Otherwise, a warning is logged and nullptr is returned.
     *
     * @param currentMaterial Pointer to the aiMaterial from which to load the texture.
     * @param dataType The name of the material property to retrieve (e.g., MATKEY_TEXTURE_DIFFUSE).
     * @param type The type of the texture (e.g., aiTextureType_DIFFUSE).
     * @param idx The index of the texture if there are multiple entries.
     * @return std::shared_ptr<ITexture> A shared pointer to the loaded texture, or nullptr if not found.
     */
    static std::shared_ptr<ITexture> LoadMaterialTextureData(aiMaterial* currentMaterial, TextureType textureType, aiTextureType aiTexture)
    {
        aiString property;
        if (currentMaterial->GetTexture(aiTexture, 0, &property) == AI_SUCCESS)
        {
            MOTION_CORE_INFO("Found texture {0} for data type {1}", property.C_Str(), GetTextureTypeString(aiTexture));
            std::filesystem::path texturePath = property.C_Str();
            std::string fileName = texturePath.filename().string();

            return AssetManager::GetInstance().Create<ITexture>(fileName, texturePath, textureType, true);
        }
        else
        {
            MOTION_CORE_WARN("Texture for data type {0} not found. Manual loading might be required", GetTextureTypeString(aiTexture));
            return nullptr;
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
        std::vector<glm::vec3> tempVertices{};
        std::vector<glm::vec2> tempUVs{};
        std::vector<glm::vec3> tempNormals{};
        std::vector<glm::vec3> tempTangents{};
        std::vector<glm::vec3> tempBitangents{};

        std::vector<float> finalVertices{};
        std::vector<uint32_t> finalIndices{};

        // 1. Extract position, UV, normal
        MOTION_CORE_INFO("Extracting MeshSegment - {0} from Mesh ({1})", meshIndex, currentMesh->mName.C_Str());
        for (uint32_t i = 0; i < currentMesh->mNumVertices; i++)
        {
            aiVector3D pos = currentMesh->mVertices[i];
            tempVertices.emplace_back(pos.x, pos.y, pos.z);

            if (currentMesh->HasTextureCoords(0))
            {
                aiVector3D tex = currentMesh->mTextureCoords[0][i];
                tempUVs.emplace_back(tex.x, tex.y);
            }
            else
            {
                tempUVs.emplace_back(0.0f, 0.0f);
            }

            aiVector3D n = currentMesh->mNormals[i];
            tempNormals.emplace_back(-n.x, -n.y, -n.z); // Flip YZ if needed
        }

        // 2. Extract indices
        MOTION_CORE_INFO("Extracting indices for MeshSegment - {0} from Mesh ({1})", meshIndex, currentMesh->mName.C_Str());
        for (std::uint32_t i = 0; i < currentMesh->mNumFaces; ++i)
        {
            const aiFace& face = currentMesh->mFaces[i];
            if (face.mNumIndices != 3) continue; // Only triangles
            finalIndices.push_back(face.mIndices[0]);
            finalIndices.push_back(face.mIndices[1]);
            finalIndices.push_back(face.mIndices[2]);
        }

        // 3. Generate tangents and bitangents
        tempTangents.resize(tempVertices.size(), glm::vec3(0.0f));
        tempBitangents.resize(tempVertices.size(), glm::vec3(0.0f));

        if (currentMesh->HasTangentsAndBitangents())
        {
            MOTION_CORE_INFO("MeshSegment - {0} has tangents and bitangents. Extracting...", meshIndex);
            for (uint32_t i = 0; i < currentMesh->mNumVertices; i++)
            {
                aiVector3D t = currentMesh->mTangents[i];
                aiVector3D b = currentMesh->mBitangents[i];
                tempTangents[i] = glm::vec3(t.x, t.y, t.z);
                tempBitangents[i] = glm::vec3(b.x, b.y, b.z);
            }
        }
        else
        {
            MOTION_CORE_INFO("MeshSegment - {0} does not have tangents and bitangents. Generating...", meshIndex);
            for (size_t i = 0; i < finalIndices.size(); i += 3)
            {
                uint32_t i0 = finalIndices[i];
                uint32_t i1 = finalIndices[i + 1];
                uint32_t i2 = finalIndices[i + 2];

                glm::vec3& p0 = tempVertices[i0];
                glm::vec3& p1 = tempVertices[i1];
                glm::vec3& p2 = tempVertices[i2];

                glm::vec2& uv0 = tempUVs[i0];
                glm::vec2& uv1 = tempUVs[i1];
                glm::vec2& uv2 = tempUVs[i2];

                glm::vec3 edge1 = p1 - p0;
                glm::vec3 edge2 = p2 - p0;

                glm::vec2 deltaUV1 = uv1 - uv0;
                glm::vec2 deltaUV2 = uv2 - uv0;

                float f = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
                f = (f == 0.0f) ? 1.0f : 1.0f / f;

                glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
                glm::vec3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

                tempTangents[i0] += tangent;
                tempTangents[i1] += tangent;
                tempTangents[i2] += tangent;

                tempBitangents[i0] += bitangent;
                tempBitangents[i1] += bitangent;
                tempBitangents[i2] += bitangent;
            }

            // 4. Normalize and orthonormalize per vertex
            MOTION_CORE_INFO("Normalizing and orthonormalizing tangents and bitangents for MeshSegment - {0}", meshIndex);
            for (size_t i = 0; i < tempVertices.size(); ++i)
            {
                glm::vec3& n = tempNormals[i];
                glm::vec3& t = tempTangents[i];
                glm::vec3& b = tempBitangents[i];

                t = glm::normalize(t - n * glm::dot(n, t));
                b = glm::normalize(glm::cross(n, t));
            }
        }

        // 5. Final vertex packing
        MOTION_CORE_INFO("Packing final vertex data for MeshSegment - {0}", meshIndex);
        for (size_t i = 0; i < tempVertices.size(); ++i)
        {
            const glm::vec3& position = tempVertices[i];
            const glm::vec2& uv = tempUVs[i];
            const glm::vec3& normal = tempNormals[i];
            const glm::vec3& tangent = tempTangents[i];
            const glm::vec3& bitangent = tempBitangents[i];

            finalVertices.insert(finalVertices.end(), { position.x, position.y, position.z });
            finalVertices.insert(finalVertices.end(), { uv.x, uv.y });
            finalVertices.insert(finalVertices.end(), { normal.x, normal.y, normal.z });
            finalVertices.insert(finalVertices.end(), { tangent.x, tangent.y, tangent.z });
            finalVertices.insert(finalVertices.end(), { bitangent.x, bitangent.y, bitangent.z });
        }

        // 6. Create MeshSegment and assign to StaticMesh
        MOTION_CORE_INFO("Creating MeshSegment for Mesh ({0}) with index {1}", currentMesh->mName.C_Str(), meshIndex);
        auto& assetManager = AssetManager::GetInstance();
        std::shared_ptr<StaticMesh::MeshSegment> meshSegment = std::make_shared<StaticMesh::MeshSegment>();
        meshSegment->MeshSelf = assetManager.Create<Mesh>(
            std::format("{}-{}-{}", staticMesh->GetName(), currentMesh->mName.C_Str(), meshIndex),
            finalVertices.data(), static_cast<std::int32_t>(finalVertices.size()),
            finalIndices.data(), static_cast<std::int32_t>(finalIndices.size()),
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

        MOTION_CORE_INFO("Adding MeshSegment to StaticMesh ({0})", staticMesh->GetName());
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
        for (std::uint32_t meshIndex = 0; meshIndex < node->mNumMeshes; meshIndex++)
        {
            MOTION_CORE_INFO("Loading MeshSegment - {0} from Node ({1})", meshIndex, node->mName.C_Str());
            LoadCurrentNodeMeshes(scene, meshIndex, scene->mMeshes[node->mMeshes[meshIndex]], staticMeshPtr);
        }

        for (std::uint32_t currentNode = 0; currentNode < node->mNumChildren; currentNode++)
        {
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
        MOTION_CORE_INFO("Extracting {0} Materials...", meshSegment->MeshSelf->GetName());
        auto& assetManager = AssetManager::GetInstance();
        aiMaterial* currentMaterial = scene->mMaterials[materialIndex];
        aiString property;

        std::shared_ptr<Material> baseMaterial = assetManager.Get<Material>("BaseMaterial");
        if (!baseMaterial)
        {
            MOTION_CORE_ERROR("Failed to create base Material for Mesh Segment {0} - Material Name ({1})", meshIndex, property.C_Str());
            meshSegment->MeshSelf.reset();
            meshSegment->Materials = nullptr;
            return;
        }

        meshSegment->Materials = assetManager.Create<MaterialInstance>(std::format("Material_{}", meshSegment->MeshSelf->GetName()), baseMaterial);
        if (!meshSegment->Materials)
        {
            MOTION_CORE_ERROR("Failed to create Material for Mesh Segment {0} - Material Name ({1})", meshIndex, property.C_Str());
            meshSegment->MeshSelf.reset();
            meshSegment->Materials = nullptr;
            return;
        }

        meshSegment->Materials->Attributes.BaseColor = LoadMaterialVec3Data(currentMaterial, AI_MATKEY_COLOR_DIFFUSE, { 1.0f, 1.0f, 1.0f });
        meshSegment->Materials->Attributes.Metallic = LoadMaterialFloatData(currentMaterial, AI_MATKEY_METALLIC_FACTOR, 0.0f);
        meshSegment->Materials->Attributes.Roughness = LoadMaterialFloatData(currentMaterial, AI_MATKEY_ROUGHNESS_FACTOR, 1.0f);
        meshSegment->Materials->Attributes.Opacity = LoadMaterialFloatData(currentMaterial, AI_MATKEY_OPACITY, 1.0f);

        meshSegment->Materials->Texture[UniformCache::BaseColorTextures] = LoadMaterialTextureData(currentMaterial, TextureType::BaseColorTexture, aiTextureType_BASE_COLOR);
        meshSegment->Materials->Texture[UniformCache::MetallicTextures] = LoadMaterialTextureData(currentMaterial, TextureType::MetallicTexture, aiTextureType_METALNESS);
        meshSegment->Materials->Texture[UniformCache::RoughnessTextures] = LoadMaterialTextureData(currentMaterial, TextureType::RoughnessTexture, aiTextureType_DIFFUSE_ROUGHNESS);
        meshSegment->Materials->Texture[UniformCache::AmbientOcclusionTextures] = LoadMaterialTextureData(currentMaterial, TextureType::AmbientOcclusionTexture, aiTextureType_AMBIENT_OCCLUSION);
        meshSegment->Materials->Texture[UniformCache::NormalTextures] = LoadMaterialTextureData(currentMaterial, TextureType::NormalTexture, aiTextureType_NORMALS);
        meshSegment->Materials->Texture[UniformCache::DisplacementTextures] = LoadMaterialTextureData(currentMaterial, TextureType::DisplacementTexture, aiTextureType_DISPLACEMENT);
    }
}