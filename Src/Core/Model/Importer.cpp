#include "CorePCH.hpp"

namespace Motion
{
    /**
     * @brief Generates a unique file path by appending "-copy" and a counter to the original file name if the file already exists.
     *
     * This function checks if the given file path exists. If it does not exist, the original path is returned.
     * If the file exists, it generates a new file name by appending "-copy" (and a counter if necessary) to the stem of the original file name,
     * ensuring that the returned path does not already exist in the file system.
     *
     * @param originalPath The original file path to check and generate a unique copy name for.
     * @return std::filesystem::path A unique file path that does not exist in the file system.
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
     * @brief Imports a 3D model file into the engine using Assimp and exports it to the specified path.
     *
     * This function reads a model file from the given path using Assimp with a set of processing flags
     * (triangulation, smooth normals generation, tangent space calculation, cache locality improvement,
     * redundant material removal, data structure validation, and UV flipping). If the import is successful,
     * it exports the model to the specified export path using the Exporter::ExportModel function.
     *
     * @param path The filesystem path to the source model file to import.
     * @param exportPath The filesystem path where the imported model should be exported.
     * @return true if the model was successfully imported and exported; false otherwise.
     */
    static bool ImportToEngine(const std::filesystem::path& path, const std::filesystem::path& exportPath)
    {
        Assimp::Importer importer;
        const std::uint32_t importFlags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_ImproveCacheLocality | aiProcess_RemoveRedundantMaterials | aiProcess_ValidateDataStructure | aiProcess_FlipUVs;
        const aiScene* scene = importer.ReadFile(path.string(), importFlags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            MOTION_CORE_ERROR("Assimp Importer Error: {0}", importer.GetErrorString());
            return false;
        }

        return false;
    }

    /**
     * @brief Generates box projection UV coordinates for a set of vertices.
     *
     * This function computes UV texture coordinates for each vertex in the input vector
     * using box projection mapping. The dominant axis of the vertex normal determines
     * the projection plane (XY, XZ, or YZ). The resulting UVs are normalized to fit
     * within the [0, 1] range based on the minimum and maximum projected coordinates.
     *
     * @param vertices Reference to a vector of Vertex objects. Each Vertex must have
     *                 Position (glm::vec3), Normal (glm::vec3), and TexCoord (glm::vec2) members.
     */
    void GenerateBoxProjectionUVs(std::vector<Vertex>& vertices)
    {
        if (vertices.empty()) return;

        glm::vec2 minUV(FLT_MAX), maxUV(-FLT_MAX);
        std::vector<glm::vec2> projected(vertices.size());

        for (size_t i = 0; i < vertices.size(); ++i) {
            const glm::vec3& pos = vertices[i].Position;
            glm::vec3 n = glm::abs(glm::normalize(vertices[i].Normal));
            glm::vec2 uv;
            // Choose projection plane based on dominant normal axis
            if (n.x >= n.y && n.x >= n.z)
                uv = { pos.y, pos.z }; // YZ
            else if (n.y >= n.x && n.y >= n.z)
                uv = { pos.x, pos.z }; // XZ
            else
                uv = { pos.x, pos.y }; // XY

            projected[i] = uv;
            minUV = glm::min(minUV, uv);
            maxUV = glm::max(maxUV, uv);
        }

        glm::vec2 range = glm::max(maxUV - minUV, glm::vec2(1e-5f));
        for (size_t i = 0; i < vertices.size(); ++i)
            vertices[i].TexCoord = (projected[i] - minUV) / range;
    }


    /**
     * @brief Generates per-vertex normals for a mesh given its vertices and triangle indices.
     *
     * This function computes smooth normals for each vertex by accumulating the normalized face normals
     * of all triangles sharing the vertex, then normalizes the result. It assumes that the mesh is
     * defined by a list of vertices and a list of triangle indices (each group of three indices forms a triangle).
     *
     * @param vertices Reference to a vector of Vertex objects. The Normal field of each vertex will be updated.
     * @param indices  Reference to a vector of triangle indices (uint32_t). Each consecutive group of three indices defines a triangle.
     */
    static void GenerateNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        // Clear existing normals
        for (auto& v : vertices)
            v.Normal = glm::vec3(0.0f);

