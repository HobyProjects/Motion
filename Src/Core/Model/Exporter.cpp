#include "CorePCH.hpp"

namespace Motion
{
    template<typename T>
    struct BinaryWriter;

    static void CreateSection(std::ofstream& outputStream, const std::string& sectionName)
    {
        outputStream.write(sectionName.c_str(), sectionName.size());
    }

    template<>
    struct BinaryWriter<float>
    {
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

    template<>
    struct BinaryWriter<MaterialAttributes>
    {
        static void Write(std::ofstream& outputStream, const MaterialAttributes& attributes)
        {
            BinaryWriter<glm::vec3>::Write(outputStream, attributes.BaseColor);
            BinaryWriter<float>::Write(outputStream, attributes.Metallic);
            BinaryWriter<float>::Write(outputStream, attributes.Roughness);
            BinaryWriter<float>::Write(outputStream, attributes.AmbientOcclusion);
            BinaryWriter<float>::Write(outputStream, attributes.Opacity);
            BinaryWriter<float>::Write(outputStream, attributes.DisplacementScale);

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
        if (material->Get(key, type, index, value) != aiReturn_SUCCESS)
        {
            MOTION_CORE_WARN("Failed to get material attribute: {}. Using default value.", key);
            return defaultValue;
        }

        return value;
    }

