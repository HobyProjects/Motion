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
        auto sectionBeginIterator = std::sregex_iterator(dataStringVersion.begin(), dataStringVersion.end(), sectionBeginRegex);
        auto sectionEndIterator = std::sregex_iterator(dataStringVersion.begin(), dataStringVersion.end(), sectionEndRegex);

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

                if (!file)
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

                for (std::uint32_t i = 0; i < meshCount; ++i)
                {
                    std::vector<float> vertices;
                    std::vector<std::uint32_t> indices;
                    std::unordered_map<std::string, std::vector<uint8_t>> textureDataMap;
                    std::unordered_map<std::string, float> floatAttributes;
                    std::unordered_map<std::string, glm::vec3> vec3Attributes;

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

                            std::regex textureRegex(R"(\[MAT_TEXTURE_TYPE:(\w+)\]\[MAT_TEXTURE_DATA_BEGIN:(\d+)\])");
                            std::sregex_iterator it(sectionStr.begin(), sectionStr.end(), textureRegex);
                            std::sregex_iterator end;

                            size_t offset = 0;
                            while (it != end) {
                                std::string type = (*it)[1];
                                std::size_t texSize = std::stoul((*it)[2]);
                                offset = it->position() + it->length();

                                const uint8_t* textureStart = reinterpret_cast<const uint8_t*>(sectionStr.data()) + offset;
                                textureDataMap[type] = std::vector<uint8_t>(textureStart, textureStart + texSize);

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
                                vec3Attributes[key] = value;
                            }

                            std::regex floatAttr(R"(\[MAT_ATTRIBUTES:\[(\w+)\]:(.+?)\])");
                            for (std::sregex_iterator i(sectionStr.begin(), sectionStr.end(), floatAttr); i != end; ++i) {
                                std::string key = (*i)[1];
                                float value = std::stof((*i)[2]);
                                floatAttributes[key] = value;
                            }
                        });
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
