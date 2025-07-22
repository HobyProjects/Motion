#include "CorePCH.hpp"
#include "Mesh.hpp"

namespace Motion::Core
{

    /**
     * @brief Constructs a Mesh object with the specified UUID and parameters.
     *
     * Initializes the mesh with vertex and index data, sets up the buffer layout,
     * and associates the mesh with a parent model. This constructor creates the
     * necessary vertex and element buffers, assigns the buffer layout, and
     * configures the vertex array object for rendering.
     *
     * @param uuid The unique identifier for the mesh.
     * @param name The name of the mesh.
     * @param vertices Pointer to the array of vertex data.
     * @param verticesSize The size (in bytes) of the vertex data array.
     * @param indices Pointer to the array of index data.
     * @param indicesCount The number of indices in the index array.
     * @param layout The layout describing the structure of the vertex buffer.
     * @param parentModel Shared pointer to the parent StaticMesh object.
     */
    Mesh::Mesh(UUID uuid, const std::string& name, float* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount,
        const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel)
        : AssetBase<IAsset>(uuid, name, AssetType::Mesh, "Undefined")
        , m_ParentModel(parentModel)
        , m_IndicesCount(indicesCount)
    {
        m_VertexBuffer = BufferFactory::CreateVertexBuffer(vertices, verticesSize);
        m_ElementBuffer = BufferFactory::CreateElementBuffer(indices, indicesCount);
        m_VertexBuffer->SetLayout(layout);

        m_VertexArray = ArrayFactory::CreateVertexArray();
        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);

