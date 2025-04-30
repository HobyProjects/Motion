#include "CorePCH.hpp"

namespace Motion::Core
{
    GL_VertexBuffer::GL_VertexBuffer(uint32_t allocatorSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, allocatorSize, nullptr, GL_DYNAMIC_DRAW);
    }

    GL_VertexBuffer::GL_VertexBuffer(float* data, uint32_t dataSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, sizeof(data[0]) * dataSize, data, GL_STATIC_DRAW);
    }

    GL_VertexBuffer::~GL_VertexBuffer()
    {
        glDeleteBuffers(1, &m_VertexBufferID);
    }

    void GL_VertexBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
    }

    void GL_VertexBuffer::Unbind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void GL_VertexBuffer::SetData(const void* data, uint32_t size)
    {
        glNamedBufferSubData(m_VertexBufferID, 0, size, data);
    }

    void GL_VertexBuffer::SetLayout(const BufferLayout& layout)
    {
        m_Layout = layout;
    }

    GL_ElementBuffer::GL_ElementBuffer(uint32_t* data, uint32_t indicesCount) : m_Count(indicesCount)
    {
        glCreateBuffers(1, &m_ElementBufferID);
        glNamedBufferData(m_ElementBufferID, indicesCount * sizeof(uint32_t), data, GL_STATIC_DRAW);
        m_Count = indicesCount;
    }

    GL_ElementBuffer::~GL_ElementBuffer()
    {
        glDeleteBuffers(1, &m_ElementBufferID);
    }

    void GL_ElementBuffer::Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ElementBufferID);
    }
}