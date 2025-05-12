#pragma once

#include "Buffers.hpp"
#include "Arrays.hpp"
#include "Shaders.hpp"
#include "Material.hpp"

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
            explicit Mesh(float* vertices, uint32_t verticeSize, uint32_t* indices, uint32_t indicesCount, const BufferLayout& layout);
            ~Mesh() = default;

            void Bind() const { m_VertexArray->Bind(); }
            void Unbind() const { m_VertexArray->Unbind(); }
            uint32_t GetIndicesCount() const { return m_IndicesCount; }
            void SetMaterial(const std::shared_ptr<Material>& material) { m_Material = material; }
            std::shared_ptr<Material> GetMaterial() const { return m_Material; }

        private:
            std::shared_ptr<IVertexBuffer> m_VertexBuffer{ nullptr };
            std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
            std::shared_ptr<IVertexArray> m_VertexArray{ nullptr };
            std::shared_ptr<Material> m_Material{ nullptr };
            uint32_t m_IndicesCount{ 0 };
    };
}