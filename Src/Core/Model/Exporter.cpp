#include "CorePCH.hpp"

namespace Motion
{
    /**
     * @brief Forward declaration of the BinaryWriter template structure.
     *
     * This structure is intended to provide binary writing functionality for objects of type T.
     * The full definition should implement methods for serializing objects of type T to a binary format.
     *
     * @tparam T The type of object to be written in binary format.
     */
    template<typename T>
    struct BinaryWriter;

    /**
     * @brief Writes a section name to the given output stream.
     *
     * This function writes the contents of the specified section name to the provided
     * output stream. The section name is written as raw bytes, without any additional
     * formatting or delimiters.
     *
     * @param outputStream Reference to the output file stream where the section name will be written.
     * @param sectionName The name of the section to write to the output stream.
     */
    static void CreateSection(std::ofstream& outputStream, const std::string& sectionName)
    {
        outputStream.write(sectionName.c_str(), sectionName.size());
    }

    template<>
    struct BinaryWriter<float>
    {
        /**
         * @brief Writes a float value to the given output stream in binary format.
         *
         * This function writes the raw bytes of the provided float value to the specified
         * std::ofstream. The value is written using its memory representation, which may
         * not be portable across different platforms due to endianness or float format differences.
         *
         * @param outputStream Reference to the output file stream where the value will be written.
         * @param value The float value to write to the stream.
         */
        static void Write(std::ofstream& outputStream, const float& value)
        {
            outputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }
    };

    template<>
    struct BinaryWriter<glm::vec2>
    {
        static void Write(std::ofstream& outputStream, const glm::vec2& vec)
        {
            BinaryWriter<float>::Write(outputStream, vec.x);
            BinaryWriter<float>::Write(outputStream, vec.y);
        }
    };

    template<>
    struct BinaryWriter<glm::vec3>
    {
        static void Write(std::ofstream& outputStream, const glm::vec3& vec)
        {
            BinaryWriter<float>::Write(outputStream, vec.x);
            BinaryWriter<float>::Write(outputStream, vec.y);
            BinaryWriter<float>::Write(outputStream, vec.z);
        }
    };

    template<>
    struct BinaryWriter<std::uint32_t>
    {
        static void Write(std::ofstream& outputStream, const std::uint32_t& value)
        {
            outputStream.write(reinterpret_cast<const char*>(&value), sizeof(value));
        }
    };

    template<>
    struct BinaryWriter<Vertex>
    {
        static void Write(std::ofstream& outputStream, const Vertex& vertex)
        {
            BinaryWriter<glm::vec3>::Write(outputStream, vertex.Position);
            BinaryWriter<glm::vec2>::Write(outputStream, vertex.TexCoord);
            BinaryWriter<glm::vec3>::Write(outputStream, vertex.Normal);
            BinaryWriter<glm::vec3>::Write(outputStream, vertex.Tangent);
            BinaryWriter<glm::vec3>::Write(outputStream, vertex.Bitangent);
        }
    };

    template<>
    struct BinaryWriter<std::vector<Vertex>>
    {
        static void Write(std::ofstream& outputStream, const std::vector<Vertex>& vertexDetails)
        {
            for (const auto& vertex : vertexDetails)
            {
                BinaryWriter<Vertex>::Write(outputStream, vertex);
            }
        }
    };

    template<>
    struct BinaryWriter<std::vector<std::uint32_t>>
    {
        static void Write(std::ofstream& outputStream, const std::vector<std::uint32_t>& indices)
        {
            for (const auto& index : indices)
            {
                BinaryWriter<std::uint32_t>::Write(outputStream, index);
            }
        }
    };

    template<>
    struct BinaryWriter<std::uint8_t*>
    {
        static void Write(std::ofstream& outputStream, const std::uint8_t* data, std::size_t size)
        {
            outputStream.write(reinterpret_cast<const char*>(data), size);
        }
    };

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

