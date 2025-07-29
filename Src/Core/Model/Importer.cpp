#include "CorePCH.hpp"

namespace Motion
{
    /**
     * @brief Generates a unique file path by appending "-copy" (and a number if needed) to the original file name.
     *
     * If the specified file path does not exist, returns the original path.
     * Otherwise, appends "-copy" to the file name (before the extension) and checks for existence.
     * If a file with that name already exists, appends an incrementing number (e.g., "-copy2", "-copy3", etc.)
     * until an available file name is found.
     *
     * @param originalPath The original file path to check and generate a unique copy name for.
     * @return std::filesystem::path A file path that does not currently exist in the file system.
     */
    static std::filesystem::path GetAvailableCopyName(const std::filesystem::path& originalPath) {
        if (!std::filesystem::exists(originalPath)) {
            return originalPath;
        }

        std::filesystem::path directory = originalPath.parent_path();
        std::string stem = originalPath.stem().string();  // file name without extension
        std::string extension = originalPath.extension().string();

        int counter = 1;
        std::filesystem::path newPath;

        do {
            newPath = directory / (stem + "-copy" + (counter > 1 ? std::to_string(counter) : "") + extension);
            ++counter;
        } while (std::filesystem::exists(newPath));

        return newPath;
    }


    /**
     * @brief Imports a 3D model from the specified file path and converts it to a StaticMesh.
     *
     * This function uses the Assimp library to read the model file, applies several processing steps
     * (triangulation, UV flipping, normal generation, vertex joining, tangent space calculation), and
     * then attempts to convert the loaded scene to the GLB format. If the conversion is successful,
     * it reads the resulting GLB file into a StaticMesh object, marks it as initialized, and returns it.
     * If any step fails, an error is logged and nullptr is returned.
     *
     * @param path The filesystem path to the model file to import.
     * @param exportPath The directory where the converted GLB file will be saved. If "default", uses "Assets/Models".
     * @return std::shared_ptr<StaticMesh> A shared pointer to the imported StaticMesh, or nullptr if import/conversion fails.
     */
    std::shared_ptr<StaticMesh> Importer::ImportModel(const std::filesystem::path& path, const std::string& exportPath)
    {
        auto& assetManager = AssetManager::GetInstance();

        Assimp::Importer importer;
        const std::uint32_t importFlags =
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_ImproveCacheLocality |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_ValidateDataStructure |
            aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(path.string(), importFlags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            return nullptr;
        }
        else
        {
            std::string modelName = path.filename().stem().string();
            MOTION_CORE_INFO("Assimp Importer: StaticMesh {0} file successfully loaded to memory from {1} > Converting...", modelName, path.string());

            std::filesystem::path appRoot = std::filesystem::absolute(std::filesystem::path(".")); // Adjust if needed
            std::string exportPathString = (exportPath == "default") ? (appRoot / "Assets/Models").string() : exportPath;

            std::filesystem::path finalOutputPath{};
            std::filesystem::path file = std::filesystem::path(std::format("{}/{}/{}.glb", exportPathString, modelName, modelName));
            if (!std::filesystem::exists(file))
            {
                std::filesystem::path outputFilePath = GetAvailableCopyName(file);
                finalOutputPath = std::filesystem::absolute(outputFilePath);
            }
            else
            {
                finalOutputPath = std::filesystem::absolute(file);
            }

            aiScene* mutableScene = importer.GetOrphanedScene();
            if (!mutableScene)
            {
                MOTION_CORE_ERROR("Failed to get mutable scene from Assimp importer.");
                return nullptr;
            }

            if (ConvertToGLB(mutableScene, finalOutputPath))
            {
                auto staticMesh = ReadGLB(finalOutputPath);
                if (staticMesh)
                {
                    staticMesh->AssetInfo.IsInitialized = true;
                    return staticMesh;
                }
                else
                {
                    MOTION_CORE_ERROR("Failed to read StaticMesh {0} from GLB format from {1}", modelName, finalOutputPath.string());
                    return nullptr;
                }
            }
            else
            {
                MOTION_CORE_ERROR("Failed to convert StaticMesh {0} to GLB format", modelName);
                return nullptr;
            }
        }

        return nullptr;
    }

