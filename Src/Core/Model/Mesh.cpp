#include "CorePCH.hpp"
#include "Mesh.hpp"

namespace Motion
{
    Mesh::Mesh(const Vertex* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout)
    {
        m_VertexBuffer = IVertexBuffer::Create(vertices, verticesSize);
        m_ElementBuffer = IElementBuffer::Create(indices, indicesCount);
        m_VertexBuffer->SetLayout(layout);

        m_VertexArray = IVertexArray::Create();
        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);
        m_IndicesCount = indicesCount;
    }

    Mesh::Mesh(const float* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout)
    {
        m_VertexBuffer = IVertexBuffer::Create(vertices, verticesSize);
        m_ElementBuffer = IElementBuffer::Create(indices, indicesCount);
        m_VertexBuffer->SetLayout(layout);

        m_VertexArray = IVertexArray::Create();
        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);
        m_IndicesCount = indicesCount;
    }

    void Mesh::Bind() const noexcept
    {
        if (m_VertexArray)
            m_VertexArray->Bind();
    }

    void Mesh::Unbind() const noexcept
    {
        if (m_VertexArray)
            m_VertexArray->Unbind();
    }

    void Mesh::Render()
    {
        Renderer::DrawIndexed(m_IndicesCount);
    }

    std::int32_t Mesh::GetIndicesCount() const noexcept
    {
        return m_IndicesCount;
    }

    std::shared_ptr<Mesh> Mesh::Create(const Vertex* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout)
    {
        return std::make_shared<Mesh>(vertices, verticesSize, indices, indicesCount, layout);
    }

    std::shared_ptr<Mesh> Mesh::Create(const float* vertices, std::uint32_t verticesSize, const std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout)
    {
        return std::make_shared<Mesh>(vertices, verticesSize, indices, indicesCount, layout);
    }
}