    void GenerateBoxProjectionUVs(std::vector<Vertex>& vertices)
    {
        if (vertices.empty())
            return;

        std::vector<glm::vec2> rawUVs;
        rawUVs.reserve(vertices.size());

        glm::vec2 minUV = glm::vec2(std::numeric_limits<float>::max());
        glm::vec2 maxUV = glm::vec2(std::numeric_limits<float>::lowest());

        // Step 1: Project positions onto planes based on dominant normal axis
        for (const Vertex& vertex : vertices)
        {
            const glm::vec3& pos = vertex.Position;
            const glm::vec3& normal = glm::normalize(vertex.Normal);

            float absX = std::abs(normal.x);
            float absY = std::abs(normal.y);
            float absZ = std::abs(normal.z);

            glm::vec2 uv;
            if (absX > absY && absX > absZ)
                uv = glm::vec2(pos.y, pos.z); // Project onto YZ
            else if (absY > absZ)
                uv = glm::vec2(pos.x, pos.z); // Project onto XZ
            else
                uv = glm::vec2(pos.x, pos.y); // Project onto XY

            rawUVs.push_back(uv);
            minUV = glm::min(minUV, uv);
            maxUV = glm::max(maxUV, uv);
        }

        // Step 2: Normalize UVs to [0, 1] range
        glm::vec2 uvRange = maxUV - minUV;
        if (uvRange.x == 0.0f) uvRange.x = 1.0f;
        if (uvRange.y == 0.0f) uvRange.y = 1.0f;

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            glm::vec2 uv = (rawUVs[i] - minUV) / uvRange;
            vertices[i].TexCoord = uv;
        }
    }


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

    template<typename T>
    static T GetMaterialAttribute(const aiMaterial* material, const char* key, std::uint32_t type, std::uint32_t index, T defaultValue)
    {
        if (!material)
        {
            MOTION_CORE_ERROR("Material is null, cannot get attribute: {}", key);
            return defaultValue;
        }

        T value{ defaultValue };
        if (material->Get(key, type, index, &value) != aiReturn_SUCCESS)
        {
            MOTION_CORE_WARN("Failed to get material attribute: {}. Using default value.", key);
            return defaultValue;
        }

        return value;
    }

    struct TextureDetails
    {
        std::uint8_t* Data{ nullptr };
        std::size_t Size{ 0 };

        TextureDetails() = default;
        TextureDetails(std::uint8_t* data, std::size_t size)
            : Data(data), Size(size) {
        }

        ~TextureDetails()
        {
            if (Data)
            {
                stbi_image_free(Data);
                Data = nullptr;
            }
        }
    };

    static TextureDetails GetTexture(const aiMaterial* material, aiTextureType textureType, std::uint32_t index)
    {
        if (!material)
        {
            MOTION_CORE_ERROR("Material is null, cannot get texture.");
            return TextureDetails();
        }

        aiString property;
        if (material->GetTexture(textureType, index, &property) != aiReturn_SUCCESS)
        {
            MOTION_CORE_WARN("Failed to get texture of type: {}", textureType);
            return TextureDetails();
        }

        std::filesystem::path texturePath(property.C_Str());
        if (!std::filesystem::exists(texturePath))
        {
            MOTION_CORE_WARN("Texture file does not exist: {}", texturePath.string());
            return TextureDetails();
        }

        int width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        std::uint8_t* data = stbi_load(texturePath.string().c_str(), &width, &height, &channels, 4);
        if (!data)
        {
            MOTION_CORE_ERROR("Failed to load texture data from: {}", texturePath.string());
            return TextureDetails();
        }

        MOTION_CORE_INFO("Successfully loaded texture: {}", texturePath.string());
        return TextureDetails(data, static_cast<std::size_t>(width * height * channels));
    }


    bool Exporter::ExportModel(const aiScene* scene, const std::filesystem::path& output)
    {
        try
        {
            if (!scene)
            {
                MOTION_CORE_ERROR("Cannot export model: Scene is null.");
                return false;
            }

            if (!std::filesystem::exists(output.parent_path()))
            {
                MOTION_CORE_INFO("Creating directory: {}", output.parent_path().string());
                std::filesystem::create_directories(output.parent_path());
            }

            std::ofstream meshFile(output, std::ios::binary);
            if (!meshFile.is_open())
            {
                MOTION_CORE_ERROR("Cannot open file for writing: {}", output.string());
                return false;
            }

            CreateSection(meshFile, MOTION_SECTION_FILE_BEGIN);

            // --------------------------------------------------------
            // Section : [M_META]
            // --------------------------------------------------------
            CreateSection(meshFile, MOTION_SECTION_META_BEGIN);
            CreateSection(meshFile, MOTION_SUBSECTION_META_VERSION("1.0v")); // Version
            CreateSection(meshFile, MOTION_SUBSECTION_META_MESH_COUNT(scene->mNumMeshes));
            CreateSection(meshFile, MOTION_SECTION_META_END);
            //---------------------------------------------------------

            auto exportMeshes =
                [&](std::uint32_t meshIndex, aiMesh* mesh, const aiScene* scene)
                {
                    // --------------------------------------------------------
                    // Loading mesh data
                    // --------------------------------------------------------
                    bool hasUVs = mesh->HasTextureCoords(0);
                    bool hasNormals = mesh->HasNormals();
                    bool hasTangents = mesh->HasTangentsAndBitangents();

                    std::vector<Vertex> vertices{};
                    std::vector<std::uint32_t> indices{};

                    for (std::uint32_t i = 0; i < mesh->mNumVertices; ++i)
                    {
                        Vertex vertex{};
                        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                        if (hasUVs) vertex.TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                        if (hasNormals) vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                        if (hasTangents)
                        {
                            vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                            vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                        }

                        vertices.push_back(vertex);
                    }

                    for (std::uint32_t i = 0; i < mesh->mNumFaces; ++i)
                    {
                        const aiFace& face = mesh->mFaces[i];
                        for (std::uint32_t j = 0; j < face.mNumIndices; ++j)
                        {
                            indices.push_back(face.mIndices[j]);
                        }
                    }

                    if (!hasNormals) GenerateNormals(vertices, indices);
                    if (!hasTangents) GenerateTangents(vertices, indices);
                    if (!hasUVs) GenerateBoxProjectionUVs(vertices);


                    // --------------------------------------------------------
                    // Loading material data
                    // --------------------------------------------------------
                    MaterialAttributes materialAttributes{};
                    std::unordered_map<aiTextureType, TextureDetails> texture{};
                    std::uint32_t materialIndex = mesh->mMaterialIndex;
                    aiMaterial* material = scene->mMaterials[materialIndex];

                    materialAttributes.BaseColor = GetMaterialAttribute<glm::vec3>(material, AI_MATKEY_BASE_COLOR, glm::vec3(1.0f, 1.0f, 1.0f));
                    materialAttributes.Metallic = GetMaterialAttribute<float>(material, AI_MATKEY_METALLIC_FACTOR, 0.0f);
                    materialAttributes.Roughness = GetMaterialAttribute<float>(material, AI_MATKEY_ROUGHNESS_FACTOR, 1.0f);
                    materialAttributes.AmbientOcclusion = 1.0f; // Not provided by Assimp, default to 1.0f
                    materialAttributes.DisplacementScale = 0.05f; // Not provided by Assimp, default to 1.0f
                    materialAttributes.Opacity = GetMaterialAttribute<float>(material, AI_MATKEY_OPACITY, 1.0f);
                    materialAttributes.PADDING1 = 0.0f; // Padding for alignment
                    materialAttributes.PADDING2 = 0.0f; // Padding for alignment

                    texture[aiTextureType_BASE_COLOR] = GetTexture(material, aiTextureType_BASE_COLOR, 0);
                    texture[aiTextureType_METALNESS] = GetTexture(material, aiTextureType_METALNESS, 0);
                    texture[aiTextureType_DIFFUSE_ROUGHNESS] = GetTexture(material, aiTextureType_DIFFUSE_ROUGHNESS, 0);
                    texture[aiTextureType_NORMALS] = GetTexture(material, aiTextureType_NORMALS, 0);
                    texture[aiTextureType_AMBIENT_OCCLUSION] = GetTexture(material, aiTextureType_AMBIENT_OCCLUSION, 0);

                    // --------------------------------------------------------

                    // --------------------------------------------------------
                    // Section : [M_VTX_<meshIndex>]
                    // --------------------------------------------------------
                    CreateSection(meshFile, MOTION_SECTION_VTX_BEGIN(meshIndex, vertices.size()));
                    BinaryWriter<std::vector<Vertex>>::Write(meshFile, vertices);
                    CreateSection(meshFile, MOTION_SECTION_VTX_END(meshIndex));

                    // --------------------------------------------------------
                    // Section : [M_IDX_<meshIndex>]
                    // --------------------------------------------------------
                    CreateSection(meshFile, MOTION_SECTION_IDX_BEGIN(meshIndex, indices.size()));
                    BinaryWriter<std::vector<std::uint32_t>>::Write(meshFile, indices);
                    CreateSection(meshFile, MOTION_SECTION_IDX_END(meshIndex));

                    // --------------------------------------------------------
                    // Section : [M_MAT_<meshIndex>]
                    // --------------------------------------------------------
                    CreateSection(meshFile, MOTION_SECTION_MAT_BEGIN(meshIndex));

                    MOTION_SUBSECTION_MAT_TEXTURE(meshFile, TextureType::BaseColorTexture, texture[aiTextureType_BASE_COLOR].Data, texture[aiTextureType_BASE_COLOR].Size);
                    MOTION_SUBSECTION_MAT_TEXTURE(meshFile, TextureType::MetallicTexture, texture[aiTextureType_METALNESS].Data, texture[aiTextureType_METALNESS].Size);
                    MOTION_SUBSECTION_MAT_TEXTURE(meshFile, TextureType::RoughnessTexture, texture[aiTextureType_DIFFUSE_ROUGHNESS].Data, texture[aiTextureType_DIFFUSE_ROUGHNESS].Size);
                    MOTION_SUBSECTION_MAT_TEXTURE(meshFile, TextureType::AmbientOcclusionTexture, texture[aiTextureType_AMBIENT_OCCLUSION].Data, texture[aiTextureType_AMBIENT_OCCLUSION].Size);
                    MOTION_SUBSECTION_MAT_TEXTURE(meshFile, TextureType::NormalTexture, texture[aiTextureType_NORMALS].Data, texture[aiTextureType_NORMALS].Size);

                    CreateSection(meshFile, MOTION_SUBSECTION_MAT_ATTRIBUTES_BEGIN);
                    CreateSection(meshFile, MOTION_SUBSECTION_MAT_ATTRIBUTES_VEC3(MOTION_MAT_ATTRIBUTE_BASE_COLOR, materialAttributes.BaseColor));
                    CreateSection(meshFile, MOTION_SUBSECTION_MAT_ATTRIBUTES_FLOAT(MOTION_MAT_ATTRIBUTE_METALLIC, materialAttributes.Metallic));
                    CreateSection(meshFile, MOTION_SUBSECTION_MAT_ATTRIBUTES_FLOAT(MOTION_MAT_ATTRIBUTE_ROUGHNESS, materialAttributes.Roughness));
                    CreateSection(meshFile, MOTION_SUBSECTION_MAT_ATTRIBUTES_FLOAT(MOTION_MAT_ATTRIBUTE_AMBIENT_OCCLUSION, materialAttributes.AmbientOcclusion));
                    CreateSection(meshFile, MOTION_SUBSECTION_MAT_ATTRIBUTES_FLOAT(MOTION_MAT_ATTRIBUTE_OPACITY, materialAttributes.Opacity));
                    CreateSection(meshFile, MOTION_SUBSECTION_MAT_ATTRIBUTES_FLOAT(MOTION_MAT_ATTRIBUTE_DISPLACEMENT_SCALE, materialAttributes.DisplacementScale));
                    CreateSection(meshFile, MOTION_SUBSECTION_MAT_ATTRIBUTES_END);

                    CreateSection(meshFile, MOTION_SECTION_MAT_END(meshIndex));
                };

            std::function<void(aiNode*, const aiScene*)> traverseNodes =
                [&](aiNode* node, const aiScene* scene)
                {
                    for (std::uint32_t i = 0; i < node->mNumMeshes; ++i)
                    {
                        std::uint32_t meshIndex = node->mMeshes[i];
                        aiMesh* mesh = scene->mMeshes[meshIndex];
                        exportMeshes(meshIndex, mesh, scene);
                    }

                    for (std::uint32_t i = 0; i < node->mNumChildren; ++i)
                    {
                        traverseNodes(node->mChildren[i], scene);
                    }
                };

            traverseNodes(scene->mRootNode, scene);
            // --------------------------------------------------------
            CreateSection(meshFile, MOTION_SECTION_FILE_END);

            meshFile.close();
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
}