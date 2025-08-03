#include "CorePCH.hpp"
#include "GL_Arrays.hpp"

namespace Motion
{
    /**
     * @brief Constructs a new OpenGL Vertex Array Object (VAO) and generates a unique ID for it.
     * This constructor initializes the VAO by generating a new ID and binding it.
     * It is essential to call this constructor before using the VAO for rendering operations.
     *
     * @note The VAO must be bound before any vertex attributes are specified.
     * @details The OpenGL function `glGenVertexArrays` is used to create the VAO, and `glBindVertexArray` binds it for subsequent operations.
     * @see glGenVertexArrays, glBindVertexArray
     */
    GL_VertexArray::GL_VertexArray()
    {
        glGenVertexArrays(1, &m_RendererID);
        glBindVertexArray(m_RendererID);
    }

    /**
     * @brief Destroys the OpenGL Vertex Array Object (VAO).
     * This destructor cleans up the resources associated with the VAO by deleting it from OpenGL.
     * It is called automatically when the GL_VertexArray object goes out of scope or is deleted.
     *
     * @note Ensure that the VAO is unbound before deletion to avoid potential issues.
     * @details The OpenGL function `glDeleteVertexArrays` is used to delete the VAO.
     * @see glDeleteVertexArrays
     */
    GL_VertexArray::~GL_VertexArray()
    {
        glDeleteVertexArrays(1, &m_RendererID);
    }

    /**
     * @brief Binds the OpenGL Vertex Array Object (VAO) for use in rendering.
     * This function makes the VAO active, allowing subsequent vertex attribute calls to affect this VAO.
     * It is essential to call this function before rendering operations that use this VAO.
     *
     * @note The VAO must be bound before any vertex attributes are specified.
     * @details The OpenGL function `glBindVertexArray` is used to bind the VAO.
     * @see glBindVertexArray
     */
    void GL_VertexArray::Bind() const
    {
        glBindVertexArray(m_RendererID);
    }

    /**
     * @brief Unbinds the OpenGL Vertex Array Object (VAO).
     * This function makes no VAO active, which is useful for preventing accidental modifications to the VAO state.
     * It is typically called when you are done with the VAO or before binding another VAO.
     *
     * @note Unbinding the VAO does not delete it; it simply makes it inactive.
     * @details The OpenGL function `glBindVertexArray` is used with a parameter of 0 to unbind the current VAO.
     * @see glBindVertexArray
     */
    void GL_VertexArray::Unbind() const
    {
        glBindVertexArray(0);
    }

    /**
     * @brief Retrieves the unique ID of the OpenGL Vertex Array Object (VAO).
     * This function returns the ID assigned to the VAO, which can be used for debugging or logging purposes.
     *
     * @return RendererID The unique identifier for the VAO.
     * @details The ID is generated when the VAO is created and remains constant throughout its lifetime.
     */
    RendererID GL_VertexArray::GetID() const
    {
        return m_RendererID;
    }

    /**
     * @brief Adds a vertex buffer to the OpenGL Vertex Array Object (VAO).
     * This function binds the VAO and the provided vertex buffer, enabling the VAO to use the vertex attributes defined in the buffer.
     * It also sets up the vertex attribute pointers based on the buffer's layout.
     *
     * @param vtxBuffer The vertex buffer to be added to the VAO.
     * @note The vertex buffer must have a valid layout defined before calling this function.
     * @details The OpenGL functions `glBindVertexArray`, `glEnableVertexAttribArray`, and `glVertexAttribPointer` are used to set up the vertex attributes.
     * @see glBindVertexArray, glEnableVertexAttribArray, glVertexAttribPointer
     */
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

    /**
     * @brief Adds an index buffer to the OpenGL Vertex Array Object (VAO).
     * This function binds the VAO and the provided index buffer, allowing the VAO to use the indices defined in the buffer for indexed rendering.
     *
     * @param idxBuffer The index buffer to be added to the VAO.
     * @note The index buffer must be valid and contain indices before calling this function.
     * @details The OpenGL functions `glBindVertexArray` and `glBindBuffer` are used to bind the index buffer to the VAO.
     * @see glBindVertexArray, glBindBuffer
     */
    void GL_VertexArray::EmplaceIndexBuffer(const std::shared_ptr<IElementBuffer>& idxBuffer)
    {
        glBindVertexArray(m_RendererID);
        idxBuffer->Bind();
        m_IndexBuffer = idxBuffer;
    }

    /**
     * @brief Retrieves the list of vertex buffers associated with the OpenGL Vertex Array Object (VAO).
     * This function returns a reference to the vector containing all vertex buffers that have been added to the VAO.
     *
     * @return std::vector<std::shared_ptr<IVertexBuffer>>& A reference to the vector of vertex buffers.
     * @details The vertex buffers can be used for rendering operations after being added to the VAO.
     */
    std::vector<std::shared_ptr<IVertexBuffer>>& GL_VertexArray::GetVertexBuffer()
    {
        return m_VertexBuffers;
    }

    /**
     * @brief Retrieves the index buffer associated with the OpenGL Vertex Array Object (VAO).
     * This function returns a reference to the index buffer that has been added to the VAO, if any.
     *
     * @return std::shared_ptr<IElementBuffer>& A reference to the index buffer.
     * @details The index buffer can be used for indexed rendering operations.
     */
    std::shared_ptr<IElementBuffer>& GL_VertexArray::GetElementBuffer()
    {
        return m_IndexBuffer;
    }

    /**
     * @brief Creates a new instance of the OpenGL Vertex Array Object (VAO).
     * This static function constructs a new GL_VertexArray object and returns it as a shared pointer.
     * It is typically used to create a VAO for rendering operations.
     *
     * @return std::shared_ptr<GL_VertexArray> A shared pointer to the newly created GL_VertexArray object.
     */
    std::shared_ptr<GL_VertexArray> GL_VertexArray::Create()
    {
        return std::make_shared<GL_VertexArray>();
    }
}
