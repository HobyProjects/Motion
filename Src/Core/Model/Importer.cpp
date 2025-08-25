#include "CorePCH.hpp"

namespace Motion
{
    static std::filesystem::path GetAvailableCopyName(const std::filesystem::path& originalPath)
    {
        if (!std::filesystem::exists(originalPath))
        {
            return originalPath;
        }

        std::filesystem::path directory = originalPath.parent_path();
        std::string stem = originalPath.stem().string();
        std::string extension = originalPath.extension().string();

        int counter = 1;
        std::filesystem::path newPath;

        do
        {
            newPath = directory / (stem + "-copy" + (counter > 1 ? std::to_string(counter) : "") + extension);
            ++counter;

        } while (std::filesystem::exists(newPath));

        return newPath;
    }

    void GenerateBoxProjectionUVs(std::vector<Vertex>& vertices)
    {
        if (vertices.empty()) return;

        glm::vec2 minUV(FLT_MAX), maxUV(-FLT_MAX);
        std::vector<glm::vec2> projected(vertices.size());

        for (size_t i = 0; i < vertices.size(); ++i) {
            const glm::vec3& pos = vertices[i].Position;
            glm::vec3 n = glm::abs(glm::normalize(vertices[i].Normal));
            glm::vec2 uv;
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

    static void GenerateNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    {
        for (auto& v : vertices)
            v.Normal = glm::vec3(0.0f);

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

        for (auto& v : vertices)
            v.Normal = glm::normalize(v.Normal);
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

        static std::int32_t GetNumFaces(const SMikkTSpaceContext* context)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            return static_cast<std::int32_t>(adapter->Indices.size() / 3);
        }

        static std::int32_t GetNumVerticesOfFace(const SMikkTSpaceContext*, std::int32_t)
        {
            return 3;
        }

        static void GetPosition(const SMikkTSpaceContext* context, float pos[3], std::int32_t face, std::int32_t vert)
        {
            auto*               adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t        idx     = adapter->Indices[face * 3 + vert];
            const glm::vec3&    p       = adapter->Vertices[idx].Position;

            pos[0] = p.x; 
            pos[1] = p.y; 
            pos[2] = p.z;
        }

        static void GetNormal(const SMikkTSpaceContext* context, float norm[3], std::int32_t face, std::int32_t vert)
        {
            auto* adapter           = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx        = adapter->Indices[face * 3 + vert];
            const glm::vec3& n      = adapter->Vertices[idx].Normal;
            norm[0] = n.x; norm[1]  = n.y; norm[2] = n.z;
        }

        static void GetTexCoord(const SMikkTSpaceContext* context, float uv[2], std::int32_t face, std::int32_t vert)
        {
            auto* adapter       = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx    = adapter->Indices[face * 3 + vert];
            const glm::vec2& t  = adapter->Vertices[idx].TexCoord;
            uv[0] = t.x; uv[1]  = t.y;
        }

        static void SetTSpaceBasic(const SMikkTSpaceContext* context, const float tangent[4], float sign, std::int32_t face, std::int32_t vert)
        {
            auto* adapter       = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx    = adapter->Indices[face * 3 + vert];
            
            adapter->Vertices[idx].Tangent      = glm::vec4(tangent[0], tangent[1], tangent[2], tangent[3]);
            adapter->Vertices[idx].Tangent.w    = sign;
        }

        inline void GenerateTangents(std::vector<Vertex>& vertices, const std::vector<std::uint32_t>& indices)
        {
            MeshMikkTSpaceAdapter   adapter{ vertices, indices };
            SMikkTSpaceInterface    iface{};
            iface.m_getNumFaces             = GetNumFaces;
            iface.m_getNumVerticesOfFace    = GetNumVerticesOfFace;
            iface.m_getPosition             = GetPosition;
            iface.m_getNormal               = GetNormal;
            iface.m_getTexCoord             = GetTexCoord;
            iface.m_setTSpaceBasic          = SetTSpaceBasic;
            iface.m_setTSpace               = nullptr;

            SMikkTSpaceContext context{};
            context.m_pInterface    = &iface;
            context.m_pUserData     = &adapter;

            genTangSpaceDefault(&context);
        }
    }

    using MeshAssetID = std::uint32_t;

    struct MeshAsset
    {
        MeshAssetID ID{ 0 };
        std::string Name{ "" };
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

        ImportedResults() = default;
        ~ImportedResults() = default;
    };

    static bool Import(const std::filesystem::path& input, const std::filesystem::path& output, ImportedResults& outResults)
    {
        try
        {
            std::filesystem::path uniqueOutput = input;

            if (!output.empty())
            {
                std::string modelFileName = input.filename().string();
                std::string modelFolderName = input.filename().stem().string();
                uniqueOutput = GetAvailableCopyName(output / modelFolderName) / modelFileName;

                if (!std::filesystem::exists(uniqueOutput))
                {
                    try
                    {
                        std::filesystem::create_directories(uniqueOutput.parent_path());
                        std::filesystem::copy(input.parent_path(), uniqueOutput.parent_path(),
                            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing);
                        MOTION_CORE_INFO("Copied all content from {} to {}", input.string(), uniqueOutput.string());
                    }
                    catch (const std::exception& e)
                    {
                        MOTION_CORE_ERROR("Failed to copy directory: {}", e.what());
                        return false;
                    }
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

            // ---- Prevent duplicate mesh loads (meshes can appear under multiple nodes)
            std::vector<bool> meshLoaded(scene->mNumMeshes, false);

            auto loadMesh = [&](std::uint32_t meshIndex, const aiMesh* mesh)
                {
                    if (!mesh || meshLoaded[meshIndex]) return;
                    meshLoaded[meshIndex] = true;

                    outResults.Meshes[meshIndex] = MeshAsset{};
                    outResults.Meshes[meshIndex].ID = meshIndex;
                    outResults.Meshes[meshIndex].Name = mesh->mName.C_Str();

                    bool hasUVs = mesh->HasTextureCoords(0);
                    bool hasNormals = mesh->HasNormals();
                    bool hasTangents = mesh->HasTangentsAndBitangents();

                    auto& verts = outResults.Meshes[meshIndex].Vertices;
                    verts.reserve(mesh->mNumVertices);

                    for (std::uint32_t i = 0; i < mesh->mNumVertices; ++i)
                    {
                        Vertex vtx;
                        vtx.Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
                        vtx.TexCoord = hasUVs ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2{ 0.f, 0.f };
                        vtx.Normal = hasNormals ? glm::vec3(mesh->mNormals[i].x,  mesh->mNormals[i].y,  mesh->mNormals[i].z) : glm::vec3{ 0.f, 0.f, 1.f };
                        vtx.Tangent = hasTangents ? glm::vec4(mesh->mTangents[i].x,  mesh->mTangents[i].y,  mesh->mTangents[i].z, 0.0f) : glm::vec4{ 1.f, 0.f, 0.f, 0.0f };
                        vtx.Bitangent = hasTangents ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : glm::vec3{ 0.f, 1.f, 0.f };
                        vtx.Tangent.w = hasTangents ? ((glm::dot(glm::cross(hasNormals ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z)  : glm::vec3{ 0.f, 0.f, 1.f }, glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z)), glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z)) < 0.f)  ? -1.f : 1.f) : 1.f;
                        verts.push_back(std::move(vtx));
                    }

                    auto& idxs = outResults.Meshes[meshIndex].Indices;
                    idxs.reserve(mesh->mNumFaces * 3);
                    for (std::uint32_t i = 0; i < mesh->mNumFaces; ++i)
                        for (std::uint32_t idx = 0; idx < mesh->mFaces[i].mNumIndices; ++idx)
                            idxs.push_back(mesh->mFaces[i].mIndices[idx]);

                    if (!hasNormals)  GenerateNormals(outResults.Meshes[meshIndex].Vertices, outResults.Meshes[meshIndex].Indices);
                    if (!hasUVs)      GenerateBoxProjectionUVs(outResults.Meshes[meshIndex].Vertices);
                    if (!hasTangents) MikkTSpace::GenerateTangents(outResults.Meshes[meshIndex].Vertices, outResults.Meshes[meshIndex].Indices);
                };

            // ---- Traverse nodes
            std::function<void(const aiNode*)> traverse =
                [&](const aiNode* node)
                {
                    for (std::size_t i = 0; i < node->mNumMeshes; ++i)
                    {
                        std::uint32_t meshIdx = node->mMeshes[i];
                        aiMesh* mesh = scene->mMeshes[meshIdx];
                        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];

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
    }

    std::shared_ptr<StaticMesh> Importer::ImportModel(const std::filesystem::path& path, bool shouldExport, const std::string& exportPath)
    {
        std::string modelName = path.filename().stem().string();

        std::filesystem::path applicationPath = std::filesystem::absolute(std::filesystem::path("."));
        std::string exportPathString = (exportPath == "default") ? (applicationPath / "Assets/Models").string() : exportPath;
        std::filesystem::path finalOutputPath{ exportPathString };

        try
        {
            ImportedResults importedModel{};
            if (Import(path, (shouldExport ? finalOutputPath : std::filesystem::path{}), importedModel))
            {
                if (!importedModel.Meshes.empty())
                {
                    auto& AM = AssetManager::GetInstance();
                    auto& MB = MaterialBuilder::GetInstance();

                    std::shared_ptr<StaticMesh> staticMesh = std::make_shared<StaticMesh>(UniqueIdentity::GetUniqueID(), modelName, finalOutputPath);
                    staticMesh->m_MaxBounds = importedModel.BoundsMax;
                    staticMesh->m_MinBounds = importedModel.BoundsMin;

                    std::shared_ptr<BaseMaterial> baseMaterial = AM.Get<BaseMaterial>("MetalBaseMaterial");

                    for (auto& [meshID, mesh] : importedModel.Meshes)
                    {
                        const BufferLayout layout
                        {
                            {"a_Position",    BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position)},
                            {"a_TexCoords",   BufferComponents::UV,  BufferStride::F2, false, offsetof(Vertex, TexCoord)},
                            {"a_Normals",     BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal)},
                            {"a_Tangents",    BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)},
                            {"a_Bitangents",  BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent)}
                        };

                        auto meshPtr = Mesh::Create(mesh.Vertices.data(), static_cast<std::uint32_t>(mesh.Vertices.size()), mesh.Indices.data(), static_cast<std::uint32_t>(mesh.Indices.size()), layout, staticMesh);
                        meshPtr->Index = meshID;
                        meshPtr->Name = mesh.Name;
                        meshPtr->Materials = MB.Create(baseMaterial);
                        staticMesh->m_Meshes.emplace_back(std::move(meshPtr));
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
