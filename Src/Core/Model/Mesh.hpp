#pragma once

#include "Buffers.hpp"
#include "Arrays.hpp"
#include "Shaders.hpp"
#include "Material.hpp"

namespace Motion::Core
{
    class Mesh
    {
        public:
            struct Vertex
            {
                glm::vec3 Position{0.0f, 0.0f, 0.0f};
                glm::vec2 TexCoord{0.0f, 0.0f};
                glm::vec3 Normal{0.0f, 0.0f, 0.0f};
                glm::vec3 Tangent{0.0f, 0.0f, 0.0f};
                glm::vec3 Bitangent{0.0f, 0.0f, 0.0f};
            };

        public:
            explicit Mesh(float* vertices, uint32_t verticeSize, uint32_t* indices, uint32_t indicesCount, const BufferLayout& layout);
            ~Mesh() = default;

            void Bind() const { m_VertexArray->Bind(); }
            void Unbind() const { m_VertexArray->Unbind(); }
            uint32_t GetIndicesCount() const { return m_IndicesCount; }

        private:

        public:
            static std::shared_ptr<Mesh> CreatePlane(float width, float height, uint32_t widthSegments = 1, uint32_t heightSegments = 1);
            static std::shared_ptr<Mesh> CreateCube(float width, float height, float depth);
            static std::shared_ptr<Mesh> CreateSphere(uint32_t sectorCount, uint32_t stackCount);
            static std::shared_ptr<Mesh> CreateQuad(uint32_t width, uint32_t height);
        

        private:
            std::shared_ptr<IVertexBuffer> m_VertexBuffer{ nullptr };
            std::shared_ptr<IVertexBuffer> m_TangentBuffer{ nullptr };
            std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
            std::shared_ptr<IVertexArray> m_VertexArray{ nullptr };
            uint32_t m_IndicesCount{ 0 };
    };
}