        // Accumulate face normals
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t I0 = indices[i];
            uint32_t I1 = indices[i + 1];
            uint32_t I2 = indices[i + 2];

            glm::vec3& P0 = vertices[I0].Position;
            glm::vec3& P1 = vertices[I1].Position;
            glm::vec3& P2 = vertices[I2].Position;

            glm::vec3 E1 = P1 - P0;
            glm::vec3 E2 = P2 - P0;

            glm::vec3 faceNormal = glm::normalize(glm::cross(E1, E2));

            vertices[I0].Normal += faceNormal;
            vertices[I1].Normal += faceNormal;
            vertices[I2].Normal += faceNormal;
        }

        // Normalize the result
        for (auto& v : vertices)
            v.Normal = glm::normalize(v.Normal);
    }

    /**
     * @brief Retrieves a material attribute from an Assimp material.
     *
     * This templated function attempts to extract a material attribute of type T from the given aiMaterial.
     * If the material pointer is null or the attribute cannot be retrieved, the provided default value is returned.
     * Error and warning messages are logged accordingly.
     *
     * @tparam T Type of the attribute to retrieve.
     * @param material Pointer to the aiMaterial from which to retrieve the attribute.
     * @param key The key identifying the attribute.
     * @param type The type of the attribute (Assimp-specific).
     * @param index The index of the attribute (for attributes that may have multiple values).
     * @param defaultValue The value to return if the attribute cannot be retrieved.
     * @return The retrieved attribute value, or defaultValue if retrieval fails.
     */
    template<typename T>
    static T GetMaterialAttribute(const aiMaterial* material, const char* key, std::uint32_t type, std::uint32_t index, T defaultValue)
    {
        if (!material)
        {
            MOTION_CORE_ERROR("Material is null, cannot get attribute: {}", key);
            return defaultValue;
        }

        T value{ defaultValue };
        if (material->Get(key, type, index, value) != aiReturn_SUCCESS)
        {
            MOTION_CORE_WARN("Failed to get material attribute: {}. Using default value.", key);
            return defaultValue;
        }

        return value;
    }

    static std::pair<glm::vec3, glm::vec3> ComputeGlobalBounds(const aiScene* scene)
    {
        glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

        for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
        {
            const aiMesh* mesh = scene->mMeshes[m];
            for (uint32_t v = 0; v < mesh->mNumVertices; ++v)
            {
                glm::vec3 pos(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
                min = glm::min(min, pos);
                max = glm::max(max, pos);
            }
        }

        return { min, max };
    }

    /**
     * @brief Generates a hexadecimal hash string from the given input string.
     *
     * This function computes the hash of the input string using std::hash,
     * and formats the resulting hash value as an uppercase hexadecimal string.
     *
     * @param input The string to be hashed.
     * @return A std::string containing the hexadecimal representation of the hash.
     */
    static std::string HashString(const std::string& input)
    {
        return std::format("{:X}", std::hash<std::string>{}(input));
    }

    namespace MikkTSpace
    {
        struct MeshMikkTSpaceAdapter
        {
            std::vector<Vertex>& Vertices;
            const std::vector<uint32_t>& Indices;
        };

        /**
         * @brief Returns the number of faces in the mesh.
         *
         * This function calculates the number of faces by dividing the total number of indices
         * by 3, assuming that each face is represented by a triangle (3 indices per face).
         *
         * @param context Pointer to the SMikkTSpaceContext containing mesh data.
         * @return Number of faces in the mesh as a 32-bit integer.
         */
        static std::int32_t GetNumFaces(const SMikkTSpaceContext* context)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            return static_cast<std::int32_t>(adapter->Indices.size() / 3);
        }

        /**
         * @brief Returns the number of vertices for a face.
         *
         * This function always returns 3, indicating that each face is assumed to be a triangle.
         *
         * @param context Pointer to the SMikkTSpaceContext (unused).
         * @param faceIndex Index of the face (unused).
         * @return Number of vertices in the face (always 3).
         */
        static std::int32_t GetNumVerticesOfFace(const SMikkTSpaceContext*, std::int32_t)
        {
            return 3;
        }

        /**
         * @brief Retrieves the position of a vertex for a given face and vertex index from the mesh adapter.
         *
         * This function is used by the SMikkTSpace library to access the position of a vertex in the mesh.
         * It extracts the vertex index from the adapter's index buffer and copies the position coordinates
         * into the provided array.
         *
         * @param context Pointer to the SMikkTSpaceContext containing user data (MeshMikkTSpaceAdapter).
         * @param pos Output array to store the position (x, y, z) of the vertex.
         * @param face The index of the face in the mesh.
         * @param vert The index of the vertex within the face (0, 1, or 2).
         */
        static void GetPosition(const SMikkTSpaceContext* context, float pos[3], std::int32_t face, std::int32_t vert)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx = adapter->Indices[face * 3 + vert];
            const glm::vec3& p = adapter->Vertices[idx].Position;
            pos[0] = p.x; pos[1] = p.y; pos[2] = p.z;
        }

        /**
         * @brief Retrieves the normal vector for a specified vertex of a face in a mesh.
         *
         * This function extracts the normal vector from the mesh adapter for the given face and vertex indices,
         * and stores it in the provided array.
         *
         * @param context Pointer to the SMikkTSpaceContext containing user data for mesh access.
         * @param norm Output array to store the normal vector components (size 3).
         * @param face Index of the face in the mesh.
         * @param vert Index of the vertex within the face.
         */
        static void GetNormal(const SMikkTSpaceContext* context, float norm[3], std::int32_t face, std::int32_t vert)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx = adapter->Indices[face * 3 + vert];
            const glm::vec3& n = adapter->Vertices[idx].Normal;
            norm[0] = n.x; norm[1] = n.y; norm[2] = n.z;
        }

        /**
         * @brief Retrieves the texture coordinates (UV) for a specific vertex of a face in a mesh.
         *
         * This function extracts the UV coordinates from the mesh adapter using the provided face and vertex indices.
         * It is intended for use with the SMikkTSpace library for tangent space generation.
         *
         * @param context Pointer to the SMikkTSpaceContext containing user data (MeshMikkTSpaceAdapter).
         * @param uv Output array to store the retrieved UV coordinates (size 2).
         * @param face Index of the face in the mesh.
         * @param vert Index of the vertex within the face (0, 1, or 2).
         */
        static void GetTexCoord(const SMikkTSpaceContext* context, float uv[2], std::int32_t face, std::int32_t vert)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx = adapter->Indices[face * 3 + vert];
            const glm::vec2& t = adapter->Vertices[idx].TexCoord;
            uv[0] = t.x; uv[1] = t.y;
        }

        /**
         * @brief Sets the tangent and tangent sign for a specific vertex in a mesh using MikkTSpace.
         *
         * This function assigns the provided tangent vector and sign to the vertex specified by the face and vertex indices.
         * The tangent sign is used to reconstruct the bitangent in the shader as: cross(normal, tangent) * sign.
         *
         * @param context Pointer to the SMikkTSpaceContext containing user data for mesh adaptation.
         * @param tangent Array of 3 floats representing the tangent vector.
         * @param sign Float value representing the tangent sign (used for bitangent reconstruction).
         * @param face Index of the face in the mesh.
         * @param vert Index of the vertex within the face.
         */
        static void SetTSpaceBasic(const SMikkTSpaceContext* context, const float tangent[3], float sign, std::int32_t face, std::int32_t vert)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx = adapter->Indices[face * 3 + vert];
            adapter->Vertices[idx].Tangent = glm::vec3(tangent[0], tangent[1], tangent[2]);
            adapter->Vertices[idx].TangentSign = sign; // Bitangent can be reconstructed in shader: cross(normal, tangent) * sign
        }

        /**
         * @brief Generates tangent vectors for a mesh using the MikkTSpace algorithm.
         *
         * This function computes tangent vectors for each vertex in the provided mesh,
         * which are necessary for advanced shading techniques such as normal mapping.
         * It uses the MikkTSpace library to ensure consistent and high-quality tangent
         * space generation. The tangents are stored directly in the input vertex array.
         *
         * @param vertices Reference to a vector of Vertex objects representing the mesh.
         * @param indices Reference to a vector of indices defining the mesh's triangles.
         */
        inline void GenerateTangents(std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
        {
            MeshMikkTSpaceAdapter adapter{ vertices, indices };
            SMikkTSpaceInterface iface{};
            iface.m_getNumFaces = GetNumFaces;
            iface.m_getNumVerticesOfFace = GetNumVerticesOfFace;
            iface.m_getPosition = GetPosition;
            iface.m_getNormal = GetNormal;
            iface.m_getTexCoord = GetTexCoord;
            iface.m_setTSpaceBasic = SetTSpaceBasic;
            iface.m_setTSpace = nullptr; // Or &SetTSpace if you want full bitangent vectors

            SMikkTSpaceContext context{};
            context.m_pInterface = &iface;
            context.m_pUserData = &adapter;

            genTangSpaceDefault(&context);
        }
    }

    using MaterialAssetID = std::string;
    using TextureAssetID = std::string;
    using MeshAssetID = std::uint32_t;

    struct MaterialAsset
    {
        MaterialAssetID ID{};
        MaterialAttributes Attributes{};
        std::unordered_map<std::string, TextureAssetID> TextureRefs{};

        MaterialAsset() = default;
        ~MaterialAsset() = default;
    };

    struct TextureAsset
    {
        TextureType Type{ TextureType::UnknownTexture };
        std::uint8_t* Data{ nullptr };
        std::uint32_t Width{ 0 };
        std::uint32_t Height{ 0 };
        std::uint32_t Channels{ 0 };
        TextureAssetID ID{ "" };

        TextureAsset() = default;
        ~TextureAsset()
        {
            if (Data)
            {
                stbi_image_free(Data);
                Data = nullptr;
            }
        }
    };

    struct MeshAsset
    {
        MaterialAssetID MaterialID{ "" };
        std::vector<Vertex> Vertices{};
        std::vector<std::uint32_t> Indices{};

        MeshAsset() = default;
        ~MeshAsset() = default;
    };

    struct ImportedResults
    {
        uint32_t MeshCount = 0;
        glm::vec3 BoundsMin = {};
        glm::vec3 BoundsMax = {};

        std::unordered_map<MeshAssetID, MeshAsset> Meshes{};
        std::unordered_map<MaterialAssetID, MaterialAsset> Materials{};
        std::unordered_map<TextureAssetID, TextureAsset> Textures{};

        ImportedResults() = default;
        ~ImportedResults() = default;
    };


    /**
     * @brief Imports a 3D model from the specified input path, processes it, and exports relevant data to the output path.
     *
     * This function uses Assimp to read and process a 3D model file, extracting mesh, material, and texture information.
     * It computes global bounds, handles file copying if necessary, and populates the provided ImportedResults structure
     * with all relevant data. Meshes are processed for vertices, indices, normals, tangents, bitangents, and texture coordinates.
     * Materials and textures are loaded and associated with meshes. If certain attributes are missing, default values or
     * procedural generation (e.g., normals, UVs, tangents) are applied.
     *
     * @param input The path to the input model file to import.
     * @param output The path where the processed model data should be exported/copied.
     * @param outResults Reference to an ImportedResults structure to be populated with the imported data.
     * @return true if the import and export were successful; false otherwise.
     *
     * @note
     * - Uses Assimp for model importing and processing.
     * - Handles copying of input files to the output location if necessary.
     * - Generates missing mesh attributes (normals, UVs, tangents) when not present.
     * - Loads textures using stb_image and associates them with materials.
     * - Logs errors and info messages using MOTION_CORE_ERROR and MOTION_CORE_INFO macros.
     * - Throws no exceptions; all errors are handled internally and reported via return value and logging.
     */
    static bool Import(const std::filesystem::path& input, const std::filesystem::path& output, ImportedResults& outResults)
    {
        try
        {
            std::string modelFileName = input.filename().string();
            std::string modelFolderName = input.filename().stem().string();
            std::filesystem::path uniqueOutput{ GetAvailableCopyName(output / modelFolderName) / modelFileName };

            if (!std::filesystem::exists(uniqueOutput))
            {
                try
                {
                    std::filesystem::create_directories(uniqueOutput.parent_path());
                    std::filesystem::copy(input.parent_path(), uniqueOutput.parent_path(), std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                    MOTION_CORE_INFO("Copied all content from {} to {}", input.string(), uniqueOutput.string());
                }
                catch (const std::exception& e)
                {
                    MOTION_CORE_ERROR("Failed to copy directory: {}", e.what());
                    return false;
                }
            }

            Assimp::Importer importer;
            constexpr std::uint32_t importFlags =
                aiProcess_Triangulate |
                aiProcess_GenSmoothNormals |
                aiProcess_CalcTangentSpace |
                aiProcess_ImproveCacheLocality |
                aiProcess_RemoveRedundantMaterials |
                aiProcess_ValidateDataStructure |
                aiProcess_FlipUVs;

            const aiScene* scene = importer.ReadFile(uniqueOutput.string(), importFlags);
            if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            {
                MOTION_CORE_ERROR("Assimp Importer Error: {}", importer.GetErrorString());
                return false;
            }


            // Global bounds
            auto [minBounds, maxBounds] = ComputeGlobalBounds(scene);
            outResults.BoundsMin = minBounds;
            outResults.BoundsMax = maxBounds;
            outResults.MeshCount = scene->mNumMeshes;

            // Helper lambdas
            auto textureTypeToString =
                [](TextureType type)
                {
                    switch (type)
                    {
                    case TextureType::BaseColorTexture: return "BaseColor";
                    case TextureType::MetallicTexture: return "Metallic";
                    case TextureType::RoughnessTexture: return "Roughness";
                    case TextureType::AmbientOcclusionTexture: return "AmbientOcclusion";
                    case TextureType::NormalTexture: return "Normal";
                    default: return "Unknown";
                    }
                };

            // Load one mesh
            auto loadMesh = [&](std::uint32_t meshIndex, const aiMesh* mesh)
                {
                    if (!mesh) return;

                    auto& meshVertexData = outResults.Meshes[meshIndex];

                    bool hasUVs = mesh->HasTextureCoords(0);
                    bool hasNormals = mesh->HasNormals();
                    bool hasTangents = mesh->HasTangentsAndBitangents();

                    for (std::uint32_t i = 0; i < mesh->mNumVertices; ++i)
                    {
                        Vertex vertex
                        {
                            .Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },
                            .TexCoord = hasUVs ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2{ 0.f, 0.f },
                            .Normal = hasNormals ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3{ 0.f, 0.f, 1.f },
                            .Tangent = hasTangents ? glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : glm::vec3{ 1.f, 0.f, 0.f },
                            .Bitangent = hasTangents ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : glm::vec3{ 0.f, 1.f, 0.f },
                            .TangentSign = hasTangents ? ((glm::dot(glm::cross(hasNormals ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3{ 0.f, 0.f, 1.f }, glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z)), glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z)) < 0.f) ? -1.f : 1.f) : 1.f
                        };

                        meshVertexData.Vertices.push_back(std::move(vertex));
                    }

                    for (std::uint32_t i = 0; i < mesh->mNumFaces; ++i)
                        for (std::uint32_t idx = 0; idx < mesh->mFaces[i].mNumIndices; ++idx)
                            meshVertexData.Indices.push_back(mesh->mFaces[i].mIndices[idx]);

                    if (!hasNormals) GenerateNormals(meshVertexData.Vertices, meshVertexData.Indices);
                    if (!hasUVs) GenerateBoxProjectionUVs(meshVertexData.Vertices);
                    if (!hasTangents) MikkTSpace::GenerateTangents(meshVertexData.Vertices, meshVertexData.Indices);
                };

            // Load all textures for a material
            auto loadTexture =
                [&](aiMaterial* mat, const std::string& matID, aiTextureType aiType, TextureType type)
                {
                    aiString texPath;
                    if (mat->GetTexture(aiType, 0, &texPath) != aiReturn_SUCCESS)
                        return;

                    std::filesystem::path textureFile(texPath.C_Str());
                    if (!std::filesystem::exists(textureFile))
                        return;

                    TextureAssetID textureID = std::format("TEX_{}", HashString(textureFile.string()));
                    if (outResults.Textures.find(textureID) != outResults.Textures.end())
                    {
                        outResults.Materials[matID].TextureRefs[textureTypeToString(type)] = textureID;
                        return;
                    }

                    int w = 0, h = 0, ch = 0;
                    stbi_set_flip_vertically_on_load(1);
                    std::uint8_t* data = stbi_load(textureFile.string().c_str(), &w, &h, &ch, 4);
                    if (!data) return;

                    TextureAsset loaded{};
                    loaded.Type = type;
                    loaded.Data = data;
                    loaded.Width = static_cast<std::uint32_t>(w);
                    loaded.Height = static_cast<std::uint32_t>(h);
                    loaded.Channels = static_cast<std::uint32_t>(ch);
                    loaded.ID = textureID;

                    outResults.Textures[textureID] = std::move(loaded);
                    outResults.Materials[matID].TextureRefs[textureTypeToString(type)] = textureID;
                };

            // Load one material
            auto loadMaterial =
                [&](std::uint32_t meshIndex, aiMaterial* material)
                {
                    if (!material) return;

                    MaterialAssetID materialID = std::format("MAT_{}{}", meshIndex, HashString(material->GetName().C_Str()));
                    outResults.Meshes[meshIndex] = MeshAsset{};
                    outResults.Materials[materialID] = MaterialAsset{};

                    outResults.Meshes[meshIndex].MaterialID = materialID;
                    auto& materialAsset = outResults.Materials[materialID];
                    materialAsset.ID = materialID;

                    MaterialAttributes attr{};
                    aiColor3D baseColor = GetMaterialAttribute<aiColor3D>(material, AI_MATKEY_BASE_COLOR, aiColor3D(1.0f));
                    attr.BaseColor = { baseColor.r, baseColor.g, baseColor.b };
                    attr.Metallic = GetMaterialAttribute<float>(material, AI_MATKEY_METALLIC_FACTOR, 0.f);
                    attr.Roughness = GetMaterialAttribute<float>(material, AI_MATKEY_ROUGHNESS_FACTOR, 1.f);
                    attr.AmbientOcclusion = 1.0f; // Default
                    attr.Opacity = GetMaterialAttribute<float>(material, AI_MATKEY_OPACITY, 1.f);
                    attr.DisplacementScale = 0.05f; // Default
                    materialAsset.Attributes = attr;

                    loadTexture(material, materialID, aiTextureType_BASE_COLOR, TextureType::BaseColorTexture);
                    loadTexture(material, materialID, aiTextureType_METALNESS, TextureType::MetallicTexture);
                    loadTexture(material, materialID, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::RoughnessTexture);
                    loadTexture(material, materialID, aiTextureType_AMBIENT_OCCLUSION, TextureType::AmbientOcclusionTexture);
                    loadTexture(material, materialID, aiTextureType_NORMALS, TextureType::NormalTexture);
                };

            // Recursive node traversal (mesh & material import)
            std::function<void(const aiNode*)> traverse =
                [&](const aiNode* node)
                {
                    for (std::size_t i = 0; i < node->mNumMeshes; ++i)
                    {
                        std::uint32_t meshIdx = node->mMeshes[i];
                        aiMesh* mesh = scene->mMeshes[meshIdx];
                        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

                        if (mat) loadMaterial(meshIdx, mat);
                        if (mesh) loadMesh(meshIdx, mesh);
                    }

                    for (std::uint32_t i = 0; i < node->mNumChildren; ++i)
                        traverse(node->mChildren[i]);
                };


            traverse(scene->mRootNode);
            MOTION_CORE_INFO("Model exported successfully to: {}", output.string());
            return true;
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to export model: {}", e.what());
            return false;
        }
        return false;
    }


    /**
     * @brief Imports a 3D model from the specified file path and creates a StaticMesh asset.
     *
     * This function handles the process of importing a model file into the engine. It determines the output path,
     * checks for existing files, and manages file naming to avoid conflicts. The model is then imported into the engine,
     * and its meshes and materials are processed. Textures referenced by the materials are loaded and assigned accordingly.
     * If the import is successful and meshes are found, a StaticMesh asset is created and returned.
     *
     * @param path The filesystem path to the model file to import.
     * @param exportPath The export directory path for the imported model. If set to "default", uses the engine's default model directory.
     * @return std::shared_ptr<StaticMesh> A shared pointer to the imported StaticMesh asset, or nullptr if import fails.
     */
    std::shared_ptr<StaticMesh> Importer::ImportModel(const std::filesystem::path& path, const std::string& exportPath)
    {
        std::string modelName = path.filename().stem().string();

        std::filesystem::path applicationPath = std::filesystem::absolute(std::filesystem::path("."));
        std::string exportPathString = (exportPath == "default") ? (applicationPath / "Assets/Models").string() : exportPath;
        std::filesystem::path finalOutputPath{ exportPathString };

        try
        {
            ImportedResults importedModel{};
            if (Import(path, finalOutputPath, importedModel))
            {
                if (!importedModel.Meshes.empty())
                {
                    auto& assetManager = AssetManager::GetInstance();
                    auto staticMesh = assetManager.Create<StaticMesh>(modelName, finalOutputPath);

                    for (auto& [meshID, mesh] : importedModel.Meshes)
                    {
                        StaticMesh::MeshSegment segment;

                        const BufferLayout layout
                        {
                            {UniformCache::Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position)},
                            {UniformCache::TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord)},
                            {UniformCache::Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal)},
                            {UniformCache::Tangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent)},
                            {UniformCache::Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent)},
                            {UniformCache::TangentSign, BufferComponents::X, BufferStride::F1, false, offsetof(Vertex, TangentSign)}
                        };

                        segment.MeshSelf = Mesh::Create(mesh.Vertices.data(), static_cast<std::uint32_t>(mesh.Vertices.size()), mesh.Indices.data(), static_cast<std::uint32_t>(mesh.Indices.size()), layout, staticMesh);
                        if (importedModel.Materials.contains(mesh.MaterialID))
                        {
                            const auto& baseMaterial = assetManager.Get<Material>("BaseMaterial");
                            const auto& matData = importedModel.Materials.at(mesh.MaterialID);

                            segment.Materials = std::make_shared<MaterialInstance>(baseMaterial);
                            segment.Materials->Attributes.BaseColor = matData.Attributes.BaseColor;
                            segment.Materials->Attributes.Metallic = matData.Attributes.Metallic;
                            segment.Materials->Attributes.Roughness = matData.Attributes.Roughness;
                            segment.Materials->Attributes.AmbientOcclusion = matData.Attributes.AmbientOcclusion;
                            segment.Materials->Attributes.Opacity = matData.Attributes.Opacity;
                            segment.Materials->Attributes.DisplacementScale = matData.Attributes.DisplacementScale;

                            auto loadTextures =
                                [&](const std::string_view& uniformName, const std::string& slotName, TextureType type)
                                {
                                    if (!matData.TextureRefs.contains(slotName))
                                    {
                                        segment.Materials->Texture[uniformName] = nullptr;
                                    }
                                    else
                                    {
                                        std::string textureID = matData.TextureRefs.at(slotName);
                                        if (importedModel.Textures.contains(textureID))
                                        {
                                            auto& texData = importedModel.Textures.at(textureID);
                                            auto texture = ITexture::Create(texData.Data, type, texData.Width, texData.Height, texData.Channels);
                                            segment.Materials->Texture[uniformName] = texture;
                                        }
                                        else
                                        {
                                            segment.Materials->Texture[uniformName] = nullptr;
                                        }
                                    }
                                };

                            loadTextures(UniformCache::BaseColorTextures, "BaseColor", TextureType::BaseColorTexture);
                            loadTextures(UniformCache::MetallicTextures, "Metallic", TextureType::MetallicTexture);
                            loadTextures(UniformCache::RoughnessTextures, "Roughness", TextureType::RoughnessTexture);
                            loadTextures(UniformCache::AmbientOcclusionTextures, "AmbientOcclusion", TextureType::AmbientOcclusionTexture);
                            loadTextures(UniformCache::DisplacementTextures, "Displacement", TextureType::DisplacementTexture);
                            loadTextures(UniformCache::NormalTextures, "Normal", TextureType::NormalTexture);
                        }

                        staticMesh->m_Meshes.push_back(segment);
                    }

                    return staticMesh;
                }
                else
                {
                    MOTION_CORE_ERROR("No meshes found in the imported model: {}", finalOutputPath.string());
                    return nullptr;
                }
            }
        }
        catch (const std::exception& e)
        {
            MOTION_CORE_ERROR("Failed to import model from path: {}. Error: {}", path.string(), e.what());
            return nullptr;
        }

        return nullptr;
    }
}
