#pragma once

#include "UUID.hpp"
#include "Asset.hpp"
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
            explicit Mesh(const Vertex* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<Model>& parentModel);
            explicit Mesh(const float* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<Model>& parentModel);
            ~Mesh() = default;

            void Bind() const noexcept;
            void Unbind() const noexcept;
            void Render();

            [[nodiscard]] std::int32_t GetIndicesCount() const noexcept;
            [[nodiscard]] std::shared_ptr<Model> GetParentModel() const noexcept;
            [[nodiscard]] RendererID GetID() const noexcept { return m_VertexArray->GetID(); }
            [[nodiscard]] const glm::vec3& GetMinBounds() const noexcept { return MIN; }
            [[nodiscard]] const glm::vec3& GetMaxBounds() const noexcept { return MAX; }

        public:
            [[nodiscard]] static std::shared_ptr<Mesh> Create(const Vertex* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<Model>& parentModel);
            [[nodiscard]] static std::shared_ptr<Mesh> Create(const float* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<Model>& parentModel);

        public:
            std::uint32_t Index{ 0 };
            std::string Name{ "Unnamed Mesh" };
            std::shared_ptr<Material> Materials{ nullptr };
            glm::vec3 MIN{ FLT_MAX, FLT_MAX, FLT_MAX };
            glm::vec3 MAX{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

        private:
            std::shared_ptr<IVertexBuffer>  m_VertexBuffer{ nullptr };
            std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
            std::shared_ptr<IVertexArray>   m_VertexArray{ nullptr };
            std::shared_ptr<Model>          m_ParentModel{ nullptr };
            std::uint32_t                   m_IndicesCount{ 0 };
            
            friend class Model; // Allow Model to access private members
    };
}