    static EmbeddedTexture GetTexture(const aiMaterial* material, aiTextureType textureType, TextureType type, std::uint32_t index)
    {
        if (!material)
        {
            MOTION_CORE_ERROR("Material is null, cannot get texture.");
            return EmbeddedTexture();
        }

        aiString property;
        if (material->GetTexture(textureType, index, &property) != aiReturn_SUCCESS)
        {
            MOTION_CORE_WARN("Failed to get texture of type: {}", static_cast<std::int32_t>(textureType));
            return EmbeddedTexture();
        }

        std::filesystem::path texturePath(property.C_Str());
        if (!std::filesystem::exists(texturePath))
        {
            MOTION_CORE_WARN("Texture file does not exist: {}", texturePath.string());
            return EmbeddedTexture();
        }

        std::int32_t width, height, channels;
        stbi_set_flip_vertically_on_load(1);
        std::uint8_t* data = stbi_load(texturePath.string().c_str(), &width, &height, &channels, 4);
        if (!data)
        {
            MOTION_CORE_ERROR("Failed to load texture data from: {}", texturePath.string());
            return EmbeddedTexture();
        }

        MOTION_CORE_INFO("Successfully loaded texture: {}", texturePath.string());
        return EmbeddedTexture(data, static_cast<std::size_t>(width * height * channels), width, height, channels, MOTION_TOSTR(type));
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

            auto [minBounds, maxBounds] = ComputeGlobalBounds(scene);
            CreateSection(meshFile, MESH_BEGIN);

            // --------------------------------------------------------
            // CREATING THE HEADER
            // --------------------------------------------------------
            CreateSection(meshFile, META_BEGIN);
            CreateSection(meshFile, META_VERSION("1.0.0"));
            CreateSection(meshFile, META_ENDIAN("LITTLE"));
            CreateSection(meshFile, META_COUNT(scene->mNumMeshes));
            CreateSection(meshFile, META_BOUNDS(minBounds, maxBounds));
            CreateSection(meshFile, META_ID(UniqueIdentity::GetUniqueID()));
            CreateSection(meshFile, META_ENGINE("MotionEngine"));
            CreateSection(meshFile, META_BUILD(DateTime::GetDate()));
            CreateSection(meshFile, META_END);
            //---------------------------------------------------------

            std::unordered_map<std::uint32_t, std::string> meshMaterialIDs{};
            std::unordered_map<std::string, MaterialAttributes> meshMaterialAttributesMapper{};

            std::unordered_map<std::string, EmbeddedTexture> materialTextureMapper{};
            std::unordered_map<std::string, std::vector< std::string>> meshMaterialTexturesMapper{};

            auto loadExportingMesh =
                [&](std::uint32_t meshIndex, aiMesh* mesh, const aiScene* scene)
                {
                    if (!mesh)
                        return;

                    // --------------------------------------------------------
                    //  GETTING MESH DATA FROM ASSIMP
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

                        if (hasUVs)
                            vertex.TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
                        else
                            vertex.TexCoord = glm::vec2(0.0f, 0.0f); // Default UVs if not present


                        if (hasNormals)
                            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
                        else
                            vertex.Normal = glm::vec3(0.0f, 0.0f, 1.0f); // Default normal if not present

                        if (hasTangents)
                        {
                            vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                            vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
                        }
                        else
                        {
                            vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f); // Default tangent if not present
                            vertex.Bitangent = glm::vec3(0.0f, 1.0f, 0.0f); // Default bitangent if not present
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

                    //[TODO]: Need to generate normals and tangents if not present, also generate UVs if not present
                    //if (!hasNormals) GenerateNormals(vertices, indices);
                    //if (!hasTangents) GenerateTangents(vertices, indices);
                    //if (!hasUVs) GenerateBoxProjectionUVs(vertices);

                    // --------------------------------------------------------
                    // CREATING SECTIONS FOR MESH n VERTEX DATA
                    // --------------------------------------------------------
                    CreateSection(meshFile, VTX_BEGIN(vertices.size()));
                    BinaryWriter<std::vector<Vertex>>::Write(meshFile, vertices);
                    CreateSection(meshFile, VTX_END);

                    // --------------------------------------------------------
                    // CREATING SECTIONS FOR MESH n INDICES DATA
                    // --------------------------------------------------------
                    CreateSection(meshFile, IDX_BEGIN(indices.size()));
                    BinaryWriter<std::vector<std::uint32_t>>::Write(meshFile, indices);
                    CreateSection(meshFile, IDX_END);

                    // --------------------------------------------------------
                    // CREATING SECTIONS FOR MESH n MATERIAL REFERENCE
                    // --------------------------------------------------------
                    if (meshMaterialIDs.contains(meshIndex))
                    {
                        CreateSection(meshFile, MAT_REF(meshMaterialIDs[meshIndex]));
                    }
                };

            auto loadExportingMaterials =
                [&](std::uint32_t meshIndex, aiMaterial* material)
                {
                    if (!material)
                        return;

                    std::string materialID = std::format("MAT_{}{}", meshIndex, HashString(material->GetName().C_Str()));
                    //MOTION_ASSERT((meshMaterialIDs.contains(meshIndex) && meshMaterialIDs[meshIndex] == materialID), "Hash collision detected for material ID: {}", materialID);
                    meshMaterialIDs[meshIndex] = materialID;

                    MaterialAttributes materialAttributes{};
                    aiColor3D baseColor = GetMaterialAttribute<aiColor3D>(material, AI_MATKEY_BASE_COLOR, aiColor3D(1.0f));
                    materialAttributes.BaseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
                    materialAttributes.Metallic = GetMaterialAttribute<float>(material, AI_MATKEY_METALLIC_FACTOR, 0.0f);
                    materialAttributes.Roughness = GetMaterialAttribute<float>(material, AI_MATKEY_ROUGHNESS_FACTOR, 1.0f);
                    materialAttributes.AmbientOcclusion = 1.0f; // Not provided by Assimp, default to 1.0f
                    materialAttributes.Opacity = GetMaterialAttribute<float>(material, AI_MATKEY_OPACITY, 1.0f);
                    materialAttributes.DisplacementScale = 0.05f; // Not provided by Assimp, default to 1.0f
                    meshMaterialAttributesMapper[materialID] = materialAttributes;

                    auto getTextureTypeString =
                        [](TextureType type) -> std::string
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

                    auto loadExportingTexture =
                        [&](aiTextureType textureType, TextureType type)
                        {
                            aiString property;
                            if (material->GetTexture(textureType, 0, &property) != aiReturn_SUCCESS)
                                return;


                            std::filesystem::path texturePath(property.C_Str());
                            if (!std::filesystem::exists(texturePath))
                                return;

                            std::string textureID = std::format("{}_{}", "TEX", HashString(texturePath.string()));
                            if (materialTextureMapper.contains(textureID))
                            {
                                meshMaterialTexturesMapper[materialID].push_back(textureID);
                                return;
                            }

                            stbi_set_flip_vertically_on_load(1);
                            std::int32_t width{ 0 }, height{ 0 }, channels{ 0 };
                            std::uint8_t* data = stbi_load(texturePath.string().c_str(), &width, &height, &channels, 4);
                            if (!data)
                                return;

                            MOTION_CORE_INFO("Successfully loaded texture: {}", texturePath.string());
                            materialTextureMapper[textureID] = { data, static_cast<std::size_t>(width * height * channels), width, height, channels, getTextureTypeString(type) };
                            meshMaterialTexturesMapper[materialID].push_back(textureID);
                        };

                    loadExportingTexture(aiTextureType_BASE_COLOR, TextureType::BaseColorTexture);
                    loadExportingTexture(aiTextureType_METALNESS, TextureType::MetallicTexture);
                    loadExportingTexture(aiTextureType_DIFFUSE_ROUGHNESS, TextureType::RoughnessTexture);
                    loadExportingTexture(aiTextureType_AMBIENT_OCCLUSION, TextureType::AmbientOcclusionTexture);
                    loadExportingTexture(aiTextureType_NORMALS, TextureType::NormalTexture);
                };

            std::function<void(const aiNode*, const aiScene*)> traverseNodes =
                [&](const aiNode* node, const aiScene* scene)
                {
                    for (std::uint32_t i = 0; i < node->mNumMeshes; ++i)
                    {
                        std::uint32_t meshIndex = node->mMeshes[i];
                        aiMesh* mesh = scene->mMeshes[meshIndex];
                        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

                        if (material)
                        {
                            loadExportingMaterials(meshIndex, material);
                        }
                    }

                    for (std::uint32_t i = 0; i < node->mNumMeshes; ++i)
                    {
                        std::uint32_t meshIndex = node->mMeshes[i];
                        aiMesh* mesh = scene->mMeshes[meshIndex];
                        if (mesh)
                        {
                            CreateSection(meshFile, MESH_BLOCK(i));
                            loadExportingMesh(meshIndex, mesh, scene);
                            CreateSection(meshFile, MESH_BLOCK_END(i));
                        }
                    }

                    for (std::uint32_t i = 0; i < node->mNumChildren; ++i)
                    {
                        traverseNodes(node->mChildren[i], scene);
                    }
                };

            traverseNodes(scene->mRootNode, scene);

            // --------------------------------------------------------
            // CREATING MATERIAL SECTION
            // --------------------------------------------------------
            CreateSection(meshFile, MATERIALS_BEGIN);
            for (const auto& [materialID, attribute] : meshMaterialAttributesMapper)
            {
                CreateSection(meshFile, MATERIAL(materialID));

                CreateSection(meshFile, MAT_ATTR_BEGIN);
                CreateSection(meshFile, ATTR_VEC3("BaseColor", attribute.BaseColor));
                CreateSection(meshFile, ATTR_FLOAT("Metallic", attribute.Metallic));
                CreateSection(meshFile, ATTR_FLOAT("Roughness", attribute.Roughness));
                CreateSection(meshFile, ATTR_FLOAT("AmbientOcclusion", attribute.AmbientOcclusion));
                CreateSection(meshFile, ATTR_FLOAT("Opacity", attribute.Opacity));
                CreateSection(meshFile, ATTR_FLOAT("DisplacementScale", attribute.DisplacementScale));
                CreateSection(meshFile, MAT_ATTR_END);

                for (const auto& textureID : meshMaterialTexturesMapper[materialID])
                {
                    if (materialTextureMapper.contains(textureID))
                    {
                        CreateSection(meshFile, TEX_REF(materialTextureMapper[textureID].Type, textureID));
                    }
                }

                CreateSection(meshFile, MATERIAL_END);
            }
            CreateSection(meshFile, MATERIALS_END);

            // --------------------------------------------------------
            // CREATING TEXTURE SECTION
            // --------------------------------------------------------
            CreateSection(meshFile, TEXTURES_BEGIN);
            for (const auto& [textureID, texture] : materialTextureMapper)
            {
                CreateSection(meshFile, TEXTURE(textureID));
                CreateSection(meshFile, TEX_META(texture.Type, texture.Width, texture.Height, texture.Channels, texture.Size));
                CreateSection(meshFile, TEXTURE_DATA_BEGIN);
                BinaryWriter<std::uint8_t*>::Write(meshFile, texture.Data, texture.Size);
                CreateSection(meshFile, TEXTURE_DATA_END);
                CreateSection(meshFile, TEXTURE_END);

                // Free the texture data after writing to file
                stbi_image_free(texture.Data);
            }
            CreateSection(meshFile, TEXTURES_END);

            // --------------------------------------------------------
            // CREATING FILE END SECTION
            // --------------------------------------------------------
            CreateSection(meshFile, MESH_END);

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