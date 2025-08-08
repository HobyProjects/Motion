#include "CorePCH.hpp"

namespace Motion
{
    static std::filesystem::path GetAvailableCopyName(const std::filesystem::path& originalPath) {
        if (!std::filesystem::exists(originalPath)) {
            return originalPath;
        }

        std::filesystem::path directory = originalPath.parent_path();
        std::string stem = originalPath.stem().string();
        std::string extension = originalPath.extension().string();

        int counter = 1;
        std::filesystem::path newPath;

        do {
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
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx = adapter->Indices[face * 3 + vert];
            const glm::vec3& p = adapter->Vertices[idx].Position;
            pos[0] = p.x; pos[1] = p.y; pos[2] = p.z;
        }

        static void GetNormal(const SMikkTSpaceContext* context, float norm[3], std::int32_t face, std::int32_t vert)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx = adapter->Indices[face * 3 + vert];
            const glm::vec3& n = adapter->Vertices[idx].Normal;
            norm[0] = n.x; norm[1] = n.y; norm[2] = n.z;
        }

        static void GetTexCoord(const SMikkTSpaceContext* context, float uv[2], std::int32_t face, std::int32_t vert)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx = adapter->Indices[face * 3 + vert];
            const glm::vec2& t = adapter->Vertices[idx].TexCoord;
            uv[0] = t.x; uv[1] = t.y;
        }

        static void SetTSpaceBasic(const SMikkTSpaceContext* context, const float tangent[3], float sign, std::int32_t face, std::int32_t vert)
        {
            auto* adapter = static_cast<MeshMikkTSpaceAdapter*>(context->m_pUserData);
            std::int32_t idx = adapter->Indices[face * 3 + vert];
            adapter->Vertices[idx].Tangent = glm::vec3(tangent[0], tangent[1], tangent[2]);
            adapter->Vertices[idx].TangentSign = sign;
        }

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
            iface.m_setTSpace = nullptr;

            SMikkTSpaceContext context{};
            context.m_pInterface = &iface;
            context.m_pUserData = &adapter;

            genTangSpaceDefault(&context);
        }
    }

    using MaterialAssetID = std::string;
    using TextureAssetID = std::string;
    using MeshAssetID = std::uint32_t;

    enum class MaterialType : std::int32_t
    {
        StandardMaterial,
        PhysicalBasedMaterial,
        UnknownMaterial
    };

    struct MaterialAsset
    {
        MaterialAssetID ID{};
        std::string Name{ "" };

        MaterialAsset() = default;
        virtual ~MaterialAsset() = default;
    };

    struct PhysicalBasedMaterialAsset : public MaterialAsset
    {
        PhysicalBasedMaterialAttribute Attributes{};
        std::unordered_set<TextureAssetID> TextureRefs{};

        PhysicalBasedMaterialAsset() = default;
        ~PhysicalBasedMaterialAsset() override = default;
    };

    struct MaterialRef
    {
        MaterialType Type{ MaterialType::UnknownMaterial };
        MaterialAssetID ID{ "" };
        std::string Name{ "" };
        std::shared_ptr<MaterialAsset> Asset{ nullptr };

        MaterialRef() = default;
        ~MaterialRef() = default;
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
        MeshAssetID ID{ 0 };
        std::string Name{ "" };
        std::vector<Vertex> Vertices{};
        std::vector<std::uint32_t> Indices{};
        std::vector<MaterialRef> MaterialRefs{};

        MeshAsset() = default;
        ~MeshAsset() = default;
    };

    struct ImportedResults
    {
        uint32_t MeshCount = 0;
        glm::vec3 BoundsMin = {};
        glm::vec3 BoundsMax = {};

        std::unordered_map<MeshAssetID, MeshAsset> Meshes{};
        std::unordered_map<TextureAssetID, TextureAsset> Textures{};

        ImportedResults() = default;
        ~ImportedResults() = default;
    };

    // -------------------------------------------------------------
    // Import (full) with robust textures, duplicate-mesh guard, etc.
    // -------------------------------------------------------------
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

            // ---- NEW: Directory for texture resolution + caches
            const std::filesystem::path modelDir = uniqueOutput.parent_path();

            auto resolveTexture = [&](const aiScene* scenePtr, const aiString& aiPath)
                -> std::variant<std::filesystem::path, const aiTexture*>
                {
                    if (!aiPath.C_Str() || !aiPath.C_Str()[0])
                        return std::filesystem::path{};

                    // Embedded (“*0”, “*1”, …)
                    if (aiPath.C_Str()[0] == '*') {
                        int idx = std::atoi(aiPath.C_Str() + 1);
                        if (idx >= 0 && idx < static_cast<int>(scenePtr->mNumTextures))
                            return scenePtr->mTextures[idx];
                        return std::filesystem::path{};
                    }

                    std::filesystem::path p(aiPath.C_Str());

                    if (p.is_absolute() && std::filesystem::exists(p))
                        return p;

                    std::filesystem::path rel = modelDir / p;
                    if (std::filesystem::exists(rel))
                        return rel;

                    std::filesystem::path fallback = input.parent_path() / p;
                    if (std::filesystem::exists(fallback))
                        return fallback;

                    return std::filesystem::path{};
                };

            std::unordered_map<std::string, TextureAssetID> texturePathToID;

            auto loadTextureAsset =
                [&](const std::variant<std::filesystem::path, const aiTexture*>& source,
                    TextureType type) -> std::optional<TextureAssetID>
                {
                    stbi_set_flip_vertically_on_load(1);

                    TextureAsset tex{};
                    tex.Type = type;

                    if (std::holds_alternative<const aiTexture*>(source))
                    {
                        const aiTexture* emb = std::get<const aiTexture*>(source);
                        if (!emb) return std::nullopt;

                        int w = 0, h = 0, ch = 0;
                        unsigned char* data = nullptr;

                        if (emb->mHeight == 0)
                        {
                            data = stbi_load_from_memory(
                                reinterpret_cast<const stbi_uc*>(emb->pcData),
                                emb->mWidth, &w, &h, &ch, 4);
                        }
                        else
                        {
                            const int bytes = emb->mWidth * emb->mHeight * 4;
                            data = stbi_load_from_memory(
                                reinterpret_cast<const stbi_uc*>(emb->pcData),
                                bytes, &w, &h, &ch, 4);
                        }

                        if (!data) return std::nullopt;

                        tex.Data = data;
                        tex.Width = static_cast<std::uint32_t>(w);
                        tex.Height = static_cast<std::uint32_t>(h);
                        tex.Channels = 4; // requested RGBA
                        std::string key = std::format("EMBED_{:016X}_{}x{}", reinterpret_cast<uintptr_t>(emb), w, h);
                        tex.ID = std::format("TEX_PBR_{}", HashString(key));

                        outResults.Textures.emplace(tex.ID, std::move(tex));
                        return tex.ID;
                    }
                    else
                    {
                        const auto& path = std::get<std::filesystem::path>(source);
                        if (path.empty() || !std::filesystem::exists(path)) return std::nullopt;

                        const std::string norm = std::filesystem::weakly_canonical(path).string();
                        if (auto it = texturePathToID.find(norm); it != texturePathToID.end())
                            return it->second;

                        int w = 0, h = 0, ch = 0;
                        unsigned char* data = stbi_load(norm.c_str(), &w, &h, &ch, 4);
                        if (!data) return std::nullopt;

                        tex.Data = data;
                        tex.Width = static_cast<std::uint32_t>(w);
                        tex.Height = static_cast<std::uint32_t>(h);
                        tex.Channels = 4; // requested RGBA
                        tex.ID = std::format("TEX_PBR_{}", HashString(norm));

                        outResults.Textures.emplace(tex.ID, std::move(tex));
                        texturePathToID[norm] = tex.ID;
                        return tex.ID;
                    }
                };

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
                        Vertex vertex
                        {
                            .Position = { mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z },
                            .TexCoord = hasUVs ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2{ 0.f, 0.f },
                            .Normal = hasNormals ? glm::vec3(mesh->mNormals[i].x,  mesh->mNormals[i].y,  mesh->mNormals[i].z) : glm::vec3{ 0.f, 0.f, 1.f },
                            .Tangent = hasTangents ? glm::vec3(mesh->mTangents[i].x,  mesh->mTangents[i].y,  mesh->mTangents[i].z) : glm::vec3{ 1.f, 0.f, 0.f },
                            .Bitangent = hasTangents ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : glm::vec3{ 0.f, 1.f, 0.f },
                            .TangentSign = hasTangents ? ((glm::dot(glm::cross(hasNormals ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3{ 0.f, 0.f, 1.f }, glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z)), glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z)) < 0.f) ? -1.f : 1.f) : 1.f
                        };

                        verts.push_back(std::move(vertex));
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

            // ---- Load all PBR textures for a material (supports multi-slot + embedded)
            auto load_PBR_Textures =
                [&](aiMaterial* mat, std::unordered_set<TextureAssetID>& pbrTextureRefs, aiTextureType aiType, TextureType type)
                {
                    if (!mat) return;

                    const unsigned int texCount = mat->GetTextureCount(aiType);
                    for (unsigned int i = 0; i < texCount; ++i)
                    {
                        aiString texPath;
                        if (mat->GetTexture(aiType, i, &texPath) != aiReturn_SUCCESS)
                            continue;

                        auto resolved = resolveTexture(scene, texPath);
                        if (auto id = loadTextureAsset(resolved, type))
                        {
                            pbrTextureRefs.insert(*id);
                        }
                    }
                };

            // ---- Build one material (PBR)
            auto loadMaterial =
                [&](std::uint32_t meshIndex, aiMaterial* material)
                {
                    if (!material) return;

                    MaterialRef pbrMaterialRef{};
                    pbrMaterialRef.ID = std::format("MAT_PBR_{}", HashString(material->GetName().C_Str()));
                    pbrMaterialRef.Type = MaterialType::PhysicalBasedMaterial;
                    pbrMaterialRef.Name = material->GetName().C_Str();
                    pbrMaterialRef.Asset = std::make_shared<PhysicalBasedMaterialAsset>();
                    auto pbrMaterialAsset = std::dynamic_pointer_cast<PhysicalBasedMaterialAsset>(pbrMaterialRef.Asset);

                    aiColor3D baseColor = GetMaterialAttribute<aiColor3D>(material, AI_MATKEY_BASE_COLOR, aiColor3D(1.0f));
                    pbrMaterialAsset->Attributes.BaseColor = { baseColor.r, baseColor.g, baseColor.b };
                    pbrMaterialAsset->Attributes.Metallic = GetMaterialAttribute<float>(material, AI_MATKEY_METALLIC_FACTOR, 1.0f);
                    pbrMaterialAsset->Attributes.Roughness = GetMaterialAttribute<float>(material, AI_MATKEY_ROUGHNESS_FACTOR, 1.0f);
                    pbrMaterialAsset->Attributes.Opacity = GetMaterialAttribute<float>(material, AI_MATKEY_OPACITY, 1.0f);

                    load_PBR_Textures(material, pbrMaterialAsset->TextureRefs, aiTextureType_BASE_COLOR, TextureType::BaseColorTexture);
                    load_PBR_Textures(material, pbrMaterialAsset->TextureRefs, aiTextureType_METALNESS, TextureType::MetallicTexture);
                    load_PBR_Textures(material, pbrMaterialAsset->TextureRefs, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::RoughnessTexture);
                    load_PBR_Textures(material, pbrMaterialAsset->TextureRefs, aiTextureType_AMBIENT_OCCLUSION, TextureType::AmbientOcclusionTexture);
                    load_PBR_Textures(material, pbrMaterialAsset->TextureRefs, aiTextureType_NORMAL_CAMERA, TextureType::NormalTexture);
                    // Optional: emissive, opacity, AO/rough/metal packed maps, etc.

                    outResults.Meshes[meshIndex].MaterialRefs.push_back(pbrMaterialRef);
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
                        if (mat)  loadMaterial(meshIdx, mat);
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
                    staticMesh->m_MaxBounds = importedModel.BoundsMax;
                    staticMesh->m_MinBounds = importedModel.BoundsMin;

                    auto load_PBR_Textures =
                        [&](std::shared_ptr<PhysicalBasedMaterialInstance> pbrMaterialInstance, std::unordered_set<TextureAssetID>& textureRefs)
                        {
                            for (const auto& [textureID, textureAsset] : importedModel.Textures)
                            {
                                if (textureRefs.contains(textureID))
                                {
                                    pbrMaterialInstance->Texture[textureAsset.Type] =
                                        std::move(ITexture::Create(textureAsset.Data, textureAsset.Type,
                                            textureAsset.Width, textureAsset.Height, textureAsset.Channels));
                                }
                            }
                        };

                    for (auto& [meshID, mesh] : importedModel.Meshes)
                    {
                        const BufferLayout layout
                        {
                            {UniformCache::Position,    BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position)},
                            {UniformCache::TexCoords,   BufferComponents::UV,  BufferStride::F2, false, offsetof(Vertex, TexCoord)},
                            {UniformCache::Normals,     BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal)},
                            {UniformCache::Tangents,    BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent)},
                            {UniformCache::Bitangents,  BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent)},
                            {UniformCache::TangentSign, BufferComponents::X,   BufferStride::F1, false, offsetof(Vertex, TangentSign)}
                        };

                        auto meshPtr = Mesh::Create(mesh.Vertices.data(),
                            static_cast<std::uint32_t>(mesh.Vertices.size()),
                            mesh.Indices.data(),
                            static_cast<std::uint32_t>(mesh.Indices.size()),
                            layout, staticMesh);
                        meshPtr->Index = meshID;
                        meshPtr->Name = mesh.Name;

                        if (!mesh.MaterialRefs.empty())
                        {
                            const auto& pbrBaseMaterial = assetManager.Get<PhysicalBasedMaterial>("PBR_BaseMaterial");

                            for (const auto& matRef : mesh.MaterialRefs)
                            {
                                if (matRef.Type == MaterialType::PhysicalBasedMaterial)
                                {
                                    auto pbrMaterialInstance = std::make_shared<PhysicalBasedMaterialInstance>(matRef.ID, pbrBaseMaterial);
                                    const auto materialAsset = std::dynamic_pointer_cast<PhysicalBasedMaterialAsset>(matRef.Asset);

                                    pbrMaterialInstance->Attributes.BaseColor = materialAsset->Attributes.BaseColor;
                                    pbrMaterialInstance->Attributes.Metallic = materialAsset->Attributes.Metallic;
                                    pbrMaterialInstance->Attributes.Roughness = materialAsset->Attributes.Roughness;
                                    pbrMaterialInstance->Attributes.Opacity = materialAsset->Attributes.Opacity;

                                    load_PBR_Textures(pbrMaterialInstance, materialAsset->TextureRefs);
                                    meshPtr->SetMaterial(pbrMaterialInstance);
                                }
                                else
                                {
                                    MOTION_CORE_ERROR("Unknown material type for mesh: {}", mesh.Name);
                                    continue;
                                }
                            }
                        }

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
