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
        const aiScene* scene = importer.ReadFile(path.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace);
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
            std::filesystem::path outputFilePath = GetAvailableCopyName(std::filesystem::path(std::format("{}/{}/{}.glb", exportPathString, modelName, modelName)));
            std::filesystem::path finalOutputPath = std::filesystem::absolute(outputFilePath);
            aiScene* mutableScene;

            auto computeSceneBoundingBox =
                [](const aiScene* scene, glm::vec3& minBounds, glm::vec3& maxBounds)
                {
                    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
                        aiMesh* mesh = scene->mMeshes[m];
                        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                            const aiVector3D& vtx = mesh->mVertices[v];

                            minBounds.x = std::min(minBounds.x, vtx.x);
                            minBounds.y = std::min(minBounds.y, vtx.y);
                            minBounds.z = std::min(minBounds.z, vtx.z);

                            maxBounds.x = std::max(maxBounds.x, vtx.x);
                            maxBounds.y = std::max(maxBounds.y, vtx.y);
                            maxBounds.z = std::max(maxBounds.z, vtx.z);
                        }
                    }
                };

            auto normalizeModelScale =
                [](aiScene* scene, float scaleFactor, const glm::vec3& center)
                {
                    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
                        aiMesh* mesh = scene->mMeshes[m];
                        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
                            aiVector3D& vertex = mesh->mVertices[v];

                            vertex.x = (vertex.x - center.x) * scaleFactor;
                            vertex.y = (vertex.y - center.y) * scaleFactor;
                            vertex.z = (vertex.z - center.z) * scaleFactor;
                        }
                    }
                };

            glm::vec3 minBounds(FLT_MAX);
            glm::vec3 maxBounds(-FLT_MAX);

            computeSceneBoundingBox(const_cast<aiScene*>(scene), minBounds, maxBounds);

            glm::vec3 center = (maxBounds + minBounds) * 0.5f;
            glm::vec3 size = maxBounds - minBounds;
            float desiredSize = 1.0f; // e.g., fit in [-1, 1]
            float scaleFactor = desiredSize / std::max({ size.x, size.y, size.z });

            normalizeModelScale(const_cast<aiScene*>(scene), scaleFactor, center);

            if (ConvertToGLB(const_cast<aiScene*>(scene), finalOutputPath))
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

        Assimp::Exporter exporter{};
        std::uint32_t exportFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_OptimizeMeshes | aiProcess_OptimizeGraph;

        if (exporter.Export(scene, "glb2", outputPath.string(), exportFlags) != AI_SUCCESS)
        {
            MOTION_CORE_ERROR("Assimp Exporter Error: {0}", exporter.GetErrorString());
            return false;
        }

        for (std::uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            aiMaterial* material = scene->mMaterials[i];
            CopyTextures(material, aiTextureType_BASE_COLOR, outputPath);
            CopyTextures(material, aiTextureType_METALNESS, outputPath);
            CopyTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, outputPath);
            CopyTextures(material, aiTextureType_NORMALS, outputPath);
            CopyTextures(material, aiTextureType_AMBIENT_OCCLUSION, outputPath);
        }

        MOTION_CORE_INFO("Assimp Exporter: Scene successfully exported to {0} in glb format.", outputPath.string());
        return true;
    }

    /**
     * @brief Copies a texture file referenced by the given material to the specified output path.
     *
     * This function retrieves the texture path from the provided Assimp material for the specified
     * texture type. If the texture file exists, it is copied to the output directory. The material's
     * texture property is then updated to reference the new texture location. If the texture file
     * does not exist or an error occurs during copying, an error is logged and the function returns.
     *
     * @param material Pointer to the aiMaterial from which to retrieve the texture.
     * @param textureType The type of texture to copy (e.g., aiTextureType_DIFFUSE).
     * @param outputPath The directory to which the texture file should be copied.
     */
    void Importer::CopyTextures(aiMaterial* material, const aiTextureType textureType, const std::filesystem::path& outputPath)
    {
        aiString texturePath;
        if (material->GetTexture(textureType, 0, &texturePath) != AI_SUCCESS)
            return;

        std::filesystem::path sourcePath(texturePath.C_Str());
        if (!std::filesystem::exists(sourcePath))
            return;

        std::filesystem::path finalTextureOutputPath = outputPath / sourcePath.filename();
        try
        {
            std::filesystem::create_directories(outputPath);
            std::filesystem::copy_file(sourcePath, finalTextureOutputPath, std::filesystem::copy_options::overwrite_existing);

            const std::string resolvedTexturePath = finalTextureOutputPath.string();
            aiString resolvedTexturePathString(resolvedTexturePath.c_str());

            material->AddProperty(&resolvedTexturePathString, AI_MATKEY_TEXTURE(textureType, 0));
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to copy texture: {0}", e.what());
            return;
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
        std::uint32_t importFlags = aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices | aiProcess_CalcTangentSpace;
        const aiScene* scene = importer.ReadFile(outputPath.string(), importFlags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            return nullptr;
        }


        auto& assetManager = AssetManager::GetInstance();
        std::string modelName = std::format("SMSH_{}", outputPath.filename().stem().string());
        auto staticMesh = assetManager.Create<StaticMesh>(modelName, outputPath);

        std::vector<float> vertices;
        std::vector<std::uint32_t> indices;

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
            [&](const aiNode* meshNode, const std::uint32_t meshIndex, const aiMesh* mesh, const aiScene* scene)
            {
                for (std::uint32_t verticesIndex = 0; verticesIndex < mesh->mNumVertices; verticesIndex++)
                {
                    // Extract vertex data
                    const aiVector3D& vertexPoint = mesh->mVertices[verticesIndex];
                    vertices.insert(vertices.end(), { vertexPoint.x, vertexPoint.y, vertexPoint.z });

                    // Extract texture coordinates, normals, tangents, and bitangents
                    if (mesh->HasTextureCoords(0))
                    {
                        const aiVector3D& texCoord = mesh->mTextureCoords[0][verticesIndex];
                        vertices.insert(vertices.end(), { texCoord.x, texCoord.y });
                    }
                    else
                    {
                        vertices.insert(vertices.end(), { 0.0f, 0.0f });
                    }

                    if (mesh->HasNormals())
                    {
                        const aiVector3D& normal = mesh->mNormals[verticesIndex];
                        vertices.insert(vertices.end(), { normal.x, normal.y, normal.z });
                    }
                    else
                    {
                        vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });
                    }

                    if (mesh->HasTangentsAndBitangents())
                    {
                        const aiVector3D& tangent = mesh->mTangents[verticesIndex];
                        vertices.insert(vertices.end(), { tangent.x, tangent.y, tangent.z });

                        const aiVector3D& bitangent = mesh->mBitangents[verticesIndex];
                        vertices.insert(vertices.end(), { bitangent.x, bitangent.y, bitangent.z });
                    }
                    else
                    {
                        vertices.insert(vertices.end(), { 0.0f, 0.0f, 0.0f });

                    }
                }

                for (std::uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++)
                {
                    const aiFace& face = mesh->mFaces[faceIndex];
                    for (std::uint32_t index = 0; index < face.mNumIndices; index++)
                    {
                        indices.push_back(face.mIndices[index]);
                    }
                }

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
                meshSegment->MeshSelf = assetManager.Create<Mesh>(std::format("MSH_{}-{}", staticMesh->GetName(), meshIndex), vertices.data(), static_cast<std::int32_t>(vertices.size()), indices.data(), static_cast<std::int32_t>(indices.size()), layout, staticMesh);

                aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
                meshSegment->Materials = assetManager.Create<MaterialInstance>(std::format("MAT_{}-{}", staticMesh->GetName(), meshIndex), assetManager.Get<Material>("BaseMaterial"));

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
                vertices.clear();
                indices.clear();
            };

        auto convertMatrix =
            [](const aiMatrix4x4& aiMat) -> glm::mat4
            {
                return glm::transpose(glm::make_mat4(&aiMat.a1));
            };

        auto decomposeTransform =
            [&](const aiMatrix4x4& matrix, glm::vec3& position, glm::quat& rotation, glm::vec3& scale)
            {
                aiVector3D aiPos, aiScale;
                aiQuaternion aiRot;
                matrix.Decompose(aiScale, aiRot, aiPos);

                position = { aiPos.x, aiPos.y, aiPos.z };
                scale = { aiScale.x, aiScale.y, aiScale.z };
                rotation = { aiRot.w, aiRot.x, aiRot.y, aiRot.z }; // GLM uses (w, x, y, z)
            };

        std::function<void(aiNode*, const aiScene*)> loadNodes =
            [&](aiNode* node, const aiScene* scene)
            {
                for (std::uint32_t i = 0; i < node->mNumMeshes; i++)
                {
                    std::uint32_t meshIndex = node->mMeshes[i];
                    aiMesh* mesh = scene->mMeshes[meshIndex];
                    if (mesh)
                    {
                        loadMeshes(node, meshIndex, mesh, scene);
                    }
                }

                for (std::uint32_t i = 0; i < node->mNumChildren; i++)
                {
                    loadNodes(node->mChildren[i], scene);
                }
            };


        loadNodes(scene->mRootNode, scene);
        return staticMesh;
    }
}
