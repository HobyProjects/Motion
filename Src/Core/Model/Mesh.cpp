#include "CorePCH.hpp"
#include "Mesh.hpp"

namespace Motion
{

    /**
     * @brief Constructs a Mesh object with the specified UUID and parameters.
     *
     * Initializes the mesh with vertex and index data, sets up the buffer layout,
     * and associates the mesh with a parent model. This constructor creates the
     * necessary vertex and element buffers, assigns the buffer layout, and
     * configures the vertex array object for rendering.
     *
     * @param vertices Pointer to the array of vertex data.
     * @param verticesSize The size (in bytes) of the vertex data array.
     * @param indices Pointer to the array of index data.
     * @param indicesCount The number of indices in the index array.
     * @param layout The layout describing the structure of the vertex buffer.
     * @param parentModel Shared pointer to the parent StaticMesh object.
     */
    Mesh::Mesh(Vertex* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel)
    {
        m_VertexBuffer = IVertexBuffer::Create(vertices, verticesSize);
        m_ElementBuffer = IElementBuffer::Create(indices, indicesCount);
        m_VertexBuffer->SetLayout(layout);

        m_VertexArray = IVertexArray::Create();
        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);

        m_ParentModel = parentModel;
        m_IndicesCount = indicesCount;
    }


    /**
     * @brief Constructs a Mesh object with the specified UUID and parameters using float vertices.
     *
     * Initializes the mesh with vertex and index data, sets up the buffer layout,
     * and associates the mesh with a parent model. This constructor creates the
     * necessary vertex and element buffers, assigns the buffer layout, and
     * configures the vertex array object for rendering.
     *
     * @param vertices Pointer to the array of vertex data as floats.
     * @param verticesSize The size (in bytes) of the vertex data array.
     * @param indices Pointer to the array of index data.
     * @param indicesCount The number of indices in the index array.
     * @param layout The layout describing the structure of the vertex buffer.
     * @param parentModel Shared pointer to the parent StaticMesh object.
     */
    Mesh::Mesh(float* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel)
    {
        m_VertexBuffer = IVertexBuffer::Create(vertices, verticesSize);
        m_ElementBuffer = IElementBuffer::Create(indices, indicesCount);
        m_VertexBuffer->SetLayout(layout);

        m_VertexArray = IVertexArray::Create();
        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);

        m_ParentModel = parentModel;
        m_IndicesCount = indicesCount;
    }

    /**
     * @brief Binds the vertex array object for rendering.
     *
     * This function binds the vertex array associated with the mesh, making it
     * ready for rendering operations. It ensures that the correct vertex and
     * index buffers are active for subsequent draw calls.
     */
    void Mesh::Bind() const noexcept
    {
        if (m_VertexArray)
            m_VertexArray->Bind();
    }

    /**
     * @brief Unbinds the vertex array object.
     *
     * This function unbinds the vertex array associated with the mesh, effectively
     * disabling it for rendering operations. It is typically called after rendering
     * is complete to reset the OpenGL state.
     */
    void Mesh::Unbind() const noexcept
    {
        if (m_VertexArray)
            m_VertexArray->Unbind();
    }

    /**
     * @brief Renders the mesh using the renderer instance.
     *
     * This function binds the mesh, issues a draw call to the renderer
     * using the number of indices in the mesh, and then unbinds the mesh.
     * It encapsulates the rendering process for this mesh object.
     */
    void Mesh::Render()
    {
        Renderer::DrawIndexed(m_IndicesCount);
    }

    /**
     * @brief Sets the material for the mesh.
     *
     * Assigns a new physically based material instance to the mesh if the provided material is valid.
     * Updates the material ID (MID) and transfers ownership of the material instance.
     *
     * @param material A shared pointer to the PhysicalBasedMaterialInstance to be set for the mesh.
     *                 If nullptr, the material is not updated.
     */
    void Mesh::SetMaterial(const std::shared_ptr<PhysicalBasedMaterialInstance>& material) noexcept
    {
        if (material)
        {
            PhysicalBasedMaterials = std::move(material);
        }
    }

    /**
     * @brief Sets the material instance for this mesh.
     *
     * Assigns the provided StandardMaterialInstance to the mesh, updating the material ID (MID)
     * accordingly. If the given material is valid (non-null), it is moved into the mesh's
     * StandardMaterials member.
     *
     * @param material A shared pointer to the StandardMaterialInstance to be assigned.
     *                 If nullptr, the mesh's material remains unchanged.
     */
    void Mesh::SetMaterial(const std::shared_ptr<StandardMaterialInstance>& material) noexcept
    {
        if (material)
        {
            StandardMaterials = std::move(material);
        }
    }

    /**
     * @brief Returns the number of indices in the mesh.
     *
     * This function retrieves the count of indices used for rendering the mesh,
     * which is essential for determining how many vertices to draw during rendering.
     *
     * @return The number of indices in the mesh.
     */
    std::int32_t Mesh::GetIndicesCount() const noexcept
    {
        return m_IndicesCount;
    }

    /**
     * @brief Returns the parent model associated with the mesh.
     *
     * This function retrieves the shared pointer to the parent StaticMesh object that
     * owns this mesh, allowing access to model-level properties and methods.
     *
     * @return Shared pointer to the parent StaticMesh object.
     */
    std::shared_ptr<StaticMesh> Mesh::GetParentModel() const noexcept
    {
        return m_ParentModel;
    }

    /**
     * @brief Creates a shared pointer to a Mesh object with the specified parameters.
     *
     * This static method constructs a new Mesh instance using the provided vertex and index data,
     * buffer layout, and parent model, and returns it as a shared pointer.
     *
     * @param vertices Pointer to the array of vertex data.
     * @param verticesSize The size (in bytes) of the vertex data array.
     * @param indices Pointer to the array of index data.
     * @param indicesCount The number of indices in the index array.
     * @param layout The layout describing the structure of the vertex buffer.
     * @param parentModel Shared pointer to the parent StaticMesh object.
     * @return A shared pointer to the newly created Mesh object.
     */
    std::shared_ptr<Mesh> Mesh::Create(Vertex* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel)
    {
        return std::make_shared<Mesh>(vertices, verticesSize, indices, indicesCount, layout, parentModel);
    }

    /**
     * @brief Creates a shared pointer to a Mesh object with float vertices.
     *
     * This static method constructs a new Mesh instance using the provided float vertex and index data,
     * buffer layout, and parent model, and returns it as a shared pointer.
     *
     * @param vertices Pointer to the array of vertex data as floats.
     * @param verticesSize The size (in bytes) of the vertex data array.
     * @param indices Pointer to the array of index data.
     * @param indicesCount The number of indices in the index array.
     * @param layout The layout describing the structure of the vertex buffer.
     * @param parentModel Shared pointer to the parent StaticMesh object.
     * @return A shared pointer to the newly created Mesh object.
     */
    std::shared_ptr<Mesh> Mesh::Create(float* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount, const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel)
    {
        return std::make_shared<Mesh>(vertices, verticesSize, indices, indicesCount, layout, parentModel);
    }
}

