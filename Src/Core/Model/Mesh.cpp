#include "CorePCH.hpp"
#include "Mesh.hpp"

namespace Motion::Core
{
    static std::vector<std::shared_ptr<Mesh>> s_Meshes{};
    
    Mesh::Mesh(float* vertices, uint32_t verticeSize, uint32_t* indices, uint32_t indicesCount, const BufferLayout& layout)
    {
        m_VertexBuffer = BufferFactory::CreateVertexBuffer(vertices, verticeSize);
        m_VertexBuffer->SetLayout(layout);

        m_ElementBuffer = BufferFactory::CreateElementBuffer(indices, indicesCount);
        m_VertexArray = ArrayFactory::CreateVertexArray();

        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);
        m_IndicesCount = indicesCount;
    }



    std::shared_ptr<Mesh> Mesh::CreatePlane(float width, float height, uint32_t widthSegments, uint32_t heightSegments)
    {
        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        float halfWidth = width / 2.0f;
        float halfHeight = height / 2.0f;

        float segmentWidth = width / widthSegments;
        float segmentHeight = height / heightSegments;

        for (uint32_t i = 0; i <= heightSegments; ++i) {
            float y = halfHeight - i * segmentHeight;

            for (uint32_t j = 0; j <= widthSegments; ++j) {
                float x = -halfWidth + j * segmentWidth;

                // Position
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(0.0f);
                // Normal
                vertices.push_back(0.0f);
                vertices.push_back(0.0f);
                vertices.push_back(1.0f);
                // TexCoord
                vertices.push_back((float)j / widthSegments);
                vertices.push_back((float)i / heightSegments);
            }
        }

        for (uint32_t i = 0; i < heightSegments; ++i) {
            for (uint32_t j = 0; j < widthSegments; ++j) {
                uint32_t k1 = i * (widthSegments + 1) + j;
                uint32_t k2 = k1 + widthSegments + 1;

                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);

                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }

        BufferLayout layout = {
            { "a_Position", BufferComponents::XYZ, BufferStride::F3, false, 0 },
            { "a_Normal", BufferComponents::XYZ, BufferStride::F3, false, 12 },
            { "a_TexCoord", BufferComponents::UV, BufferStride::F2, false, 24 }
        };
        
        auto planeMesh = std::make_shared<Mesh>(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)), indices.data(), static_cast<uint32_t>(indices.size()), layout);
        s_Meshes.push_back(planeMesh);
        return planeMesh;
    }

    std::shared_ptr<Mesh> Mesh::CreateCube(float width, float height, float depth)
    {
        //[TODO] Implement CreateCube method
        MOTION_ASSERT(false, "Mesh::CreateCube is not implemented yet.");

        // Placeholder for cube mesh creation logic
        return nullptr;
    }

    std::shared_ptr<Mesh> Mesh::CreateSphere(uint32_t sectorCount, uint32_t stackCount)
    {
        std::vector<float> vertices;
        std::vector<uint32_t> indices;

        const float PI = 3.14159265359f;

        for (uint32_t i = 0; i <= stackCount; ++i) {
            float stackAngle = PI / 2 - i * PI / stackCount; // from pi/2 to -pi/2
            float xy = cosf(stackAngle);
            float z = sinf(stackAngle);

            for (uint32_t j = 0; j <= sectorCount; ++j) {
                float sectorAngle = j * 2 * PI / sectorCount;

                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);

                float u = (float)j / sectorCount;
                float v = (float)i / stackCount;

                // Position
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                // Normal
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);
                // TexCoord
                vertices.push_back(u);
                vertices.push_back(v);
            }
        }

        for (uint32_t i = 0; i < stackCount; ++i) {
            uint32_t k1 = i * (sectorCount + 1);
            uint32_t k2 = k1 + sectorCount + 1;

            for (uint32_t j = 0; j < sectorCount; ++j, ++k1, ++k2) {
                if (i != 0) {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                if (i != (stackCount - 1)) {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }


        BufferLayout layout = {
            { "a_Position", BufferComponents::XYZ, BufferStride::F3, false, 0 },
            { "a_Normal", BufferComponents::XYZ, BufferStride::F3, false, 12 },
            { "a_TexCoord", BufferComponents::UV, BufferStride::F2, false, 24 }
        };


        auto sphereMesh = std::make_shared<Mesh>(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)), indices.data(), static_cast<uint32_t>(indices.size()), layout);
        s_Meshes.push_back(sphereMesh);
        return sphereMesh;
    }
    std::shared_ptr<Mesh> Mesh::CreateQuad(uint32_t width, uint32_t height)
    {
        std::vector<float> vertices = {
            // Positions          // Normals           // TexCoords
            -1.0f,  1.0f, 0.0f,   0.0f, 0.0f, -1.0f,   0.0f, 1.0f,
             1.0f,  1.0f, 0.0f,   0.0f, 0.0f, -1.0f,   1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,   0.0f, 0.0f, -1.0f,   1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, -1.0f,   0.0f, 0.0f
        };

        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0
        };

        BufferLayout layout = {
            { "a_Position", BufferComponents::XYZ, BufferStride::F3, false, 0 },
            { "a_Normal", BufferComponents::XYZ, BufferStride::F3, false, 12 },
            { "a_TexCoord", BufferComponents::UV, BufferStride::F2, false, 24 }
        };

        auto quadMesh = std::make_shared<Mesh>(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(float)), indices.data(), static_cast<uint32_t>(indices.size()), layout);
        s_Meshes.push_back(quadMesh);
        return quadMesh;
    }
}

