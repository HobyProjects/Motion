#pragma once

#include "UUID.hpp"
#include "Buffers.hpp"
#include "Arrays.hpp"
#include "Shaders.hpp"
#include "Material.hpp"

namespace Motion
{
    class Model; 

    class Mesh
    {
        public:
            explicit Mesh(const Vertex* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout);
            explicit Mesh(const float* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout);
            ~Mesh() = default;

            void Bind() const noexcept;
            void Unbind() const noexcept;
            void Render();

            [[nodiscard]] std::int32_t GetIndicesCount() const noexcept;
            [[nodiscard]] RendererID GetID() const noexcept { return m_VertexArray->GetID(); }

        public:
            [[nodiscard]] static std::shared_ptr<Mesh> Create(const Vertex* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout);
            [[nodiscard]] static std::shared_ptr<Mesh> Create(const float* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout);

        public:
            std::uint32_t Index{ 0 };
            std::string Name{ "Unnamed Mesh" };
            std::vector<glm::vec3> Positions{};
            std::vector<glm::uint32_t> Faces{};

            glm::vec3 MIN{};
            glm::vec3 MAX{};

        private:
            std::shared_ptr<IVertexBuffer>  m_VertexBuffer{ nullptr };
            std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
            std::shared_ptr<IVertexArray>   m_VertexArray{ nullptr };
            std::uint32_t                   m_IndicesCount{ 0 };

    };
}