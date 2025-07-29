#include "CorePCH.hpp"

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
     * @param uuid The unique identifier for the mesh.
     * @param name The name of the mesh.
     * @param vertices Pointer to the array of vertex data.
     * @param verticesSize The size (in bytes) of the vertex data array.
     * @param indices Pointer to the array of index data.
     * @param indicesCount The number of indices in the index array.
     * @param layout The layout describing the structure of the vertex buffer.
     * @param parentModel Shared pointer to the parent StaticMesh object.
     */
    Mesh::Mesh(UUID uuid, const std::string& name, Vertex* vertices, std::uint32_t verticesSize, std::uint32_t* indices, std::uint32_t indicesCount,
        const BufferLayout& layout, const std::shared_ptr<StaticMesh>& parentModel)
        : AssetBase<IAsset>(uuid, name, AssetType::Mesh, "Undefined")
        , m_ParentModel(parentModel)
        , m_IndicesCount(indicesCount)
    {
        m_VertexBuffer = BufferFactory::CreateVertexBuffer(vertices, verticesSize);
        m_ElementBuffer = BufferFactory::CreateElementBuffer(indices, indicesCount);
        m_VertexBuffer->SetLayout(layout);

        m_VertexArray = ArrayFactory::CreateVertexArray();
        m_VertexArray->EmplaceVertexBuffer(m_VertexBuffer);
        m_VertexArray->EmplaceIndexBuffer(m_ElementBuffer);

        if (m_VertexBuffer && m_ElementBuffer && m_VertexArray)
        {
            MOTION_CORE_INFO("Mesh '{}' created successfully with {} vertices and {} indices.", name, verticesSize / sizeof(float), indicesCount);
            AssetInfo.IsInitialized = true;
        }
        else
        {
            MOTION_ASSERT(false, "Failed to create Mesh '{}': VertexBuffer, ElementBuffer, or VertexArray is null.", name);
            AssetInfo.IsInitialized = false;
        }
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
}

