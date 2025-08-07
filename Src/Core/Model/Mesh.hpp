#pragma once

#include "UUID.hpp"
#include "Asset.hpp"
#include "Buffers.hpp"
#include "Arrays.hpp"
#include "Shaders.hpp"
#include "Material.hpp"

namespace Motion
{
    class StaticMesh; // Forward declaration

    class Mesh
    {
    public:
        Mesh() = default;
        Mesh(Vertex* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel);
        Mesh(float* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel);
        ~Mesh() = default;

        void Bind() const noexcept;
        void Unbind() const noexcept;
        void Render();

        void SetMaterial(const std::shared_ptr<PhysicalBasedMaterialInstance>& material) noexcept;
        void SetMaterial(const std::shared_ptr<StandardMaterialInstance>& material) noexcept;

        [[nodiscard]] std::int32_t GetIndicesCount() const noexcept;
        [[nodiscard]] std::shared_ptr<StaticMesh> GetParentModel() const noexcept;
        [[nodiscard]] RendererID GetID() const noexcept { return m_VertexArray->GetID(); }

    public:
        [[nodiscard]] static std::shared_ptr<Mesh> Create(Vertex* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel);
        [[nodiscard]] static std::shared_ptr<Mesh> Create(float* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel);

    public:
        std::uint32_t Index{ 0 };
        std::string Name{ "Unnamed Mesh" };
        std::shared_ptr<PhysicalBasedMaterialInstance> PhysicalBasedMaterials{ nullptr };
        std::shared_ptr<StandardMaterialInstance> StandardMaterials{ nullptr };

    private:
        std::shared_ptr<IVertexBuffer> m_VertexBuffer{ nullptr };
        std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
        std::shared_ptr<IVertexArray> m_VertexArray{ nullptr };
        std::shared_ptr<StaticMesh> m_ParentModel{ nullptr };
        uint32_t m_IndicesCount{ 0 };

        friend class StaticMesh; // Allow StaticMesh to access private members
    };
}