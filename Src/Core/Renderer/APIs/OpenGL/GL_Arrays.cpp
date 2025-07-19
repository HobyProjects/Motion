#include "CorePCH.hpp"
#include "GL_Arrays.hpp"

namespace Motion::Core
{
    GL_VertexArray::GL_VertexArray()
    {
        glGenVertexArrays(1, &m_RendererID);
        glBindVertexArray(m_RendererID);
    }

    GL_VertexArray::~GL_VertexArray()
    {
        glDeleteVertexArrays(1, &m_RendererID);
    }

    void GL_VertexArray::Bind() const
    {
        glBindVertexArray(m_RendererID);
    }

    void GL_VertexArray::Unbind() const
    {
        glBindVertexArray(0);
    }

    RendererID GL_VertexArray::GetID() const
    {
        return m_RendererID;
    }

    void GL_VertexArray::EmplaceVertexBuffer(const std::shared_ptr<IVertexBuffer>& vtxBuffer)
    {
        glBindVertexArray(m_RendererID);
        vtxBuffer->Bind();
        const auto& layout = vtxBuffer->GetLayout();
        const auto& elements = layout.GetElements();
        uint32_t layoutIndex{ 0 };

        for (const auto& element : elements)
        {
            glEnableVertexAttribArray(layoutIndex);
            glVertexAttribPointer(
                layoutIndex++,
                static_cast<GLint>(element.Components),
                GL_FLOAT,
                element.Normalized ? GL_TRUE : GL_FALSE,
                static_cast<GLsizei>(element.Stride),
                reinterpret_cast<const void*>(static_cast<uintptr_t>(element.Offset))
            );
        }

        m_VertexBuffers.push_back(vtxBuffer);
    }

    void GL_VertexArray::EmplaceIndexBuffer(const std::shared_ptr<IElementBuffer>& idxBuffer)
    {
        glBindVertexArray(m_RendererID);
        idxBuffer->Bind();
        m_IndexBuffer = idxBuffer;
    }

    std::vector<std::shared_ptr<IVertexBuffer>>& GL_VertexArray::GetVertexBuffer()
    {
        return m_VertexBuffers;
    }

    std::shared_ptr<IElementBuffer>& GL_VertexArray::GetElementBuffer()
    {
        return m_IndexBuffer;
    }

    std::shared_ptr<GL_VertexArray> GL_CreateVertexArray()
    {
        return std::make_shared<GL_VertexArray>();
    }
}
