#include "CorePCH.hpp"

namespace Motion
{
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
        else
        {
            std::string modelName = path.filename().stem().string();
            MOTION_CORE_INFO("Assimp Importer: StaticMesh {0} file successfully loaded to memory from {1} > Converting...", modelName, path.string());
            return Exporter::ExportModel(scene, exportPath);
        }

        return false;
    }

    struct LoadedMaterial
    {
        std::string ID{};
        MaterialAttributes Attributes{};
        std::unordered_map<std::string, std::string> TextureRefs{};
    };

    struct LoadedTexture
    {
        std::string ID{};
        TextureType Type{};
        std::vector<std::uint8_t> Data{};
        std::uint32_t Width{};
        std::uint32_t Height{};
        std::uint32_t Channels{};
    };

    struct LoadedMesh
    {
        std::vector<Vertex> Vertices{};
        std::vector<std::uint32_t> Indices{};
        std::string MaterialID{};
    };

    struct ImportedModel
    {
        uint32_t MeshCount = 0;
        glm::vec3 BoundsMin = {};
        glm::vec3 BoundsMax = {};

        std::vector<LoadedMesh> Meshes;
        std::unordered_map<std::string, LoadedMaterial> Materials;
        std::unordered_map<std::string, LoadedTexture> Textures;
    };

    static bool Import(const std::filesystem::path& path, ImportedModel& result)
    {
        using namespace std::string_view_literals;

        // Read entire file into buffer
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        // --- META ---
        if (auto meta_match = std::smatch{}; std::regex_search(buffer, meta_match, std::regex(R"(\[META_BEGIN\]([\s\S]*?)\[META_END\])")))
        {
            std::string meta = meta_match[1].str();
            // [COUNT:x]
            auto m = std::smatch{};
            if (std::regex_search(meta, m, std::regex(R"(\[COUNT:(\d+)\])")))
                result.MeshCount = static_cast<uint32_t>(std::stoul(m[1].str()));

            // [BOUNDS:x,y,z|x,y,z]
            if (std::regex_search(meta, m, std::regex(R"(\[BOUNDS:([-\d.,]+)\|([-\d.,]+)\])")))
            {
                auto parseVec3 =
                    [](const std::string& s) -> glm::vec3
                    {
                        float x, y, z;
                        std::sscanf(s.c_str(), "%f,%f,%f", &x, &y, &z);
                        return { x, y, z };
                    };

                result.BoundsMin = parseVec3(m[1].str());
                result.BoundsMax = parseVec3(m[2].str());
            }
        }

        // --- MATERIALS ---
        if (auto mat_block = std::smatch{}; std::regex_search(buffer, mat_block, std::regex(R"(\[MATERIALS_BEGIN\]([\s\S]*?)\[MATERIALS_END\])")))
        {
            std::string mats = mat_block[1].str();
            std::regex mat_regex(R"(\[MAT:([^\]]+)\]([\s\S]*?)\[MAT_END\])");
            auto mat_begin = mats.cbegin();
            auto mat_end = mats.cend();
            std::smatch m;

            while (std::regex_search(mat_begin, mat_end, m, mat_regex))
            {
                LoadedMaterial mat{};
                mat.ID = m[1].str();
                std::string matbody = m[2].str();

                // Attributes
                if (auto attr = std::smatch{}; std::regex_search(matbody, attr, std::regex(R"(\[ATTR:BaseColor:([-\d.,]+)\])")))
                {
                    float r, g, b;
                    std::sscanf(attr[1].str().c_str(), "%f,%f,%f", &r, &g, &b);
                    mat.Attributes.BaseColor = { r, g, b };
                }
                auto attrf =
                    [&](const char* name, float& out)
                    {
                        std::regex re(std::string(R"(\[ATTR:)") + name + R"(:([-\d.eE]+)\])");
                        if (auto attr = std::smatch{}; std::regex_search(matbody, attr, re))
                            out = std::stof(attr[1].str());
                    };

                attrf("Metallic", mat.Attributes.Metallic);
                attrf("Roughness", mat.Attributes.Roughness);
                attrf("AmbientOcclusion", mat.Attributes.AmbientOcclusion);
                attrf("Opacity", mat.Attributes.Opacity);
                attrf("DisplacementScale", mat.Attributes.DisplacementScale);

                // Texture references
                std::regex tref(R"(\[TEX_REF:([A-Za-z0-9_]+)=([A-Za-z0-9_]+)\])");
                auto tb = matbody.cbegin(), te = matbody.cend();
                std::smatch tr;
                while (std::regex_search(tb, te, tr, tref))
                {
                    mat.TextureRefs[tr[1].str()] = tr[2].str();
                    tb += tr.position() + tr.length();
                }

                result.Materials[mat.ID] = std::move(mat);
                mat_begin += m.position() + m.length();
            }
        }

        // --- TEXTURES ---
        std::regex tex_block(R"(\[TEX:([^\]]+)\][\s\S]*?\[TEX_META:([^\]]+)\][\s\S]*?\[TEX_DATA_BEGIN\]([\s\S]*?)\[TEX_DATA_END\][\s\S]*?\[TEX_END\])");
        auto tb = buffer.cbegin(), te = buffer.cend();
        std::smatch tmatch;

        while (std::regex_search(tb, te, tmatch, tex_block))
        {
            std::string texID = tmatch[1].str();
            std::string meta = tmatch[2].str();
            size_t w = 0, h = 0, ch = 0, size = 0;
            std::sscanf(meta.c_str(), "%*[^,],%zu,%zu,%zu,%zu", &w, &h, &ch, &size);
            std::ptrdiff_t binBeginPos = tmatch.position(3) + std::distance(buffer.cbegin(), tb);
            LoadedTexture tex;
            tex.ID = texID;
            tex.Width = static_cast<uint32_t>(w);
            tex.Height = static_cast<uint32_t>(h);
            tex.Channels = static_cast<uint32_t>(ch);
            tex.Data.resize(size);
            std::memcpy(tex.Data.data(), buffer.data() + binBeginPos, size);
            result.Textures[texID] = std::move(tex);
            tb += tmatch.position() + tmatch.length();
        }

        // --- MESHES ---
        std::regex mesh_block(R"(\[M(\d+)_BEGIN\]([\s\S]*?)\[M\1_END\])");
        auto mb = buffer.cbegin(), me = buffer.cend();
        std::smatch mmatch;

        while (std::regex_search(mb, me, mmatch, mesh_block))
        {
            LoadedMesh mesh{};
            std::string meshbody = mmatch[2].str();
            // Vertices
            if (auto vx = std::smatch{}; std::regex_search(meshbody, vx, std::regex(R"(\[VTX:(\d+)\])")))
            {
                size_t count = std::stoul(vx[1].str());
                // Find binary offset for vertices: locate position after [VTX:count]\n
                std::ptrdiff_t vtx_pos = mmatch.position(2) + meshbody.find(vx[0].str()) + vx[0].length() + 1;
                mesh.Vertices.resize(count);
                std::memcpy(mesh.Vertices.data(), buffer.data() + std::distance(buffer.cbegin(), mb) + vtx_pos, count * sizeof(Vertex));
            }
            // Indices
            if (auto ix = std::smatch{}; std::regex_search(meshbody, ix, std::regex(R"(\[IDX:(\d+)\])")))
            {
                size_t count = std::stoul(ix[1].str());
                std::ptrdiff_t idx_pos = mmatch.position(2) + meshbody.find(ix[0].str()) + ix[0].length() + 1;
                mesh.Indices.resize(count);
                std::memcpy(mesh.Indices.data(), buffer.data() + std::distance(buffer.cbegin(), mb) + idx_pos, count * sizeof(uint32_t));
            }
            // Material ref
            if (auto mr = std::smatch{}; std::regex_search(meshbody, mr, std::regex(R"(\[MAT_REF:([^\]]+)\])")))
            {
                mesh.MaterialID = mr[1].str();
            }

            result.Meshes.push_back(std::move(mesh));
            mb += mmatch.position() + mmatch.length();
        }

        return true;
    }


    std::shared_ptr<StaticMesh> Importer::ImportModel(const std::filesystem::path& path, const std::string& exportPath)
    {
        std::string modelName = path.filename().stem().string();

        std::filesystem::path appRoot = std::filesystem::absolute(std::filesystem::path("."));
        std::string exportPathString = (exportPath == "default") ? (appRoot / "Assets/Models").string() : exportPath;

        std::filesystem::path finalOutputPath{};
        std::filesystem::path file = std::filesystem::path(std::format("{}/{}/{}.mmesh", exportPathString, modelName, modelName));
        if (!std::filesystem::exists(file))
        {
            std::filesystem::path outputFilePath = GetAvailableCopyName(file);
            finalOutputPath = std::filesystem::absolute(outputFilePath);
        }
        else
        {
            finalOutputPath = std::filesystem::absolute(file);
        }

        if (ImportToEngine(path, finalOutputPath))
        {
            try
            {
                ImportedModel importedModel{};
                if (Import(finalOutputPath, importedModel))
                {
                    if (!importedModel.Meshes.empty())
                    {
                        auto& assetManager = AssetManager::GetInstance();
                        auto staticMesh = assetManager.Create<StaticMesh>(modelName, finalOutputPath);
                        staticMesh->m_Meshes.reserve(importedModel.Meshes.size());

                        for (auto& mesh : importedModel.Meshes)
                        {
                            StaticMesh::MeshSegment segment;

                            const BufferLayout layout
                            {
                                {UniformCache::Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position)},
                                {UniformCache::TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord)},
                                {UniformCache::Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal)},
                                {UniformCache::Tangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent)},
                                {UniformCache::Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent)}
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
                                        std::string textureID = matData.TextureRefs.at(slotName);
                                        if (importedModel.Textures.contains(textureID))
                                        {
                                            auto& texData = importedModel.Textures.at(textureID);
                                            auto texture = ITexture::Create(texData.Data.data(), type, texData.Width, texData.Height, texData.Channels);
                                            segment.Materials->Texture[uniformName] = texture;
                                        }
                                        else
                                        {
                                            MOTION_CORE_WARN("Texture ID '{}' not found in imported model data.", textureID);
                                            segment.Materials->Texture[uniformName] = nullptr;
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
        }
        else
        {
            MOTION_CORE_ERROR("Failed to import model from path: {}", path.string());
            return nullptr;
        }

        return nullptr;
    }
}
