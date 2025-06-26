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
            explicit Mesh(float* vertices, uint32_t verticeSize, uint32_t* indices, uint32_t indicesCount, const BufferLayout& layout);
            ~Mesh() = default;

            void Bind() const { m_VertexArray->Bind(); }
            void Unbind() const { m_VertexArray->Unbind(); }
            uint32_t GetIndicesCount() const { return m_IndicesCount; }
        
        public:
            struct Vertex
            {
                glm::vec3 Position{0.0f, 0.0f, 0.0f};
                glm::vec2 TexCoords{0.0f, 0.0f};
                glm::vec3 Normals{0.0f, 0.0f, 0.0f};
            };

        private:
            std::shared_ptr<IVertexBuffer> m_VertexBuffer{ nullptr };
            std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
            std::shared_ptr<IVertexArray> m_VertexArray{ nullptr };
            uint32_t m_IndicesCount{ 0 };
    };
}