        if (m_VertexBuffer && m_ElementBuffer && m_VertexArray)
        {
            MOTION_CORE_INFO("Mesh '{}' created successfully with {} vertices and {} indices.", name, verticesSize / sizeof(float), indicesCount);
            AssetInfo.IsInitialized = true;
        }
        else
        {
            MOTION_ASSERT(false, "Failed to create Mesh '{}': VertexBuffer, ElementBuffer, or VertexArray is null.", name);
            AssetInfo.IsInitialized = false;
        }
    }

    /**
     * @brief Binds the vertex array object for rendering.
     *
     * This function binds the vertex array associated with the mesh, making it
     * ready for rendering operations. It ensures that the correct vertex and
     * index buffers are active for subsequent draw calls.
     */
    void Mesh::Bind() const noexcept
    {
        if (m_VertexArray)
            m_VertexArray->Bind();
    }

    /**
     * @brief Unbinds the vertex array object.
     *
     * This function unbinds the vertex array associated with the mesh, effectively
     * disabling it for rendering operations. It is typically called after rendering
     * is complete to reset the OpenGL state.
     */
    void Mesh::Unbind() const noexcept
    {
        if (m_VertexArray)
            m_VertexArray->Unbind();
    }

    /**
     * @brief Renders the mesh using the renderer instance.
     *
     * This function binds the mesh, issues a draw call to the renderer
     * using the number of indices in the mesh, and then unbinds the mesh.
     * It encapsulates the rendering process for this mesh object.
     */
    void Mesh::Render()
    {
        Bind();
        Renderer::DrawIndexed(m_IndicesCount);
        Unbind();
    }

    /**
     * @brief Returns the number of indices in the mesh.
     *
     * This function retrieves the count of indices used for rendering the mesh,
     * which is essential for determining how many vertices to draw during rendering.
     *
     * @return The number of indices in the mesh.
     */
    std::uint32_t Mesh::GetIndicesCount() const noexcept
    {
        return m_IndicesCount;
    }

    /**
     * @brief Returns the parent model associated with the mesh.
     *
     * This function retrieves the shared pointer to the parent StaticMesh object that
     * owns this mesh, allowing access to model-level properties and methods.
     *
     * @return Shared pointer to the parent StaticMesh object.
     */
    std::shared_ptr<StaticMesh> Mesh::GetParentModel() const noexcept
    {
        return m_ParentModel;
    }

    /**
     * @brief Creates a plane mesh with the specified dimensions and segment counts.
     *
     * This function generates a plane mesh centered at the origin on the XZ plane (Y up),
     * with configurable width, height, and subdivisions along both axes. It computes
     * vertex positions, normals, texture coordinates, tangents, and bitangents for each vertex.
     * The mesh is constructed as a grid of quads, each split into two triangles.
     *
     * @param isRegistered Indicates whether the mesh should be registered with the AssetManager.
     * @param name The name to assign to the mesh asset.
     * @param width The total width of the plane along the X axis.
     * @param height The total height of the plane along the Z axis.
     * @param widthSegments Number of subdivisions along the width (X axis).
     * @param heightSegments Number of subdivisions along the height (Z axis).
     * @return std::shared_ptr<Mesh> A shared pointer to the created Mesh object, or nullptr if creation fails.
     *
     * @note The mesh is registered with the AssetManager and uses a generated name if creation succeeds.
     * @note Tangents and bitangents are calculated per triangle and accumulated per vertex, then normalized.
     */
    std::shared_ptr<Mesh> QuickMesh::CreatePlane(bool isRegistered, const std::string name, float width, float height, std::uint32_t widthSegments, std::uint32_t heightSegments)
    {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
        float dx = width / static_cast<float>(widthSegments);
        float dy = height / static_cast<float>(heightSegments);

        for (std::uint32_t y = 0; y <= heightSegments; ++y) {
            for (std::uint32_t x = 0; x <= widthSegments; ++x) {
                float xpos = -halfWidth + x * dx;
                float ypos = -halfHeight + y * dy;
                float u = static_cast<float>(x) / widthSegments;
                float v = static_cast<float>(y) / heightSegments;

                Vertex vert;
                vert.Position = glm::vec3(xpos, 0.0f, ypos);
                vert.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vert.TexCoord = glm::vec2(u, v);
                vert.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);    // X axis
                vert.Bitangent = glm::vec3(0.0f, 0.0f, -1.0f); // -Z axis
                vertices.push_back(vert);
            }
        }

        for (std::uint32_t y = 0; y < heightSegments; ++y) {
            for (std::uint32_t x = 0; x < widthSegments; ++x) {
                std::uint32_t i0 = y * (widthSegments + 1) + x;
                std::uint32_t i1 = i0 + 1;
                std::uint32_t i2 = i0 + (widthSegments + 1);
                std::uint32_t i3 = i2 + 1;

                // First triangle
                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i1);

                // Second triangle
                indices.push_back(i1);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }

        // Calculate tangents and bitangents per triangle
        for (size_t i = 0; i < indices.size(); i += 3) {
            Vertex& v0 = vertices[indices[i + 0]];
            Vertex& v1 = vertices[indices[i + 1]];
            Vertex& v2 = vertices[indices[i + 2]];

            glm::vec3 edge1 = v1.Position - v0.Position;
            glm::vec3 edge2 = v2.Position - v0.Position;
            glm::vec2 deltaUV1 = v1.TexCoord - v0.TexCoord;
            glm::vec2 deltaUV2 = v2.TexCoord - v0.TexCoord;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            glm::vec3 tangent, bitangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
            tangent = glm::normalize(tangent);

            bitangent.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
            bitangent.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
            bitangent.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
            bitangent = glm::normalize(bitangent);

            v0.Tangent += tangent;
            v1.Tangent += tangent;
            v2.Tangent += tangent;
            v0.Bitangent += bitangent;
            v1.Bitangent += bitangent;
            v2.Bitangent += bitangent;
        }

        // Normalize tangents and bitangents
        for (auto& v : vertices) {
            v.Tangent = glm::normalize(v.Tangent);
            v.Bitangent = glm::normalize(v.Bitangent);
        }

        // Prepare buffer data
        std::vector<float> vertexData;
        for (const auto& v : vertices) {
            vertexData.push_back(v.Position.x);
            vertexData.push_back(v.Position.y);
            vertexData.push_back(v.Position.z);
            vertexData.push_back(v.Normal.x);
            vertexData.push_back(v.Normal.y);
            vertexData.push_back(v.Normal.z);
            vertexData.push_back(v.TexCoord.x);
            vertexData.push_back(v.TexCoord.y);
            vertexData.push_back(v.Tangent.x);
            vertexData.push_back(v.Tangent.y);
            vertexData.push_back(v.Tangent.z);
            vertexData.push_back(v.Bitangent.x);
            vertexData.push_back(v.Bitangent.y);
            vertexData.push_back(v.Bitangent.z);
        }

        BufferLayout layout({
            { UniformCache::VertexAttri_Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
            { UniformCache::VertexAttri_TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
            { UniformCache::VertexAttri_Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
            { UniformCache::VertexAttri_Tangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent) },
            { UniformCache::VertexAttri_Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent) }
            });

        static std::uint32_t meshCount = 0;
        std::shared_ptr<Mesh> mesh = nullptr;

        if (isRegistered)
        {
            mesh = AssetManager::GetInstance().Create<Mesh>(
                std::format("{}_{}", name, meshCount++).c_str(),
                vertexData.data(), vertexData.size(),
                indices.data(), indices.size(),
                layout, nullptr
            );
        }
        else
        {
            mesh = std::make_shared<Mesh>(
                UniqueIdentity::GetUniqueID(),
                std::format("{}_{}", name, meshCount++),
                vertexData.data(), vertexData.size(),
                indices.data(), indices.size(),
                layout, nullptr
            );
        }


        if (mesh)
        {
            MOTION_CORE_INFO("Created Plane Mesh: '{}'", mesh->GetName());
            return mesh;
        }

        MOTION_CORE_ERROR("Failed to create Plane Mesh: '{}'", name);
        return nullptr;
    }

    /**
     * @brief Creates a cube mesh with the specified dimensions.
     *
     * This static function generates a cube mesh centered at the origin, with the given width, height, and depth.
     * The cube is constructed with 24 unique vertices (4 per face) to ensure correct normals, tangents, and bitangents
     * for each face, which is important for proper lighting and normal mapping. The function also sets up texture
     * coordinates for each face.
     *
     * @param isRegistered Indicates whether the mesh should be registered with the AssetManager.
     * @param name The name to assign to the mesh asset.
     * @param width The width of the cube along the X axis.
     * @param height The height of the cube along the Y axis.
     * @param depth The depth of the cube along the Z axis.
     * @return std::shared_ptr<Mesh> A shared pointer to the created Mesh object, or nullptr if creation failed.
     */
    std::shared_ptr<Mesh> QuickMesh::CreateCube(bool isRegistered, const std::string name, float width, float height, float depth)
    {
        // Half dimensions
        float hw = width * 0.5f;
        float hh = height * 0.5f;
        float hd = depth * 0.5f;

        // Cube vertices (8 corners)
        glm::vec3 positions[8] = {
            {-hw, -hh, -hd}, // 0
            { hw, -hh, -hd}, // 1
            { hw,  hh, -hd}, // 2
            {-hw,  hh, -hd}, // 3
            {-hw, -hh,  hd}, // 4
            { hw, -hh,  hd}, // 5
            { hw,  hh,  hd}, // 6
            {-hw,  hh,  hd}  // 7
        };

        // Cube faces (each face has 4 vertices, but we need unique normals/tangents per face, so 24 vertices)
        struct Face {
            int idx[4];
            glm::vec3 normal;
            glm::vec3 tangent;
            glm::vec3 bitangent;
        };

        Face faces[6] = {
            // -Z (back)
            {{0, 1, 2, 3}, {0, 0, -1}, {1, 0, 0}, {0, 1, 0}},
            // +Z (front)
            {{5, 4, 7, 6}, {0, 0, 1}, {-1, 0, 0}, {0, 1, 0}},
            // -X (left)
            {{4, 0, 3, 7}, {-1, 0, 0}, {0, 0, -1}, {0, 1, 0}},
            // +X (right)
            {{1, 5, 6, 2}, {1, 0, 0}, {0, 0, 1}, {0, 1, 0}},
            // -Y (bottom)
            {{4, 5, 1, 0}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}},
            // +Y (top)
            {{3, 2, 6, 7}, {0, 1, 0}, {1, 0, 0}, {0, 0, -1}}
        };

        glm::vec2 uvs[4] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        for (int f = 0; f < 6; ++f) {
            int base = static_cast<int>(vertices.size());
            for (int v = 0; v < 4; ++v) {
                Vertex vert;
                vert.Position = positions[faces[f].idx[v]];
                vert.Normal = faces[f].normal;
                vert.TexCoord = uvs[v];
                vert.Tangent = faces[f].tangent;
                vert.Bitangent = faces[f].bitangent;
                vertices.push_back(vert);
            }
            // Two triangles per face
            indices.push_back(base + 0);
            indices.push_back(base + 1);
            indices.push_back(base + 2);
            indices.push_back(base + 2);
            indices.push_back(base + 3);
            indices.push_back(base + 0);
        }

        // Prepare buffer data
        std::vector<float> vertexData;
        for (const auto& v : vertices) {
            vertexData.push_back(v.Position.x);
            vertexData.push_back(v.Position.y);
            vertexData.push_back(v.Position.z);
            vertexData.push_back(v.Normal.x);
            vertexData.push_back(v.Normal.y);
            vertexData.push_back(v.Normal.z);
            vertexData.push_back(v.TexCoord.x);
            vertexData.push_back(v.TexCoord.y);
            vertexData.push_back(v.Tangent.x);
            vertexData.push_back(v.Tangent.y);
            vertexData.push_back(v.Tangent.z);
            vertexData.push_back(v.Bitangent.x);
            vertexData.push_back(v.Bitangent.y);
            vertexData.push_back(v.Bitangent.z);
        }

        BufferLayout layout({
            { UniformCache::VertexAttri_Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
            { UniformCache::VertexAttri_TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
            { UniformCache::VertexAttri_Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
            { UniformCache::VertexAttri_Tangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent) },
            { UniformCache::VertexAttri_Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent) }
            });

        static std::uint32_t meshCount = 0;
        std::shared_ptr<Mesh> mesh = nullptr;

        if (isRegistered)
        {
            mesh = AssetManager::GetInstance().Create<Mesh>(
                std::format("{}_{}", name, meshCount++).c_str(),
                vertexData.data(), static_cast<std::uint32_t>(vertexData.size()),
                indices.data(), static_cast<std::uint32_t>(indices.size()),
                layout, nullptr
            );
        }
        else
        {
            mesh = std::make_shared<Mesh>(
                UniqueIdentity::GetUniqueID(),
                std::format("{}_{}", name, meshCount++),
                vertexData.data(), static_cast<std::uint32_t>(vertexData.size()),
                indices.data(), static_cast<std::uint32_t>(indices.size()),
                layout, nullptr
            );
        }

        if (mesh)
        {
            MOTION_CORE_INFO("Created Cube Mesh: '{}'", mesh->GetName());
            return mesh;
        }

        MOTION_CORE_ERROR("Failed to create Cube Mesh: '{}'", name);
        return nullptr;
    }

    /**
     * @brief Creates a sphere mesh with the specified sector and stack counts.
     *
     * This static function generates a sphere mesh centered at the origin, with configurable
     * sector (longitude) and stack (latitude) subdivisions. The sphere has a radius of 1.0.
     * Vertex positions, normals, texture coordinates, tangents, and bitangents are computed.
     *
     * @param isRegistered Indicates whether the mesh should be registered with the AssetManager.
     * @param name The name to assign to the mesh asset.
     * @param sectorCount Number of longitudinal slices (minimum 3).
     * @param stackCount Number of latitudinal slices (minimum 2).
     * @return std::shared_ptr<Mesh> A shared pointer to the created Mesh object, or nullptr if creation failed.
     */
    std::shared_ptr<Mesh> QuickMesh::CreateSphere(bool isRegistered, const std::string name, std::uint32_t sectorCount, std::uint32_t stackCount)
    {
        if (sectorCount < 3) sectorCount = 3;
        if (stackCount < 2) stackCount = 2;

        constexpr float PI = 3.14159265359f;
        float radius = 1.0f;

        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;

        // Generate vertices
        for (std::uint32_t i = 0; i <= stackCount; ++i) {
            float stackAngle = PI / 2 - i * (PI / stackCount); // from pi/2 to -pi/2
            float xy = radius * cosf(stackAngle);
            float y = radius * sinf(stackAngle);

            for (std::uint32_t j = 0; j <= sectorCount; ++j) {
                float sectorAngle = j * (2 * PI / sectorCount); // from 0 to 2pi

                float x = xy * cosf(sectorAngle);
                float z = xy * sinf(sectorAngle);

                float u = (float)j / sectorCount;
                float v = (float)i / stackCount;

                glm::vec3 pos(x, y, z);
                glm::vec3 normal = glm::normalize(pos);

                // Calculate tangent (points along increasing u, i.e., along sector)
                glm::vec3 tangent(-sinf(sectorAngle), 0.0f, cosf(sectorAngle));
                // Calculate bitangent (cross product of normal and tangent)
                glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

                Vertex vert;
                vert.Position = pos;
                vert.Normal = normal;
                vert.TexCoord = glm::vec2(u, v);
                vert.Tangent = tangent;
                vert.Bitangent = bitangent;
                vertices.push_back(vert);
            }
        }

        // Generate indices
        for (std::uint32_t i = 0; i < stackCount; ++i) {
            for (std::uint32_t j = 0; j < sectorCount; ++j) {
                std::uint32_t first = i * (sectorCount + 1) + j;
                std::uint32_t second = first + sectorCount + 1;

                if (i != 0) {
                    indices.push_back(first);
                    indices.push_back(second);
                    indices.push_back(first + 1);
                }
                if (i != (stackCount - 1)) {
                    indices.push_back(first + 1);
                    indices.push_back(second);
                    indices.push_back(second + 1);
                }
            }
        }

        // Prepare buffer data
        std::vector<float> vertexData;
        for (const auto& v : vertices) {
            vertexData.push_back(v.Position.x);
            vertexData.push_back(v.Position.y);
            vertexData.push_back(v.Position.z);
            vertexData.push_back(v.Normal.x);
            vertexData.push_back(v.Normal.y);
            vertexData.push_back(v.Normal.z);
            vertexData.push_back(v.TexCoord.x);
            vertexData.push_back(v.TexCoord.y);
            vertexData.push_back(v.Tangent.x);
            vertexData.push_back(v.Tangent.y);
            vertexData.push_back(v.Tangent.z);
            vertexData.push_back(v.Bitangent.x);
            vertexData.push_back(v.Bitangent.y);
            vertexData.push_back(v.Bitangent.z);
        }

        BufferLayout layout({
            { UniformCache::VertexAttri_Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
            { UniformCache::VertexAttri_TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
            { UniformCache::VertexAttri_Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
            { UniformCache::VertexAttri_Tangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent) },
            { UniformCache::VertexAttri_Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent) }
            });

        static std::uint32_t meshCount = 0;
        std::shared_ptr<Mesh> mesh = nullptr;

        if (isRegistered)
        {
            mesh = AssetManager::GetInstance().Create<Mesh>(
                std::format("{}_{}", name, meshCount++).c_str(),
                vertexData.data(), static_cast<std::uint32_t>(vertexData.size()),
                indices.data(), static_cast<std::uint32_t>(indices.size()),
                layout, nullptr
            );
        }
        else
        {
            mesh = std::make_shared<Mesh>(
                UniqueIdentity::GetUniqueID(),
                std::format("{}_{}", name, meshCount++),
                vertexData.data(), static_cast<std::uint32_t>(vertexData.size()),
                indices.data(), static_cast<std::uint32_t>(indices.size()),
                layout, nullptr
            );
        }

        if (mesh)
        {
            MOTION_CORE_INFO("Created Sphere Mesh: '{}'", mesh->GetName());
            return mesh;
        }

        MOTION_CORE_ERROR("Failed to create Sphere Mesh: '{}'", name);
        return nullptr;
    }

    /**
     * @brief Creates a quad mesh with the specified dimensions.
     *
     * This static function generates a quad mesh centered at the origin, with the given width and height.
     * The quad is constructed with 4 vertices and 6 indices to form two triangles.
     *
     * @param isRegistered Indicates whether the mesh should be registered with the AssetManager.
     * @param name The name to assign to the mesh asset.
     * @param width The width of the quad along the X axis.
     * @param height The height of the quad along the Z axis.
     * @return std::shared_ptr<Mesh> A shared pointer to the created Mesh object, or nullptr if creation failed.
     */
    std::shared_ptr<Mesh> QuickMesh::CreateQuad(bool isRegistered, const std::string name, std::uint32_t width, std::uint32_t height)
    {
        std::vector<Vertex> vertices(4);
        float halfWidth = static_cast<float>(width) * 0.5f;
        float halfHeight = static_cast<float>(height) * 0.5f;

        // Positions (X, Y, Z), Normals (Y up), TexCoords, Tangents (X), Bitangents (-Z)
        vertices[0].Position = glm::vec3(-halfWidth, 0.0f, -halfHeight);
        vertices[0].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertices[0].TexCoord = glm::vec2(0.0f, 0.0f);
        vertices[0].Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        vertices[0].Bitangent = glm::vec3(0.0f, 0.0f, -1.0f);

        vertices[1].Position = glm::vec3(halfWidth, 0.0f, -halfHeight);
        vertices[1].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertices[1].TexCoord = glm::vec2(1.0f, 0.0f);
        vertices[1].Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        vertices[1].Bitangent = glm::vec3(0.0f, 0.0f, -1.0f);

        vertices[2].Position = glm::vec3(halfWidth, 0.0f, halfHeight);
        vertices[2].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertices[2].TexCoord = glm::vec2(1.0f, 1.0f);
        vertices[2].Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        vertices[2].Bitangent = glm::vec3(0.0f, 0.0f, -1.0f);

        vertices[3].Position = glm::vec3(-halfWidth, 0.0f, halfHeight);
        vertices[3].Normal = glm::vec3(0.0f, 1.0f, 0.0f);
        vertices[3].TexCoord = glm::vec2(0.0f, 1.0f);
        vertices[3].Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        vertices[3].Bitangent = glm::vec3(0.0f, 0.0f, -1.0f);

        std::vector<std::uint32_t> indices = { 0, 1, 2, 2, 3, 0 };

        // Prepare buffer data
        std::vector<float> vertexData;
        for (const auto& v : vertices) {
            vertexData.push_back(v.Position.x);
            vertexData.push_back(v.Position.y);
            vertexData.push_back(v.Position.z);
            vertexData.push_back(v.Normal.x);
            vertexData.push_back(v.Normal.y);
            vertexData.push_back(v.Normal.z);
            vertexData.push_back(v.TexCoord.x);
            vertexData.push_back(v.TexCoord.y);
            vertexData.push_back(v.Tangent.x);
            vertexData.push_back(v.Tangent.y);
            vertexData.push_back(v.Tangent.z);
            vertexData.push_back(v.Bitangent.x);
            vertexData.push_back(v.Bitangent.y);
            vertexData.push_back(v.Bitangent.z);
        }

        BufferLayout layout({
            { UniformCache::VertexAttri_Position, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Position) },
            { UniformCache::VertexAttri_TexCoords, BufferComponents::UV, BufferStride::F2, false, offsetof(Vertex, TexCoord) },
            { UniformCache::VertexAttri_Normals, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Normal) },
            { UniformCache::VertexAttri_Tangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Tangent) },
            { UniformCache::VertexAttri_Bitangents, BufferComponents::XYZ, BufferStride::F3, false, offsetof(Vertex, Bitangent) }
            });

        static std::uint32_t meshCount = 0;
        std::shared_ptr<Mesh> mesh = nullptr;

        if (isRegistered)
        {
            mesh = AssetManager::GetInstance().Create<Mesh>(
                std::format("{}_{}", name, meshCount++),
                vertexData.data(), static_cast<std::uint32_t>(vertexData.size()),
                indices.data(), static_cast<std::uint32_t>(indices.size()),
                layout, nullptr
            );
        }
        else
        {
            mesh = std::make_shared<Mesh>(
                UniqueIdentity::GetUniqueID(),
                std::format("{}_{}", name, meshCount++),
                vertexData.data(), static_cast<std::uint32_t>(vertexData.size()),
                indices.data(), static_cast<std::uint32_t>(indices.size()),
                layout, nullptr
            );
        }

        if (mesh)
        {
            MOTION_CORE_INFO("Created Quad Mesh: '{}'", mesh->GetName());
            return mesh;
        }

        MOTION_CORE_ERROR("Failed to create Quad Mesh: '{}'", name);
        return nullptr;
    }
}

