#include "CorePCH.hpp"

namespace Motion::Core
{
    Mesh::Mesh(float* vertices, uint32_t verticeSize, uint32_t* indices, uint32_t indicesCount, const BufferLayout& layout)
    {
        m_VertexBuffer = BuffersBuilder::CreateVertexBuffer(vertices, verticeSize);
        m_VertexBuffer->SetLayout(layout);

        m_ElementBuffer = BuffersBuilder::CreateElementBuffer(indices, indicesCount);
        m_VertexArray = ArrayBuilder::CreateVertexArray();

        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);
        m_IndicesCount = indicesCount;
    }

    void Mesh::Create(float* vertices, uint32_t verticeSize, uint32_t* indices, uint32_t indicesCount, const BufferLayout& layout)
    {
        m_VertexBuffer = BuffersBuilder::CreateVertexBuffer(vertices, verticeSize);
        m_VertexBuffer->SetLayout(layout);

        m_ElementBuffer = BuffersBuilder::CreateElementBuffer(indices, indicesCount);
        m_VertexArray = ArrayBuilder::CreateVertexArray();

        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);
        m_IndicesCount = indicesCount;
    }

    void Mesh::Render()
    {
        m_VertexArray->Bind();
        Renderer::Draw(m_IndicesCount);
        m_VertexArray->Unbind();
    }
}