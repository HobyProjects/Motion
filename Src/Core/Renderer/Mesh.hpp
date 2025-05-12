#pragma once

#include "Buffers.hpp"
#include "Texture.hpp"
#include "Arrays.hpp"

namespace Motion::Core
{
    struct VertexStructure
    {
        glm::vec3 Position;
        glm::vec2 TexCoords;
        glm::vec3 Normals;
    };

    class Mesh
    {
        public:
            Mesh() = default;
            Mesh(float* vertices, uint32_t verticeSize, uint32_t* indices, uint32_t indicesCount, const BufferLayout& layout);
            ~Mesh() = default;

            uint32_t GetIndicesCount() const { return m_IndicesCount; }
            void Create(float* vertices, uint32_t verticeSize, uint32_t* indices, uint32_t indicesCount, const BufferLayout& layout);
            void Render();

        private:
            std::shared_ptr<IVertexBuffer> m_VertexBuffer{ nullptr };
            std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
            std::shared_ptr<IVertexArray> m_VertexArray{ nullptr };
            uint32_t m_IndicesCount{ 0 };
    };
}