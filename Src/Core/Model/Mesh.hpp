#pragma once

#include "UUID.hpp"
#include "Asset.hpp"
#include "Buffers.hpp"
#include "Arrays.hpp"
#include "Shaders.hpp"

namespace Motion
{
    class StaticMesh; // Forward declaration

    class Mesh : public AssetBase<IAsset>
    {
    public:
        Mesh() = default;
        Mesh(UUID uuid, const std::string& name, Vertex* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel);
        ~Mesh() = default;

        void Bind() const noexcept;
        void Unbind() const noexcept;
        void Render();
        std::int32_t GetIndicesCount() const noexcept;
        std::shared_ptr<StaticMesh> GetParentModel() const noexcept;

    private:
        std::shared_ptr<IVertexBuffer> m_VertexBuffer{ nullptr };
        std::shared_ptr<IElementBuffer> m_ElementBuffer{ nullptr };
        std::shared_ptr<IVertexArray> m_VertexArray{ nullptr };
        std::shared_ptr<StaticMesh> m_ParentModel{ nullptr };
        uint32_t m_IndicesCount{ 0 };

        friend class StaticMesh; // Allow StaticMesh to access private members
    };
}