    /**
     * @brief Converts an Assimp scene to the GLB format and exports it to the specified output path.
     *
     * This function uses the Assimp Exporter to export the provided scene to the GLB format (or another specified format).
     * After exporting, it iterates through all materials in the scene and copies associated textures (base color, metalness,
     * roughness, normals, and ambient occlusion) to the output path.
     *
     * @param scene Pointer to the Assimp scene to be exported.
     * @param outputPath The filesystem path where the exported file will be saved.
     * @param exportFormat The export format identifier (e.g., "gltf2").
     * @return true if the export and texture copying succeed, false otherwise.
     */
    bool Importer::ConvertToGLB(aiScene* scene, const std::filesystem::path& outputPath)
    {
        if (!std::filesystem::exists(outputPath))
        {
            try
            {
                std::filesystem::create_directories(outputPath.parent_path());
            }
            catch (const std::exception& e)
            {
                MOTION_CORE_ERROR("Failed to create output directory: {0}", e.what());
                return false;
            }

        }

        if (std::filesystem::exists(outputPath))
        {
            MOTION_CORE_INFO("Output file already exists: {0}", outputPath.string());
            return true;
        }

        Assimp::Exporter exporter{};
        std::uint32_t exportFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;

        if (exporter.Export(scene, "glb2", outputPath.string(), exportFlags) != AI_SUCCESS)
        {
            MOTION_CORE_ERROR("Assimp Exporter Error: {0}", exporter.GetErrorString());
            return false;
        }

        MOTION_CORE_INFO("Assimp Exporter: Scene successfully exported to {0} in glb format.", outputPath.string());
        return true;
    }

    /**
     * @brief Generates tangents and bitangents for the given vertices based on their positions and texture coordinates.
     *
     * This function computes tangents and bitangents for each triangle defined by the indices vector.
     * It uses the positions and texture coordinates of the vertices to calculate the tangent space vectors,
     * which are essential for normal mapping in 3D graphics.
     *
     * @param vertices A vector of Vertex structures containing position and texture coordinate data.
     * @param indices A vector of indices defining the triangles in the mesh.
     */
    static void GenerateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            Vertex& v0 = vertices[indices[i + 0]];
            Vertex& v1 = vertices[indices[i + 1]];
            Vertex& v2 = vertices[indices[i + 2]];

            glm::vec3 pos1 = v0.Position;
            glm::vec3 pos2 = v1.Position;
            glm::vec3 pos3 = v2.Position;

            glm::vec2 uv1 = v0.TexCoord;
            glm::vec2 uv2 = v1.TexCoord;
            glm::vec2 uv3 = v2.TexCoord;

            glm::vec3 edge1 = pos2 - pos1;
            glm::vec3 edge2 = pos3 - pos1;
            glm::vec2 deltaUV1 = uv2 - uv1;
            glm::vec2 deltaUV2 = uv3 - uv1;

            float f = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            if (fabs(f) < 1e-6f) f = 1.0f;
            else f = 1.0f / f;

            glm::vec3 tangent = f * (deltaUV2.y * edge1 - deltaUV1.y * edge2);
            glm::vec3 bitangent = f * (-deltaUV2.x * edge1 + deltaUV1.x * edge2);

            v0.Tangent += tangent;
            v1.Tangent += tangent;
            v2.Tangent += tangent;

