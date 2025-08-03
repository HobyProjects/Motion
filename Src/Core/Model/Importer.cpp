#include "CorePCH.hpp"

namespace Motion
{
    class BinaryReader
    {
    public:
        explicit BinaryReader(std::uint8_t* data, std::size_t size)
            : m_Data{ data }, m_Size{ size }, m_CurrentPosition{ 0 } {
        }
        ~BinaryReader() = default;

        template<typename T>
        T Read()
        {
            static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
            if (m_CurrentPosition + sizeof(T) > m_Size) {
                MOTION_CORE_ERROR("Attempt to read beyond buffer size");
                return T{};
            }

            T value;
            std::memcpy(&value, m_Data + m_CurrentPosition, sizeof(T));
            m_CurrentPosition += sizeof(T);
            return value;
        }

        std::vector<std::uint8_t> ReadBytes(std::size_t size) {
            if (m_CurrentPosition + size > m_Size) {
                MOTION_CORE_ERROR("Attempt to read beyond buffer");
                return {};
            }

            std::vector<std::uint8_t> out(size);
            std::memcpy(out.data(), m_Data + m_CurrentPosition, size);
            m_CurrentPosition += size;
            return out;
        }

        bool HasMore() const {
            return m_CurrentPosition < m_Size;
        }

    private:
        std::uint8_t* m_Data{ nullptr };
        std::size_t m_Size{ 0 };
        std::size_t m_CurrentPosition{ 0 };
    };

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
            return Exporter::ExportModel(scene, exportPath);
        }

        return false;
    }

    static std::uint32_t GetMeshCount(const std::string& dataStringVersion)
    {
        std::regex meshCountRegex(R"(\[M_MSH_COUNT:(\d+)\])");
        std::smatch match;
        if (std::regex_search(dataStringVersion, match, meshCountRegex) && match.size() > 1) {
            return std::stoul(match[1].str());
        }
        return 0; // Default value if not found
    }

    template<typename CallbackFunc>
    static void ReadSection(const std::string& dataStringVersion, const std::string& sectionBeginRegex, const std::string& sectionEndRegex, const std::vector<std::uint8_t>& rawBytes, CallbackFunc&& callback)
    {
        std::regex sectionBeginPattern(sectionBeginRegex);
        std::regex sectionEndPattern(sectionEndRegex);

        auto sectionBeginIterator = std::sregex_iterator(dataStringVersion.begin(), dataStringVersion.end(), sectionBeginPattern);
        auto sectionEndIterator = std::sregex_iterator(dataStringVersion.begin(), dataStringVersion.end(), sectionEndPattern);

        if (sectionBeginIterator == std::sregex_iterator() || sectionEndIterator == std::sregex_iterator()) {
            MOTION_CORE_ERROR("No matching sections found for regex: {} and {}", sectionBeginRegex, sectionEndRegex);
            return;
        }

        while (sectionBeginIterator != std::sregex_iterator() && sectionEndIterator != std::sregex_iterator()) {
            const std::smatch& beginMatch = *sectionBeginIterator;
            const std::smatch& endMatch = *sectionEndIterator;

            std::size_t matchStartIndex = beginMatch.position(0) + beginMatch.length(0);
            std::size_t matchEndIndex = endMatch.position(0);

            if (matchEndIndex < matchStartIndex || matchEndIndex > rawBytes.size()) {
                ++sectionBeginIterator; ++sectionEndIterator;
                continue;
            }

            std::size_t dataBlockSize = matchEndIndex - matchStartIndex;
            const std::uint8_t* binaryDataPointer = rawBytes.data() + matchStartIndex;

            callback(beginMatch, binaryDataPointer, dataBlockSize);

            ++sectionBeginIterator;
            ++sectionEndIterator;
        }
    }

    struct ImportTexture
    {
        MaterialAttributes Attributes{};
        std::unordered_map<std::string, EmbeddedTexture> Textures{};

        ImportTexture() = default;
        ~ImportTexture() = default;
    };

    std::shared_ptr<StaticMesh> Importer::ImportModel(const std::filesystem::path& path, const std::string& exportPath)
    {
        std::string modelName = path.filename().stem().string();
        MOTION_CORE_INFO("Assimp Importer: StaticMesh {0} file successfully loaded to memory from {1} > Converting...", modelName, path.string());

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
                std::ifstream file(finalOutputPath, std::ios::binary | std::ios::ate);
                if (!file.is_open())
                {
                    MOTION_CORE_ERROR("Failed to open file: {}", finalOutputPath.string());
                    return nullptr;
                }

                std::streamsize size = file.tellg();
                file.seekg(0, std::ios::beg);

                std::vector<std::uint8_t> buffer(size);
                file.read(reinterpret_cast<char*>(buffer.data()), size);

                if (!file && buffer.empty())
                {
                    MOTION_CORE_ERROR("Failed to read the entire file: {}", finalOutputPath.string());
                    return nullptr;
                }

                file.close();

                std::string fileStringVersion(reinterpret_cast<const char*>(buffer.data()), size);
                std::uint32_t meshCount = GetMeshCount(fileStringVersion);
                if (meshCount == 0)
                {
                    MOTION_CORE_ERROR("No meshes found in the file: {}", finalOutputPath.string());
                    return nullptr;
                }

                auto& assetManager = AssetManager::GetInstance();
                std::string modelFileName = finalOutputPath.filename().stem().string();
                std::shared_ptr<StaticMesh>  staticMesh = assetManager.Create<StaticMesh>(modelFileName, finalOutputPath);

                for (std::uint32_t i = 0; i < meshCount; ++i)
                {
                    std::vector<float> vertices;
                    std::vector<std::uint32_t> indices;
                    ImportTexture importTexture;

                    std::string meshSectionBegin = std::format(R"(\[M_VTX_{}_BEGIN:(\d+)\])", i);
                    std::string meshSectionEnd = std::format(R"(\[M_VTX_{}_END\])", i);
                    ReadSection(fileStringVersion, meshSectionBegin, meshSectionEnd, buffer,
                        [&](const std::smatch& match, const std::uint8_t* data, std::size_t size)
                        {
                            std::size_t floatCount = size / sizeof(float);
                            const float* floatData = reinterpret_cast<const float*>(data);

                            // Insert into vector
                            vertices.insert(vertices.end(), floatData, floatData + floatCount);
                        });

                    std::string indexSectionBegin = std::format(R"(\[M_IDX_{}_BEGIN:(\d+)\])", i);
                    std::string indexSectionEnd = std::format(R"(\[M_IDX_{}_END\])", i);
                    ReadSection(fileStringVersion, indexSectionBegin, indexSectionEnd, buffer,
                        [&](const std::smatch& match, const std::uint8_t* data, std::size_t size)
                        {
                            std::size_t indexCount = size / sizeof(std::uint32_t);
                            const std::uint32_t* indexData = reinterpret_cast<const std::uint32_t*>(data);

                            // Insert into vector
                            indices.insert(indices.end(), indexData, indexData + indexCount);
                        });

                    std::string matSectionBegin = std::format(R"(\[M_MAT_{}_BEGIN\])", i);
                    std::string matSectionEnd = std::format(R"(\[M_MAT_{}_END\])", i);
                    ReadSection(fileStringVersion, matSectionBegin, matSectionEnd, buffer,
                        [&](const std::smatch& match, const uint8_t* data, std::size_t size)
                        {
                            std::string sectionStr(reinterpret_cast<const char*>(data), size);

                            // --- Extract Texture Blocks ---
                            std::regex textureRegex(
                                R"(\[MAT_TEXTURE_TYPE:(\w+)\|WIDTH:(\d+)\|HEIGHT:(\d+)\|CHANNELS:(\d+)\]\[MAT_TEXTURE_DATA_BEGIN:(\d+)\])"
                            );

                            std::sregex_iterator it(sectionStr.begin(), sectionStr.end(), textureRegex);
                            std::sregex_iterator end;
                            ImportTexture importTexture;

                            while (it != end)
                            {
                                std::string texType = (*it)[1];
                                int32_t width = std::stoi((*it)[2]);
                                int32_t height = std::stoi((*it)[3]);
                                int32_t channels = std::stoi((*it)[4]);
                                size_t texSize = static_cast<size_t>(std::stoul((*it)[5]));

                                std::size_t dataOffset = it->position() + it->length();
                                const uint8_t* textureStart = reinterpret_cast<const uint8_t*>(sectionStr.data()) + dataOffset;

                                EmbeddedTexture texture;
                                texture.Width = width;
                                texture.Height = height;
                                texture.Channels = channels;
                                texture.Size = texSize;
                                texture.Type = texType;

                                if (texSize > 0) {
                                    texture.Data = new std::uint8_t[texSize];
                                    std::memcpy(texture.Data, textureStart, texSize);
                                }
                                else {
                                    texture.Data = nullptr;
                                }

                                importTexture.Textures[texType] = texture;
                                ++it;
                            }

                            std::regex vec3Attr(R"(\[MAT_ATTRIBUTES:\[(\w+)\]:\[(.+?),(.+?),(.+?)\]\])");
                            for (std::sregex_iterator i(sectionStr.begin(), sectionStr.end(), vec3Attr); i != end; ++i) {
                                std::string key = (*i)[1];
                                glm::vec3 value{
                                    std::stof((*i)[2]),
                                    std::stof((*i)[3]),
                                    std::stof((*i)[4])
                                };

                                if (key == MOTION_MAT_ATTRIBUTE_BASE_COLOR)
                                    importTexture.Attributes.BaseColor = value;
                            }

                            std::regex floatAttr(R"(\[MAT_ATTRIBUTES:\[(\w+)\]:(.+?)\])");
                            for (std::sregex_iterator i(sectionStr.begin(), sectionStr.end(), floatAttr); i != end; ++i) {
                                std::string key = (*i)[1];
                                float value = std::stof((*i)[2]);

                                if (key == MOTION_MAT_ATTRIBUTE_METALLIC)
                                    importTexture.Attributes.Metallic = value;
                                else if (key == MOTION_MAT_ATTRIBUTE_ROUGHNESS)
                                    importTexture.Attributes.Roughness = value;
                                else if (key == MOTION_MAT_ATTRIBUTE_AMBIENT_OCCLUSION)
                                    importTexture.Attributes.AmbientOcclusion = value;
                                else if (key == MOTION_MAT_ATTRIBUTE_OPACITY)
                                    importTexture.Attributes.Opacity = value;
                                else
                                    importTexture.Attributes.DisplacementScale = value;
                            }

                            //----------------------------------------------------------------------------------------------

                            StaticMesh::MeshSegment meshSegment{};

                            meshSegment.MeshIndex = i;
                            meshSegment.MeshSelf = Mesh::Create(
                                vertices.data(),
                                static_cast<std::uint32_t>(vertices.size()),
                                indices.data(),
                                static_cast<std::uint32_t>(indices.size()),
                                BufferLayout{
                                    { UniformCache::Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
                                    { UniformCache::TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
                                    { UniformCache::Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
                                    { UniformCache::Tangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent) },
                                    { UniformCache::Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent) }
                                },
                                staticMesh
                            );

                            std::shared_ptr<Material> baseMaterial = assetManager.Get<Material>("BaseMaterial");
                            if (!baseMaterial)
                            {
                                MOTION_ASSERT(baseMaterial, "Base Material not found in AssetManager!");
                                return;
                            }

                            meshSegment.Materials = std::make_shared<MaterialInstance>(baseMaterial);
                            meshSegment.Materials->Attributes = importTexture.Attributes;

                            auto createTexture =
                                [&](const std::string_view& textureName, TextureType type, const EmbeddedTexture& embeddedTexture)
                                {
                                    if (embeddedTexture.Data && embeddedTexture.Size > 0) {
                                        std::shared_ptr<ITexture> texture = ITexture::Create(embeddedTexture.Data, type, embeddedTexture.Width, embeddedTexture.Height, embeddedTexture.Channels);
                                        meshSegment.Materials->Texture[textureName] = texture;
                                    }
                                    else
                                    {
                                        MOTION_CORE_WARN("Texture data for '{}' is empty or invalid.", textureName);
                                        meshSegment.Materials->Texture[textureName] = nullptr;
                                    }
                                };

                            createTexture(UniformCache::BaseColorTextures, TextureType::BaseColorTexture, importTexture.Textures[MOTION_TOSTR(TextureType::BaseColorTexture)]);
                            createTexture(UniformCache::MetallicTextures, TextureType::MetallicTexture, importTexture.Textures[MOTION_TOSTR(TextureType::MetallicTexture)]);
                            createTexture(UniformCache::RoughnessTextures, TextureType::RoughnessTexture, importTexture.Textures[MOTION_TOSTR(TextureType::RoughnessTexture)]);
                            createTexture(UniformCache::AmbientOcclusionTextures, TextureType::AmbientOcclusionTexture, importTexture.Textures[MOTION_TOSTR(TextureType::AmbientOcclusionTexture)]);
                            createTexture(UniformCache::DisplacementTextures, TextureType::DisplacementTexture, importTexture.Textures[MOTION_TOSTR(TextureType::DisplacementTexture)]);
                            createTexture(UniformCache::NormalTextures, TextureType::NormalTexture, importTexture.Textures[MOTION_TOSTR(TextureType::NormalTexture)]);

                            staticMesh->m_Meshes.push_back(meshSegment);
                        });

                    //----------------------------------------------------------------------------------------------

                    vertices.clear();
                    indices.clear();

                    for (auto& [_, texture] : importTexture.Textures)
                    {
                        if (texture.Data)
                            delete[] texture.Data; // Clean up dynamically allocated memory
                    }

                    importTexture.Textures.clear();
                    importTexture.Attributes = MaterialAttributes{}; // Reset attributes for the next mesh
                    MOTION_CORE_INFO("Mesh {0} imported successfully with {1} vertices and {2} indices.", i, vertices.size(), indices.size());
                }

                return staticMesh;
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