            v0.Bitangent += bitangent;
            v1.Bitangent += bitangent;
            v2.Bitangent += bitangent;
        }

        // Normalize all tangents/bitangents
        for (auto& v : vertices)
        {
            v.Tangent = glm::normalize(v.Tangent);
            v.Bitangent = glm::normalize(v.Bitangent);
        }
    }

    /**
     * @brief Generates normals for the given vertices based on their positions and indices.
     *
     * This function computes vertex normals by averaging the normals of adjacent faces.
     * It iterates through the indices to form triangles, calculates face normals, and accumulates them
     * for each vertex. Finally, it normalizes the accumulated normals to produce smooth shading.
     *
     * @param vertices A vector of Vertex structures containing position and normal data.
     * @param indices A vector of indices defining the triangles in the mesh.
     */
    void GenerateNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        // Clear existing normals
        for (auto& v : vertices)
            v.Normal = glm::vec3(0.0f);

        // Accumulate face normals
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            glm::vec3& p0 = vertices[i0].Position;
            glm::vec3& p1 = vertices[i1].Position;
            glm::vec3& p2 = vertices[i2].Position;

            glm::vec3 edge1 = p1 - p0;
            glm::vec3 edge2 = p2 - p0;

            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

            vertices[i0].Normal += faceNormal;
            vertices[i1].Normal += faceNormal;
            vertices[i2].Normal += faceNormal;
        }

        // Normalize the result
        for (auto& v : vertices)
            v.Normal = glm::normalize(v.Normal);
    }

    /**
     * @brief Generates triplanar UV coordinates for each vertex based on its normal.
     *
     * This function computes UV coordinates for each vertex by projecting it onto the three principal axes
     * (X, Y, Z) and using the absolute value of the normal to determine which axis to use for the UV mapping.
     * It ensures that the UVs are normalized to the range [0, 1].
     *
     * @param vertices A vector of Vertex structures containing position and texture coordinate data.
     */
    static void GenerateTriplanarUVs(std::vector<Vertex>& vertices)
    {
        for (auto& vertex : vertices)
        {
            glm::vec3 n = glm::abs(vertex.Normal);

            if (n.x >= n.y && n.x >= n.z)
                vertex.TexCoord = glm::vec2(vertex.Position.y, vertex.Position.z);
            else if (n.y >= n.z)
                vertex.TexCoord = glm::vec2(vertex.Position.x, vertex.Position.z);
            else
                vertex.TexCoord = glm::vec2(vertex.Position.x, vertex.Position.y);

            vertex.TexCoord = vertex.TexCoord * 0.5f + 0.5f; // Normalize to [0, 1]
        }
    }

    /**
     * @brief Checks if a glm::vec2 or glm::vec3 contains finite values.
     *
     * This function checks if all components of the vector are finite (not NaN or Inf).
     * It is used to validate mesh attributes before processing.
     *
     * @param v The vector to check.
     * @return true if all components are finite, false otherwise.
     */
    static bool IsFinite(const glm::vec2& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y);
    }


    /**
     * @brief Checks if a glm::vec3 contains finite values.
     *
     * This function checks if all components of the vector are finite (not NaN or Inf).
     * It is used to validate mesh attributes before processing.
     *
     * @param v The vector to check.
     * @return true if all components are finite, false otherwise.
     */
    static bool IsFinite(const glm::vec3& v)
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    /**
     * @brief Validates and fixes mesh attributes such as normals, UVs, and tangents.
     *
     * This function checks if the mesh has valid normals, UVs, and tangents.
     * If any of these attributes are missing or invalid, it generates fallback values
     * (e.g., generating normals or triplanar UVs) to ensure the mesh is usable.
     *
     * @param mesh The Assimp mesh to validate.
     * @param vertices The vector of vertices to check and modify.
     * @param indices The vector of indices defining the mesh triangles.
     * @param meshIndex The index of the mesh being processed (for logging).
     */
    static void ValidateAndFixMeshAttributes(const aiMesh* mesh, std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t meshIndex)
    {
        bool regenerateNormals = false;
        bool regenerateUVs = false;
        bool regenerateTangents = false;

        // Validate Normals
        if (!mesh->HasNormals())
        {
            regenerateNormals = true;
        }
        else
        {
            bool allZero = true;
            for (auto& v : vertices)
            {
                if (!IsFinite(v.Normal) || glm::length(v.Normal) < 0.001f)
                {
                    regenerateNormals = true;
                    break;
                }
                if (glm::length(v.Normal) > 0.001f)
                    allZero = false;
            }
            if (allZero)
                regenerateNormals = true;
        }

        // Validate UVs
        if (!mesh->HasTextureCoords(0))
        {
            regenerateUVs = true;
        }
        else
        {
            bool uvHasVariation = false;
            glm::vec2 firstUV = vertices.front().TexCoord;

            for (const auto& v : vertices)
            {
                if (!IsFinite(v.TexCoord))
                {
                    regenerateUVs = true;
                    break;
                }
                if (glm::distance(v.TexCoord, firstUV) > 0.01f)
                    uvHasVariation = true;
            }
            if (!uvHasVariation)
                regenerateUVs = true;
        }

        // Validate Tangents and Bitangents
        if (!mesh->HasTangentsAndBitangents())
        {
            regenerateTangents = true;
        }
        else
        {
            bool allZero = true;
            for (auto& v : vertices)
            {
                if (!IsFinite(v.Tangent) || !IsFinite(v.Bitangent) ||
                    glm::length(v.Tangent) < 0.001f || glm::length(v.Bitangent) < 0.001f)
                {
                    regenerateTangents = true;
                    break;
                }
                if (glm::length(v.Tangent) > 0.001f || glm::length(v.Bitangent) > 0.001f)
                    allZero = false;

                // Optional: Gram-Schmidt orthogonalization
                v.Tangent = glm::normalize(v.Tangent - glm::dot(v.Tangent, v.Normal) * v.Normal);
                v.Bitangent = glm::normalize(glm::cross(v.Normal, v.Tangent));
            }
            if (allZero)
                regenerateTangents = true;
        }

        if (regenerateNormals)
        {
            MOTION_CORE_WARN("Mesh [{}] has invalid or missing normals — generating fallback normals...", meshIndex);
            GenerateNormals(vertices, indices);
        }

        if (regenerateUVs)
        {
            MOTION_CORE_WARN("Mesh [{}] has invalid or missing UVs — generating triplanar fallback...", meshIndex);
            GenerateTriplanarUVs(vertices);
        }

        if (regenerateTangents)
        {
            MOTION_CORE_WARN("Mesh [{}] has invalid or missing tangents — generating...", meshIndex);
            GenerateTangents(vertices, indices);
        }
    }


    /**
     * @brief Imports a GLB file and constructs a StaticMesh object from its contents.
     *
     * This function reads a GLB (.glb) file from the specified path, parses its mesh and material data
     * using the Assimp library, and creates a StaticMesh asset containing mesh segments and material instances.
     * It supports extraction of vertex positions, texture coordinates, normals, tangents, and bitangents,
     * as well as material properties such as base color, metallic, roughness, opacity, and associated textures.
     *
     * @param outputPath The filesystem path to the GLB file to import.
     * @return std::shared_ptr<StaticMesh> A shared pointer to the imported StaticMesh object,
     *         or nullptr if the import fails due to file errors or parsing issues.
     *
     * @note This function logs errors using MOTION_CORE_ERROR if the file does not exist, has an invalid extension,
     *       or if Assimp fails to import the file. It also handles missing materials by assigning default values.
     */
    std::shared_ptr<StaticMesh> Importer::ReadGLB(const std::filesystem::path& outputPath)
    {
        if (!std::filesystem::exists(outputPath))
        {
            MOTION_CORE_ERROR("GLB file does not exist: {0}", outputPath.string());
            return nullptr;
        }

        if (outputPath.extension() != ".glb")
        {
            MOTION_CORE_ERROR("Invalid GLB file extension: {0}", outputPath.extension().string());
            return nullptr;
        }

        Assimp::Importer importer{};
        const std::uint32_t importFlags =
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_ImproveCacheLocality |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_ValidateDataStructure |
            aiProcess_FlipUVs;

        const aiScene* scene = importer.ReadFile(outputPath.string(), importFlags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            return nullptr;
        }
        else
        {
            aiScene* mutableScene = importer.GetOrphanedScene();
            if (!mutableScene)
            {
                MOTION_CORE_ERROR("Failed to get mutable scene from Assimp importer.");
                return nullptr;
            }

            auto& assetManager = AssetManager::GetInstance();
            std::string modelName = std::format("SMSH_{}", outputPath.filename().stem().string());
            auto staticMesh = assetManager.Create<StaticMesh>(modelName, outputPath);

            auto getMaterialFloat =
                [](aiMaterial* material, const char* key, int type, int idx, float defaultValue) -> float
                {
                    float value{ 0.0f };
                    if (material->Get(key, type, idx, value) == AI_SUCCESS)
                        return value;

                    return defaultValue;
                };

            auto getMaterialVec3Data =
                [](aiMaterial* material, const char* key, int type, int idx, const glm::vec3& defaultValue) -> glm::vec3
                {
                    aiVector3D value{ 0.0f, 0.0f, 0.0f };
                    if (material->Get(key, type, idx, value) == AI_SUCCESS)
                        return { value.x, value.y, value.z };

                    return { defaultValue.x, defaultValue.y, defaultValue.z };
                };

            auto getMaterialTexture =
                [&](aiMaterial* material, aiTextureType assimpTextureType, TextureType engineTextureType) -> std::shared_ptr<ITexture>
                {
                    aiString texturePath;
                    if (material->GetTexture(assimpTextureType, 0, &texturePath) == AI_SUCCESS)
                    {
                        std::filesystem::path sourcePath(texturePath.C_Str());
                        if (std::filesystem::exists(sourcePath))
                        {
                            std::string textureName = sourcePath.filename().stem().string();
                            return assetManager.Create<ITexture>(textureName, sourcePath, engineTextureType, true);
                        }
                        else
                        {
                            MOTION_CORE_ERROR("Texture file does not exist: {0}", sourcePath.string());
                        }
                    }

                    return nullptr;
                };

            auto loadMeshes =
                [&](aiNode* meshNode, std::uint32_t meshIndex, aiMesh* mesh, aiScene* scene)
                {
                    std::vector<Vertex> vertices;
                    std::vector<std::uint32_t> indices;

                    for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
                    {
                        const aiFace& face = mesh->mFaces[faceIndex];

                        for (uint32_t i = 0; i < face.mNumIndices; i++)
                        {
                            uint32_t idx = face.mIndices[i];

                            // Extract vertex data
                            Vertex vertex{};
                            vertex.Position = { mesh->mVertices[idx].x, mesh->mVertices[idx].y, mesh->mVertices[idx].z };

                            // Extract texture coordinates, normals, tangents, and bitangents
                            if (mesh->HasTextureCoords(0))
                            {
                                vertex.TexCoord = { mesh->mTextureCoords[0][idx].x, mesh->mTextureCoords[0][idx].y };
                            }

                            if (mesh->HasNormals())
                            {
                                const aiVector3D& normal = mesh->mNormals[idx];
                                vertex.Normal = { normal.x, normal.y, normal.z };
                            }

                            if (mesh->HasTangentsAndBitangents())
                            {
                                vertex.Tangent = { mesh->mTangents[idx].x, mesh->mTangents[idx].y, mesh->mTangents[idx].z };
                                vertex.Bitangent = { mesh->mBitangents[idx].x, mesh->mBitangents[idx].y, mesh->mBitangents[idx].z };
                            }

                            vertices.push_back(vertex);
                            indices.push_back(static_cast<uint32_t>(vertices.size() - 1));
                        }
                    }


                    // for (std::uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
                    // {
                    //     const aiFace& face = mesh->mFaces[faceIndex];
                    //     for (std::uint32_t index = 0; index < face.mNumIndices; index++)
                    //     {
                    //         indices.push_back(face.mIndices[index]);
                    //     }
                    // }


                    ValidateAndFixMeshAttributes(mesh, vertices, indices, meshIndex);

                    BufferLayout layout
                    (
                        {
                            { UniformCache::Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
                            { UniformCache::TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
                            { UniformCache::Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
                            { UniformCache::Tangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent) },
                            { UniformCache::Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent) }
                        }
                    );

                    auto meshSegment = std::make_shared<StaticMesh::MeshSegment>();
                    meshSegment->MeshSelf = assetManager.Create<Mesh>(
                        std::format("MSH_{}-{}", staticMesh->GetName(), meshIndex),
                        vertices.data(), static_cast<std::uint32_t>(vertices.size()),
                        indices.data(), static_cast<std::uint32_t>(indices.size()),
                        layout, staticMesh);

                    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
                    meshSegment->Materials = assetManager.Create<MaterialInstance>(
                        std::format("MAT_{}-{}", staticMesh->GetName(), meshIndex), assetManager.Get<Material>("BaseMaterial"));

                    if (!material)
                    {
                        // If no material is found, use a default material (base materials)
                        meshSegment->Materials->Attributes.BaseColor = { 1.0f, 1.0f, 1.0f };
                        meshSegment->Materials->Attributes.Metallic = 0.0f;
                        meshSegment->Materials->Attributes.Roughness = 1.0f;
                        meshSegment->Materials->Attributes.Opacity = 1.0f;
                        meshSegment->Materials->Attributes.AmbientOcclusion = 1.0f;

                        meshSegment->Materials->Texture[UniformCache::BaseColorTextures] = nullptr;
                        meshSegment->Materials->Texture[UniformCache::MetallicTextures] = nullptr;
                        meshSegment->Materials->Texture[UniformCache::RoughnessTextures] = nullptr;
                        meshSegment->Materials->Texture[UniformCache::AmbientOcclusionTextures] = nullptr;
                        meshSegment->Materials->Texture[UniformCache::NormalTextures] = nullptr;
                    }
                    else
                    {
                        meshSegment->Materials->Attributes.BaseColor = getMaterialVec3Data(material, AI_MATKEY_COLOR_DIFFUSE, { 1.0f, 1.0f, 1.0f });
                        meshSegment->Materials->Attributes.Metallic = getMaterialFloat(material, AI_MATKEY_METALLIC_FACTOR, 0.0f);
                        meshSegment->Materials->Attributes.Roughness = getMaterialFloat(material, AI_MATKEY_ROUGHNESS_FACTOR, 1.0f);
                        meshSegment->Materials->Attributes.Opacity = getMaterialFloat(material, AI_MATKEY_OPACITY, 1.0f);
                        meshSegment->Materials->Attributes.AmbientOcclusion = 1.0f; // Assimp does not provide ambient occlusion factor, set to 1.0f

                        meshSegment->Materials->Texture[UniformCache::BaseColorTextures] = getMaterialTexture(material, aiTextureType_BASE_COLOR, TextureType::BaseColorTexture);
                        meshSegment->Materials->Texture[UniformCache::MetallicTextures] = getMaterialTexture(material, aiTextureType_METALNESS, TextureType::MetallicTexture);
                        meshSegment->Materials->Texture[UniformCache::RoughnessTextures] = getMaterialTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::RoughnessTexture);
                        meshSegment->Materials->Texture[UniformCache::AmbientOcclusionTextures] = getMaterialTexture(material, aiTextureType_AMBIENT_OCCLUSION, TextureType::AmbientOcclusionTexture);
                        meshSegment->Materials->Texture[UniformCache::NormalTextures] = getMaterialTexture(material, aiTextureType_NORMALS, TextureType::NormalTexture);
                    }

                    meshSegment->MeshIndex = meshIndex;
                    staticMesh->m_Meshes.emplace_back(meshSegment);
                };

            std::function<void(aiNode*, aiScene*)> loadNodes =
                [&](aiNode* node, aiScene* scene)
                {
                    for (std::uint32_t i = 0; i < node->mNumMeshes; i++)
                    {
                        std::uint32_t meshIndex = node->mMeshes[i];
                        aiMesh* mesh = scene->mMeshes[meshIndex];
                        if (mesh)
                        {
                            loadMeshes(node, meshIndex, mesh, mutableScene);
                        }
                    }

                    for (std::uint32_t i = 0; i < node->mNumChildren; i++)
                    {
                        loadNodes(node->mChildren[i], mutableScene);
                    }
                };


            loadNodes(mutableScene->mRootNode, mutableScene);
            return staticMesh;
        }
    }
}
