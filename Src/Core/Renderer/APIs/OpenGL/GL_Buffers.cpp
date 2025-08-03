#include "CorePCH.hpp"
#include "Material.hpp"
#include "GL_Buffers.hpp"

namespace Motion
{
    /******************************************************************************************
     *                         GL_VertexBuffer Implementation                                 *
     *******************************************************************************************/

     /**
      * @brief Constructs a GL_VertexBuffer with a specified allocator size.
      *
      * This constructor creates an OpenGL vertex buffer object (VBO) and allocates memory for it.
      * The buffer is created with the specified size and is intended for dynamic drawing usage.
      *
      * @param allocatorSize The size in bytes to allocate for the vertex buffer.
      */
    GL_VertexBuffer::GL_VertexBuffer(std::int32_t allocatorSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, allocatorSize, nullptr, GL_DYNAMIC_DRAW);
    }

    /**
     * @brief Constructs a GL_VertexBuffer object and initializes an OpenGL vertex buffer with the provided data.
     *
     * This constructor creates a new OpenGL buffer object and uploads the given vertex data to the GPU.
     *
     * @param data Pointer to the array of vertex data to be stored in the buffer.
     * @param dataSize The number of elements in the data array.
     */
    GL_VertexBuffer::GL_VertexBuffer(Vertex* data, std::uint32_t dataSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, sizeof(Vertex) * dataSize, data, GL_STATIC_DRAW);
    }

    /**
     * @brief Constructs a GL_VertexBuffer object and initializes an OpenGL vertex buffer with the provided float data.
     *
     * This constructor creates a new OpenGL buffer object and uploads the given float data to the GPU.
     *
     * @param data Pointer to the array of float data to be stored in the buffer.
     * @param dataSize The number of elements in the data array.
     */
    GL_VertexBuffer::GL_VertexBuffer(float* data, std::uint32_t dataSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, sizeof(float) * dataSize, data, GL_STATIC_DRAW);
    }

    /**
     * @brief Destructor for the GL_VertexBuffer class.
     *
     * This destructor releases the OpenGL vertex buffer resource associated with this object
     * by calling glDeleteBuffers on the stored buffer ID. Ensures proper cleanup of GPU memory
     * when the vertex buffer object is destroyed.
     */
    GL_VertexBuffer::~GL_VertexBuffer()
    {
        glDeleteBuffers(1, &m_VertexBufferID);
    }

    /**
     * @brief Binds the vertex buffer to the OpenGL GL_ARRAY_BUFFER target.
     *
     * This method makes the vertex buffer active in the OpenGL context,
     * allowing subsequent vertex attribute operations to use this buffer.
     * It should be called before issuing draw calls or configuring vertex attributes.
     */
    void GL_VertexBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_VertexBufferID);
    }

    /**
     * @brief Unbinds the currently bound OpenGL vertex buffer.
     *
     * This method unbinds the vertex buffer object (VBO) from the GL_ARRAY_BUFFER target,
     * effectively resetting the binding to 0. This is useful to prevent accidental modifications
     * to the buffer and to ensure that subsequent OpenGL operations do not affect this buffer.
     */
    void GL_VertexBuffer::Unbind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    /**
     * @brief Updates the contents of the vertex buffer with new data.
     *
     * This function replaces the current data in the vertex buffer with the data provided.
     * It uses OpenGL's glNamedBufferSubData to update the buffer starting from offset 0.
     *
     * @param data Pointer to the source data to be copied into the buffer.
     * @param size The size, in bytes, of the data to be copied.
     */
    void GL_VertexBuffer::SetData(const void* data, std::int32_t size)
    {
        glNamedBufferSubData(m_VertexBufferID, 0, size, data);
    }

    /**
     * @brief Sets the layout of the vertex buffer.
     *
     * This function assigns a new buffer layout to the vertex buffer, which defines
     * the structure and organization of the vertex attributes within the buffer.
     *
     * @param layout The BufferLayout object specifying the arrangement of vertex attributes.
     */
    void GL_VertexBuffer::SetLayout(const BufferLayout& layout)
    {
        m_Layout = layout;
    }

    /**
     * @brief Returns the layout of the vertex buffer.
     *
     * This function retrieves the current buffer layout, which describes how vertex attributes
     * are organized within the vertex buffer. It is used to configure vertex attribute pointers
     * for rendering.
     *
     * @return const BufferLayout& The current buffer layout of the vertex buffer.
     */
    std::shared_ptr<GL_VertexBuffer> GL_VertexBuffer::Create(std::int32_t allocatorSize)
    {
        return std::make_shared<GL_VertexBuffer>(allocatorSize);
    }


    /**
     * @brief Creates a vertex buffer with the specified vertex data.
     *
     * This factory method creates and returns a shared pointer to an GL_VertexBuffer
     * implementation, initialized with the provided vertex data and size.
     *
     * @param data Pointer to the array of vertex data (of type Vertex).
     * @param dataSize The number of vertices in the data array.
     * @return std::shared_ptr<GL_VertexBuffer> A shared pointer to the created vertex buffer,
     *         or nullptr if the rendering API is not implemented or unknown.
     */
    std::shared_ptr<GL_VertexBuffer> GL_VertexBuffer::Create(Vertex* data, std::uint32_t dataSize)
    {
        return std::make_shared<GL_VertexBuffer>(data, dataSize);
    }


    /**
     * @brief Creates a vertex buffer with the specified float data.
     *
     * This factory method creates and returns a shared pointer to an GL_VertexBuffer
     * implementation, initialized with the provided float data and size.
     *
     * @param data Pointer to the array of float data.
     * @param dataSize The number of floats in the data array.
     * @return std::shared_ptr<GL_VertexBuffer> A shared pointer to the created vertex buffer,
     *         or nullptr if the rendering API is not implemented or unknown.
     */
    std::shared_ptr<GL_VertexBuffer> GL_VertexBuffer::Create(float* data, std::uint32_t dataSize)
    {
        return std::make_shared<GL_VertexBuffer>(data, dataSize);
    }


    /******************************************************************************************
     *                         GL_ElementBuffer Implementation                                *
     *******************************************************************************************/

     /**
      * @brief Constructs a GL_ElementBuffer with the given index data.
      *
      * This constructor creates an OpenGL element buffer object (EBO) and uploads the provided
      * index data to the GPU. The buffer is initialized with the specified number of indices.
      *
      * @param data Pointer to the array of index data (of type std::int32_t) to be uploaded.
      * @param indicesCount The number of indices in the data array.
      */
    GL_ElementBuffer::GL_ElementBuffer(std::uint32_t* data, std::uint32_t indicesCount) : m_Count(indicesCount)
    {
        glCreateBuffers(1, &m_ElementBufferID);
        glNamedBufferData(m_ElementBufferID, indicesCount * sizeof(std::uint32_t), data, GL_STATIC_DRAW);
        m_Count = indicesCount;
    }

    /**
     * @brief Destructor for the GL_ElementBuffer class.
     *
     * This destructor releases the OpenGL element buffer resource associated with this object
     * by calling glDeleteBuffers on the buffer ID. Ensures proper cleanup of GPU memory
     * when the GL_ElementBuffer instance is destroyed.
     */
    GL_ElementBuffer::~GL_ElementBuffer()
    {
        glDeleteBuffers(1, &m_ElementBufferID);
    }

    /**
     * @brief Binds the element buffer to the OpenGL context.
     *
     * This method binds the element buffer object represented by m_ElementBufferID
     * to the GL_ELEMENT_ARRAY_BUFFER target, making it the current element buffer
     * for subsequent OpenGL operations.
     */
    void GL_ElementBuffer::Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ElementBufferID);
    }

    /**
     * @brief Unbinds the currently bound OpenGL element array buffer.
     *
     * This method unbinds the element buffer object by binding the buffer target
     * GL_ELEMENT_ARRAY_BUFFER to 0. This is useful to prevent accidental modification
     * of the buffer or to ensure that no element buffer is currently bound.
     */
    void GL_ElementBuffer::Unbind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    /**
     * @brief Returns the ID of the element buffer.
     *
     * This function retrieves the OpenGL buffer ID associated with this element buffer,
     * which can be used for rendering operations or debugging purposes.
     *
     * @return BufferID The OpenGL buffer ID of the element buffer.
     */
    std::shared_ptr<GL_ElementBuffer> GL_ElementBuffer::Create(std::uint32_t* data, std::uint32_t indicesCount)
    {
        return std::make_shared<GL_ElementBuffer>(data, indicesCount);
    }

    /******************************************************************************************
     *                         GL_ShaderBuffer Implementation                                 *
     *******************************************************************************************/

     /**
      * @brief Constructs a GL_ShaderBuffer with the specified size and binding point.
      *
      * This constructor creates an OpenGL shader buffer object, allocates storage for it,
      * and assigns it to a specific binding point. The buffer is created with dynamic storage,
      * allowing for efficient updates.
      *
      * @param size The size in bytes to allocate for the shader buffer.
      * @param binding The binding point to which this buffer will be associated.
      */
    GL_ShaderBuffer::GL_ShaderBuffer(std::int32_t size, BindingPoint binding)
    {
        glCreateBuffers(1, &m_ShaderBufferID);
        glNamedBufferData(m_ShaderBufferID, size, nullptr, GL_DYNAMIC_DRAW);
        m_BindingPoint = binding;
    }

    /**
     * @brief Destructor for the GL_ShaderBuffer class.
     *
     * This destructor releases the OpenGL buffer resource associated with this shader buffer
     * by calling glDeleteBuffers on the buffer ID. Ensures proper cleanup of GPU resources
     * when the GL_ShaderBuffer object is destroyed.
     */
    GL_ShaderBuffer::~GL_ShaderBuffer()
    {
        glDeleteBuffers(1, &m_ShaderBufferID);
    }

    /**
     * @brief Binds the shader storage buffer to a specific binding point.
     *
     * This method binds the OpenGL shader storage buffer object, identified by
     * m_ShaderBufferID, to the binding point specified by m_BindingPoint using
     * glBindBufferBase. This allows the buffer to be accessed in shaders via the
     * corresponding binding point.
     *
     * @note This function should be called before dispatching compute shaders or
     *       drawing commands that require access to this buffer.
     */
    void GL_ShaderBuffer::Bind() const
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_BindingPoint, m_ShaderBufferID);
    }

    /**
     * @brief Unbinds the shader storage buffer from the binding point.
     *
     * This method unbinds the currently bound shader storage buffer by binding buffer 0
     * to binding point 0, effectively detaching any buffer previously bound to that point.
     * This is typically used to ensure that no shader storage buffer is currently bound,
     * which can help prevent unintended side effects in subsequent OpenGL operations.
     */
    void GL_ShaderBuffer::Unbind() const
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    }

    /**
     * @brief Updates a portion of the shader buffer with new matrix data.
     *
     * This function uploads a 4x4 matrix (glm::mat4) to the shader buffer object at the specified offset.
     *
     * @param offset The byte offset within the buffer where the data should be updated.
     * @param size The size in bytes of the data to update (should match sizeof(glm::mat4)).
     * @param data The matrix data to upload to the buffer.
     */
    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat4& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Updates a portion of the shader buffer with the provided 3x3 matrix data.
     *
     * This function uploads the contents of a glm::mat3 to the shader buffer object,
     * starting at the specified offset and writing the specified number of bytes.
     *
     * @param offset The byte offset within the buffer where the data should be written.
     * @param size The size in bytes of the data to write.
     * @param data The glm::mat3 matrix containing the data to upload.
     */
    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat3& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Sets a portion of the shader buffer's data with the provided glm::vec4 value.
     *
     * Updates the buffer object identified by m_ShaderBufferID starting at the specified offset,
     * writing 'size' bytes from the memory pointed to by the glm::vec4 'data'.
     *
     * @param offset The byte offset within the buffer where the data replacement should begin.
     * @param size The size in bytes of the data to be written.
     * @param data The glm::vec4 value to write into the buffer.
     */
    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec4& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Updates a portion of the shader buffer with new data.
     *
     * This function uploads a glm::vec3 value to the shader buffer starting at the specified offset.
     *
     * @param offset The byte offset within the buffer where the data upload should begin.
     * @param size The size in bytes of the data to upload (should match sizeof(glm::vec3)).
     * @param data The glm::vec3 data to be uploaded to the buffer.
     */
    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec3& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Updates a portion of the shader buffer with new data.
     *
     * This function uploads a glm::vec2 value to the shader buffer starting at the specified offset.
     *
     * @param offset The byte offset within the buffer where the data upload should begin.
     * @param size The size in bytes of the data to upload (should match sizeof(glm::vec2)).
     * @param data The glm::vec2 data to be uploaded to the buffer.
     */
    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec2& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Updates a portion of the shader buffer with a single float value.
     *
     * This function uploads a float value to the shader buffer starting at the specified offset.
     *
     * @param offset The byte offset within the buffer where the data upload should begin.
     * @param size The size in bytes of the data to upload (should be sizeof(float)).
     * @param data The float value to be uploaded to the buffer.
     */
    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, float data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, &data);
    }

    /**
     * @brief Sets raw buffer data for the shader buffer.
     *
     * This function uploads raw data to the shader buffer object, allowing for
     * arbitrary data to be stored in the buffer.
     *
     * @param size The size in bytes of the data to upload.
     * @param data Pointer to the raw data to be uploaded.
     */
    void GL_ShaderBuffer::SetRawBufferData(std::int32_t size, const void* data)
    {
        glNamedBufferSubData(m_ShaderBufferID, 0, size, data);
    }

    /**
     * @brief Creates a shared pointer to a GL_ShaderBuffer with the specified size and binding point.
     *
     * This factory method creates and returns a shared pointer to an GL_ShaderBuffer
     * implementation, initialized with the specified size and binding point.
     *
     * @param size The size in bytes to allocate for the shader buffer.
     * @param binding The binding point to which this buffer will be associated.
     * @return std::shared_ptr<GL_ShaderBuffer> A shared pointer to the created shader buffer.
     */
    std::shared_ptr<GL_ShaderBuffer> GL_ShaderBuffer::Create(std::int32_t size, BindingPoint binding)
    {
        return std::make_shared<GL_ShaderBuffer>(size, binding);
    }

    /******************************************************************************************
     *                         GL_UniformBuffer Implementation                                *
     *******************************************************************************************/

     /**
      * @brief Constructs a GL_UniformBuffer with the specified size and binding point.
      *
      * This constructor creates an OpenGL uniform buffer object (UBO) of the given size,
      * allocates storage for it, and assigns it to the specified binding point.
      *
      * @param size The size in bytes of the uniform buffer to allocate.
      * @param binding The binding point to which this uniform buffer will be bound.
      */
    GL_UniformBuffer::GL_UniformBuffer(std::int32_t size, BindingPoint binding)
    {
        glCreateBuffers(1, &m_UniformBufferID);
        glNamedBufferData(m_UniformBufferID, size, nullptr, GL_DYNAMIC_DRAW);
        m_BindingPoint = binding;
    }

    /**
     * @brief Destructor for the GL_UniformBuffer class.
     *
     * This destructor releases the OpenGL uniform buffer resource associated with this object
     * by calling glDeleteBuffers on the buffer ID. Ensures proper cleanup of GPU resources
     * when the GL_UniformBuffer instance is destroyed.
     */
    GL_UniformBuffer::~GL_UniformBuffer()
    {
        glDeleteBuffers(1, &m_UniformBufferID);
    }

    /**
     * @brief Binds the uniform buffer to the specified binding point.
     *
     * This function binds the OpenGL uniform buffer object, identified by
     * m_UniformBufferID, to the binding point specified by m_BindingPoint
     * using glBindBufferBase. This allows the shader programs to access
     * the uniform data stored in this buffer.
     *
     * @note This method should be called before rendering operations that
     * require access to the uniform buffer data.
     */
    void GL_UniformBuffer::Bind() const
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_UniformBufferID);
    }

    /**
     * @brief Unbinds the currently bound uniform buffer from the OpenGL uniform buffer binding point.
     *
     * This method resets the binding of the uniform buffer object by binding buffer 0 to binding point 0,
     * effectively unbinding any previously bound uniform buffer from the specified binding point.
     */
    void GL_UniformBuffer::Unbind() const
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    }

    /**
     * @brief Updates a portion of the uniform buffer with new matrix data.
     *
     * This function uploads a 4x4 matrix (glm::mat4) to the uniform buffer object at the specified offset.
     *
     * @param offset The byte offset within the buffer where the data should be updated.
     * @param size The size in bytes of the data to update (should match sizeof(glm::mat4)).
     * @param data The matrix data to upload to the buffer.
     */
    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat4& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Updates a portion of the uniform buffer with the provided 3x3 matrix data.
     *
     * This function uploads the contents of a glm::mat3 to the uniform buffer object,
     * starting at the specified offset and writing the specified number of bytes.
     *
     * @param offset The byte offset within the buffer where the data should be written.
     * @param size The size in bytes of the data to write.
     * @param data The glm::mat3 matrix containing the data to upload.
     */
    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat3& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Sets a portion of the uniform buffer's data with the provided glm::vec4 value.
     *
     * Updates the buffer object identified by m_UniformBufferID starting at the specified offset,
     * writing 'size' bytes from the memory pointed to by the glm::vec4 'data'.
     *
     * @param offset The byte offset within the buffer where the data replacement should begin.
     * @param size The size in bytes of the data to be written.
     * @param data The glm::vec4 value to write into the buffer.
     */
    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec4& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Updates a portion of the uniform buffer with new data.
     *
     * This function uploads a glm::vec3 value to the uniform buffer starting at the specified offset.
     *
     * @param offset The byte offset within the buffer where the data upload should begin.
     * @param size The size in bytes of the data to upload (should match sizeof(glm::vec3)).
     * @param data The glm::vec3 data to be uploaded to the buffer.
     */
    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec3& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Updates a portion of the uniform buffer with new data.
     *
     * This function uploads a glm::vec2 value to the uniform buffer starting at the specified offset.
     *
     * @param offset The byte offset within the buffer where the data upload should begin.
     * @param size The size in bytes of the data to upload (should match sizeof(glm::vec2)).
     * @param data The glm::vec2 data to be uploaded to the buffer.
     */
    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec2& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    /**
     * @brief Updates a portion of the uniform buffer with a single float value.
     *
     * This function uploads a float value to the uniform buffer starting at the specified offset.
     *
     * @param offset The byte offset within the buffer where the data upload should begin.
     * @param size The size in bytes of the data to upload (should be sizeof(float)).
     * @param data The float value to be uploaded to the buffer.
     */
    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, float data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, &data);
    }

    /**
     * @brief Sets raw buffer data for the uniform buffer.
     *
     * This function uploads raw data to the uniform buffer object, allowing for
     * arbitrary data to be stored in the buffer.
     *
     * @param size The size in bytes of the data to upload.
     * @param data Pointer to the raw data to be uploaded.
     */
    void GL_UniformBuffer::SetRawBufferData(std::int32_t size, const void* data)
    {
        glNamedBufferSubData(m_UniformBufferID, 0, size, data);
    }

    /**
     * @brief Creates a shared pointer to a GL_UniformBuffer with the specified size and binding point.
     * This factory method allocates a new GL_UniformBuffer object and returns it wrapped in a shared pointer.
     *
     * @param size The size in bytes of the uniform buffer to allocate.
     * @param binding The binding point to which this uniform buffer will be bound.
     * @return std::shared_ptr<GL_UniformBuffer> A shared pointer to the created GL_UniformBuffer object.
    */
    std::shared_ptr<GL_UniformBuffer> GL_UniformBuffer::Create(std::int32_t size, BindingPoint binding)
    {
        return std::make_shared<GL_UniformBuffer>(size, binding);
    }

    /******************************************************************************************
     *                         GL_FrameBuffer Implementation                                  *
     *******************************************************************************************/

     /**
      * @brief Constructs a GL_FrameBuffer with the specified frame buffer specification.
      *
      * This constructor initializes an OpenGL frame buffer object based on the provided
      * FrameBufferSpecification, setting up color attachments, depth/stencil attachments,
      * and multi-sampling if specified.
      *
      * @param specification The FrameBufferSpecification containing details for the frame buffer.
      */
    GL_FrameBuffer::GL_FrameBuffer(const FrameBufferSpecification& specification) : m_Specification(specification)
    {
        Invalidate();
    }

    /**
     * @brief Destructor for the GL_FrameBuffer class.
     *
     * This destructor releases the OpenGL frame buffer resources associated with this object
     * by calling glDeleteFramebuffers on the frame buffer ID. Ensures proper cleanup of GPU
     * resources when the GL_FrameBuffer instance is destroyed.
     */
    GL_FrameBuffer::~GL_FrameBuffer()
    {
        glDeleteFramebuffers(1, &m_FrameBufferID);

        for (const auto& [attachmentPoint, colorAttachment] : m_ColorAttachments)
        {
            glDeleteTextures(1, &colorAttachment.ID);
        }

        if (m_DepthAttachment.ID)
            glDeleteTextures(1, &m_DepthAttachment.ID);

        m_ColorAttachments.clear();
        m_DepthAttachment = {};
    }

    /**
     * @brief Binds the frame buffer for rendering operations.
     *
     * This method sets the current OpenGL frame buffer to the one represented by this object,
     * allowing subsequent rendering commands to target this frame buffer.
     */
    void GL_FrameBuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
        glViewport(0, 0, m_Specification.Width, m_Specification.Height);
    }

    /**
     * @brief Unbinds the currently bound frame buffer, resetting to the default frame buffer.
     *
     * This method unbinds the frame buffer by binding the default frame buffer (ID 0),
     * effectively resetting the OpenGL context to render to the default framebuffer.
     */
    void GL_FrameBuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    /**
     * @brief Invalidates the current frame buffer and re-creates it based on the specification.
     *
     * This method deletes the existing frame buffer and its attachments, then re-creates them
     * according to the current FrameBufferSpecification. It sets up color attachments, depth/stencil
     * attachments, and multi-sampling if specified.
     */
    void GL_FrameBuffer::ResizeFrame(std::int32_t width, std::int32_t Height)
    {
        m_Specification.Width = width;
        m_Specification.Height = Height;

        Invalidate();
    }

    /**
     * @brief Invalidates the current frame buffer and re-creates it based on the specification.
     *
     * This method deletes the existing frame buffer and its attachments, then re-creates them
     * according to the current FrameBufferSpecification. It sets up color attachments, depth/stencil
     * attachments, and multi-sampling if specified.
     */
    static GLbitfield GetBlitMask(FrameBufferBlitMask mask)
    {
        switch (mask)
        {
        case FrameBufferBlitMask::None: MOTION_ASSERT(false, "Blit mask cannot be None"); return 0; // No bits set
        case FrameBufferBlitMask::Color: return GL_COLOR_BUFFER_BIT;
        case FrameBufferBlitMask::Depth: return GL_DEPTH_BUFFER_BIT;
        case FrameBufferBlitMask::Stencil: return GL_STENCIL_BUFFER_BIT;
        case FrameBufferBlitMask::All: return GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
        default: return GL_COLOR_BUFFER_BIT; // Default to color buffer if mask is not recognized
        };
    }

    /**
     * @brief Converts a FrameBufferBlitFilter enum value to the corresponding OpenGL filter constant.
     *
     * This function maps custom framebuffer blit filters to their equivalent OpenGL filter constants.
     * It is typically used when performing blitting operations between framebuffers.
     *
     * @param filter The FrameBufferBlitFilter value to convert.
     * @return GLenum The corresponding OpenGL filter constant.
     *
     * @note If an unsupported filter is provided, the function asserts and defaults to GL_NEAREST.
     */
    static GLenum GetBlitFilter(FrameBufferBlitFilter filter)
    {
        switch (filter)
        {
        case FrameBufferBlitFilter::Nearest: return GL_NEAREST;
        case FrameBufferBlitFilter::Linear: return GL_LINEAR;
        default: MOTION_ASSERT(false, "Unsupported blit filter"); return GL_NEAREST; // Default to nearest if not recognized
        }
    }

    /**
     * @brief Blits the contents of this framebuffer to another framebuffer.
     *
     * This function copies the contents of the current framebuffer to the target framebuffer
     * using OpenGL's glBlitFramebuffer function. It allows for selective copying of color,
     * depth, and stencil buffers based on the provided mask and filter.
     *
     * @param targetFrameBuffer The target framebuffer to which the contents will be copied.
     * @param mask The FrameBufferBlitMask specifying which buffers to copy.
     * @param filter The FrameBufferBlitFilter specifying the filtering method to use during blitting.
     */
    void GL_FrameBuffer::BlitTo(IFrameBuffer* targetFrameBuffer, FrameBufferBlitMask mask, FrameBufferBlitFilter filter)
    {
        auto* target = dynamic_cast<GL_FrameBuffer*>(targetFrameBuffer);
        if (!target)
        {
            MOTION_ASSERT(false, "Target frame buffer is not a GL_FrameBuffer");
            return;
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FrameBufferID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target->m_FrameBufferID);

        glBlitFramebuffer(
            0, 0, m_Specification.Width, m_Specification.Height,
            0, 0, target->m_Specification.Width, target->m_Specification.Height,
            GetBlitMask(mask),
            GetBlitFilter(filter)
        );

        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    }

    /**
     * Resolves the contents of this framebuffer to the specified target framebuffer.
     *
     * If the current framebuffer uses multisampling (i.e., m_Specification.Samples > 1),
     * this function performs a blit operation to resolve the multisampled color buffer
     * into the target framebuffer using the nearest filter. Otherwise, it simply returns
     * the texture ID of the standard color attachment.
     *
     * @param target Pointer to the target IFrameBuffer to which the contents will be resolved.
     * @return FrameTextureID The texture ID of the standard color attachment after resolving.
     */
    FrameTextureID GL_FrameBuffer::ResolveTo(IFrameBuffer* target)
    {
        if (m_Specification.Samples > 1)
        {
            BlitTo(target, FrameBufferBlitMask::Color, FrameBufferBlitFilter::Nearest);
            return GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
        }

        return GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
    }


    /**
     * @brief Retrieves the color attachment associated with the specified framebuffer color attachment format.
     *
     * Searches through the framebuffer's color attachments to find one that matches the given
     * attachment format. If found, returns the corresponding ColorAttachments object.
     * If not found, triggers an assertion and returns a default-constructed ColorAttachments object.
     *
     * @param attachment The framebuffer color attachment format to search for.
     * @return The ColorAttachments object corresponding to the specified format.
     * @note If the attachment is not found, an assertion will be triggered.
     */
    ColorAttachments GL_FrameBuffer::GetAttachment(FrameBufferColorAttachmentStandards attachment) const
    {
        auto it = std::find_if(m_ColorAttachments.begin(), m_ColorAttachments.end(),
            [&](const std::pair<const std::int32_t, ColorAttachments>& pair)
            {
                return pair.second.Format == attachment;
            }
        );

        if (it != m_ColorAttachments.end())
        {
            return it->second;
        }

        MOTION_ASSERT(false, "Attachment not found in the framebuffer");
        return ColorAttachments();
    }


    /**
     * @brief Reads the value of a single pixel from a specified color attachment in the framebuffer.
     *
     * This function searches for the given color attachment in the framebuffer's color attachments.
     * If found, it sets the read buffer to the corresponding attachment point and reads the pixel value
     * at the specified (x, y) coordinates using integer format. If the attachment is not found, an error
     * is logged and 0 is returned.
     *
     * @param attachment The color attachment from which to read the pixel.
     * @param x The x-coordinate of the pixel to read.
     * @param y The y-coordinate of the pixel to read.
     * @return The integer value of the pixel at the specified coordinates, or 0 if the attachment is not found.
     */
    std::int32_t GL_FrameBuffer::ReadPixel(FrameBufferColorAttachmentStandards attachment, std::int32_t x, std::int32_t y)
    {
        auto it = std::find_if(m_ColorAttachments.begin(), m_ColorAttachments.end(),
            [&](const std::pair<const std::int32_t, ColorAttachments>& pair)
            {
                return pair.second.Format == attachment;
            }
        );

        if (it != m_ColorAttachments.end())
        {
            std::int32_t attachmentPoint = it->second.AttachmentPoint;
            glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentPoint);

            std::int32_t pixelData = 0;
            glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

            return pixelData;
        }

        MOTION_CORE_ERROR("Attachment not found in the framebuffer");
        return 0; // Return 0 or an appropriate error value if the attachment is not found
    }

    /**
     * @brief Creates a shared pointer to a GL_FrameBuffer with the specified FrameBufferSpecification.
     *
     * This factory method allocates a new GL_FrameBuffer object and returns it wrapped in a shared pointer.
     * The GL_FrameBuffer is initialized with the provided specification, which includes details like width,
     * height, color attachments, depth attachments, and multi-sampling settings.
     *
     * @param specification The FrameBufferSpecification containing details for the frame buffer.
     * @return std::shared_ptr<GL_FrameBuffer> A shared pointer to the created GL_FrameBuffer object.
     */
    std::shared_ptr<GL_FrameBuffer> GL_FrameBuffer::Create(const FrameBufferSpecification& specification)
    {
        return std::make_shared<GL_FrameBuffer>(specification);
    }

    /**
     * @brief Returns the OpenGL format for the specified color attachment type.
     *
     * This function maps the FrameBufferColorAttachmentStandards enum to the corresponding OpenGL
     * format used for color attachments in a framebuffer.
     *
     * @param type The FrameBufferColorAttachmentStandards type to convert.
     * @return GLenum The OpenGL format constant (e.g., GL_RGBA8, GL_RGBA16F).
     */
    static GLenum GL_ColorAttachmentFormat(FrameBufferColorAttachmentStandards type)
    {
        switch (type)
        {
        case FrameBufferColorAttachmentStandards::Standard: return GL_RGBA8;
        case FrameBufferColorAttachmentStandards::HighDynamicRange: return GL_RGBA16F;
        case FrameBufferColorAttachmentStandards::LightweightHDR: return GL_RGB10_A2;
        case FrameBufferColorAttachmentStandards::SingleChannelFloat16: return GL_R16F;
        case FrameBufferColorAttachmentStandards::SingleChannelFloat32: return GL_R32F;
        case FrameBufferColorAttachmentStandards::MultiChannelFloat16: return GL_RG16F;
        case FrameBufferColorAttachmentStandards::MultiChannelFloat32: return GL_RG32F;
        default: return GL_NONE;
        }
    }

    /**
     * @brief Returns the OpenGL format for the specified color attachment type.
     *
     * This function maps the FrameBufferColorAttachmentStandards enum to the corresponding OpenGL
     * format used for color attachments in a framebuffer.
     *
     * @param type The FrameBufferColorAttachmentStandards type to convert.
     * @return GLenum The OpenGL format constant (e.g., GL_RGBA8, GL_RGBA16F).
     */
    static GLenum GL_Texture2D_Format(FrameBufferColorAttachmentStandards type)
    {
        switch (type)
        {
        case FrameBufferColorAttachmentStandards::Standard: return GL_RGBA;
        case FrameBufferColorAttachmentStandards::HighDynamicRange: return GL_RGBA;
        case FrameBufferColorAttachmentStandards::LightweightHDR: return GL_RGB;
        case FrameBufferColorAttachmentStandards::SingleChannelFloat16: return GL_RED;
        case FrameBufferColorAttachmentStandards::SingleChannelFloat32: return GL_RED;
        case FrameBufferColorAttachmentStandards::MultiChannelFloat16: return GL_RG;
        case FrameBufferColorAttachmentStandards::MultiChannelFloat32: return GL_RG;
        default: return GL_NONE;
        }
    }

    /**
     * @brief Returns the OpenGL type for the specified color attachment type.
     *
     * This function maps the FrameBufferColorAttachmentStandards enum to the corresponding OpenGL
     * type used for color attachments in a framebuffer.
     *
     * @param type The FrameBufferColorAttachmentStandards type to convert.
     * @return GLenum The OpenGL type constant (e.g., GL_UNSIGNED_BYTE, GL_HALF_FLOAT).
     */
    static  GLenum GL_Texture2D_Type(FrameBufferColorAttachmentStandards type)
    {
        switch (type)
        {
        case FrameBufferColorAttachmentStandards::Standard: return GL_UNSIGNED_BYTE;
        case FrameBufferColorAttachmentStandards::HighDynamicRange: return GL_HALF_FLOAT;
        case FrameBufferColorAttachmentStandards::LightweightHDR: return GL_UNSIGNED_INT_2_10_10_10_REV;
        case FrameBufferColorAttachmentStandards::SingleChannelFloat16: return GL_HALF_FLOAT;
        case FrameBufferColorAttachmentStandards::SingleChannelFloat32: return GL_FLOAT;
        case FrameBufferColorAttachmentStandards::MultiChannelFloat16: return GL_HALF_FLOAT;
        case FrameBufferColorAttachmentStandards::MultiChannelFloat32: return GL_FLOAT;
        default: return GL_NONE;
        }
    }

    /**
     * @brief Returns the OpenGL format for the specified depth attachment type.
     *
     * This function maps the FrameBufferDepthAttachmentStandards enum to the corresponding OpenGL
     * format used for depth or depth-stencil attachments in a framebuffer.
     *
     * @param type The FrameBufferDepthAttachmentStandards type to convert.
     * @return GLenum The OpenGL format constant (e.g., GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT32F).
     */
    static GLenum GL_DepthAttachmentFormat(FrameBufferDepthAttachmentStandards type)
    {
        switch (type)
        {
        case FrameBufferDepthAttachmentStandards::Standard:
        case FrameBufferDepthAttachmentStandards::StandardPrecision: return GL_DEPTH_COMPONENT24;
        case FrameBufferDepthAttachmentStandards::HighPrecision: return GL_DEPTH_COMPONENT32F;
        case FrameBufferDepthAttachmentStandards::CommonCombined: return GL_DEPTH24_STENCIL8;
        case FrameBufferDepthAttachmentStandards::HighPrecisionCombined: return GL_DEPTH32F_STENCIL8;
        default: return GL_NONE;
        }
    }

    /**
     * @brief Returns the OpenGL type for the specified depth attachment type.
     *
     * This function maps the FrameBufferDepthAttachmentStandards enum to the corresponding OpenGL
     * type used for depth or depth-stencil attachments in a framebuffer.
     *
     * @param type The FrameBufferDepthAttachmentStandards type to convert.
     * @return GLenum The OpenGL type constant (e.g., GL_UNSIGNED_INT, GL_FLOAT).
     */
    static GLenum GL_Texture2D_DepthFormat(FrameBufferDepthAttachmentStandards type)
    {
        switch (type)
        {
        case FrameBufferDepthAttachmentStandards::Standard:
        case FrameBufferDepthAttachmentStandards::StandardPrecision: return GL_DEPTH_COMPONENT;
        case FrameBufferDepthAttachmentStandards::HighPrecision: return GL_DEPTH_COMPONENT;
        case FrameBufferDepthAttachmentStandards::CommonCombined: return GL_DEPTH_STENCIL;
        case FrameBufferDepthAttachmentStandards::HighPrecisionCombined: return GL_DEPTH_STENCIL;
        default: return GL_NONE;
        }
    }

    /**
     * @brief Returns the OpenGL data type for the specified depth attachment type.
     *
     * This function maps the FrameBufferDepthAttachmentStandards enum to the corresponding OpenGL
     * data type used for depth or depth-stencil attachments in a framebuffer.
     *
     * @param type The FrameBufferDepthAttachmentStandards type to convert.
     * @return GLenum The OpenGL data type constant (e.g., GL_UNSIGNED_INT, GL_FLOAT).
     */
    static GLenum GL_Texture2D_DepthType(FrameBufferDepthAttachmentStandards type)
    {
        switch (type)
        {
        case FrameBufferDepthAttachmentStandards::Standard:
        case FrameBufferDepthAttachmentStandards::StandardPrecision: return GL_UNSIGNED_INT;
        case FrameBufferDepthAttachmentStandards::HighPrecision: return GL_FLOAT;
        case FrameBufferDepthAttachmentStandards::CommonCombined: return GL_UNSIGNED_INT_24_8;
        case FrameBufferDepthAttachmentStandards::HighPrecisionCombined: return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
        default: return GL_NONE;
        }
    }

    /**
     * @brief Returns the OpenGL attachment point for the specified depth attachment type.
     *
     * This function maps the FrameBufferDepthAttachmentStandards enum to the corresponding OpenGL
     * attachment point used for depth or depth-stencil attachments in a framebuffer.
     *
     * @param type The FrameBufferDepthAttachmentStandards type to convert.
     * @return GLenum The OpenGL attachment point constant (e.g., GL_DEPTH_ATTACHMENT, GL_DEPTH_STENCIL_ATTACHMENT).
     */
    static GLenum GetDepthAttachmentPoint(FrameBufferDepthAttachmentStandards type)
    {
        switch (type)
        {
        case FrameBufferDepthAttachmentStandards::CommonCombined:
        case FrameBufferDepthAttachmentStandards::HighPrecisionCombined: return GL_DEPTH_STENCIL_ATTACHMENT;
        case FrameBufferDepthAttachmentStandards::Standard:
        case FrameBufferDepthAttachmentStandards::StandardPrecision:
        case FrameBufferDepthAttachmentStandards::HighPrecision: return GL_DEPTH_ATTACHMENT;
        default: return GL_NONE;
        }
    }

    /**
     * @brief Recreates and configures the OpenGL framebuffer and its attachments.
     *
     * This method deletes any existing framebuffer and its associated color and depth attachments,
     * then generates a new framebuffer object. It creates and attaches color textures for each
     * color attachment specified in the framebuffer specification, and optionally creates and
     * attaches a depth-stencil texture if required. The method also sets up the draw buffers
     * according to the number of color attachments and verifies that the framebuffer is complete.
     *
     * Typical usage involves calling this method when the framebuffer specification changes,
     * such as when resizing or reconfiguring attachments.
     *
     * @note Throws an assertion if the framebuffer is not complete after setup.
     */
    void GL_FrameBuffer::Invalidate()
    {
        if (m_FrameBufferID)
        {
            glDeleteFramebuffers(1, &m_FrameBufferID);

            for (const auto& [attachmentPoint, colorAttachment] : m_ColorAttachments)
            {
                glDeleteTextures(1, &colorAttachment.ID);
            }

            if (m_DepthAttachment.ID)
                glDeleteTextures(1, &m_DepthAttachment.ID);

            m_ColorAttachments.clear();
            m_DepthAttachment = {};
        }

        glGenFramebuffers(1, &m_FrameBufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);

        const bool hasDepth = m_Specification.Depth.Format != FrameBufferDepthAttachmentStandards::None;
        const bool useMultiSampling = m_Specification.Samples > 1;

        for (std::int32_t i = 0; i < m_Specification.Colors.size(); i++)
        {
            auto colorAttachment = m_Specification.Colors[i];
            if (useMultiSampling)
            {
                glGenTextures(1, &colorAttachment.ID);
                glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, colorAttachment.ID);
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specification.Samples, GL_ColorAttachmentFormat(colorAttachment.Format), m_Specification.Width, m_Specification.Height, GL_TRUE);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, -1000);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, 1000);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, 0);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachment.AttachmentPoint, GL_TEXTURE_2D_MULTISAMPLE, colorAttachment.ID, 0);

            }
            else
            {
                glGenTextures(1, &colorAttachment.ID);
                glBindTexture(GL_TEXTURE_2D, colorAttachment.ID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_ColorAttachmentFormat(colorAttachment.Format), m_Specification.Width, m_Specification.Height, 0, GL_Texture2D_Format(colorAttachment.Format), GL_Texture2D_Type(colorAttachment.Format), nullptr);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachment.AttachmentPoint, GL_TEXTURE_2D, colorAttachment.ID, 0);
            }

            m_ColorAttachments[i] = colorAttachment;
        }

        if (hasDepth)
        {
            glGenTextures(1, &m_DepthAttachment.ID);
            glBindTexture(GL_TEXTURE_2D, m_DepthAttachment.ID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DepthAttachmentFormat(m_Specification.Depth.Format), m_Specification.Width, m_Specification.Height, 0, GL_Texture2D_DepthFormat(m_Specification.Depth.Format), GL_Texture2D_DepthType(m_Specification.Depth.Format), nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, (m_DepthAttachment.AttachmentPoint = GetDepthAttachmentPoint(m_Specification.Depth.Format)), GL_TEXTURE_2D, m_DepthAttachment.ID, 0);
        }

        if (!m_ColorAttachments.empty())
        {
            std::vector<GLenum> drawBuffers;

            for (const auto& [index, attachment] : m_ColorAttachments)
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + attachment.AttachmentPoint);

            if (drawBuffers.size() == 1)
            {
                glDrawBuffer(drawBuffers[0]);
            }
            else
            {
                glDrawBuffers(static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());
            }
        }
        else
        {
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }


        MOTION_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is not complete!");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    /******************************************************************************************
     *                         GL_CaptureFrameBuffer Implementation                            *
     *******************************************************************************************/

     /**
      * @brief Constructs a GL_CaptureFrameBuffer object with the specified width and height.
      *
      * This constructor creates and configures an OpenGL framebuffer and a renderbuffer for depth storage.
      * It generates the framebuffer and renderbuffer objects, binds them, allocates storage for the renderbuffer
      * with a 24-bit depth component, and attaches the renderbuffer to the framebuffer as a depth attachment.
      * After setup, it unbinds both the renderbuffer and framebuffer. The constructor also asserts that the
      * framebuffer is complete.
      *
      * @param width  The width of the framebuffer and renderbuffer in pixels.
      * @param height The height of the framebuffer and renderbuffer in pixels.
      */
    GL_CaptureFrameBuffer::GL_CaptureFrameBuffer(std::int32_t width, std::int32_t height)
    {
        glGenFramebuffers(1, &m_CaptureFrameBufferID);
        glGenRenderbuffers(1, &m_CaptureRenderTextureID);

        glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFrameBufferID);
        glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRenderTextureID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_CaptureRenderTextureID);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        m_Width = width;
        m_Height = height;

        MOTION_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Capture Framebuffer is not complete!");
    }

    /**
     * @brief Binds the framebuffer for capturing and sets the viewport.
     *
     * This method binds the framebuffer identified by m_CaptureFrameBufferID
     * as the current framebuffer target using glBindFramebuffer. It also sets
     * the OpenGL viewport to match the dimensions specified by m_Width and m_Height,
     * ensuring that rendering operations are directed to the correct region of the framebuffer.
     */
    void GL_CaptureFrameBuffer::BindFrameBuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFrameBufferID);
    }

    /**
     * @brief Unbinds the currently bound OpenGL framebuffer.
     *
     * This method resets the framebuffer binding to the default framebuffer (0),
     * effectively unbinding any custom framebuffer that was previously bound.
     * It is typically called after rendering to a framebuffer is complete,
     * to resume rendering to the default window framebuffer.
     */
    void GL_CaptureFrameBuffer::UnbindFrameBuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    /**
     * @brief Binds the renderbuffer for capturing depth information.
     *
     * This method binds the renderbuffer identified by m_CaptureRenderTextureID
     * as the current renderbuffer target using glBindRenderbuffer. It is typically
     * used to prepare the renderbuffer for depth storage operations in OpenGL.
     */
    void GL_CaptureFrameBuffer::BindRenderBuffer()
    {
        glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRenderTextureID);
    }

    /**
     * @brief Unbinds the currently bound OpenGL renderbuffer.
     *
     * This method resets the renderbuffer binding to 0, effectively unbinding any
     * custom renderbuffer that was previously bound. It is typically called after
     * rendering operations involving the renderbuffer are complete.
     */
    void GL_CaptureFrameBuffer::UnbindRenderBuffer()
    {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    /**
     * @brief Resizes the capture framebuffer to the specified width and height.
     *
     * This method checks if the new dimensions are different from the current ones.
     * If they are, it deletes the existing framebuffer and renderbuffer, generates new ones,
     * and sets up the renderbuffer storage with the new dimensions. It also updates the
     * internal width and height variables and asserts that the framebuffer is complete.
     *
     * @param width The new width of the framebuffer in pixels.
     * @param height The new height of the framebuffer in pixels.
     */
    void GL_CaptureFrameBuffer::ResizeFrame(std::int32_t width, std::int32_t height)
    {
        if (width == m_Width && height == m_Height)
        {
            MOTION_CORE_WARN("Capture FrameBuffer Resize called with same dimensions, ignoring.");
            return;
        }
        else
        {
            glDeleteFramebuffers(1, &m_CaptureFrameBufferID);
            glDeleteRenderbuffers(1, &m_CaptureRenderTextureID);

            m_CaptureFrameBufferID = 0;
            m_CaptureRenderTextureID = 0;

            glGenFramebuffers(1, &m_CaptureFrameBufferID);
            glGenRenderbuffers(1, &m_CaptureRenderTextureID);

            glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFrameBufferID);
            glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRenderTextureID);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_CaptureRenderTextureID);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            m_Width = width;
            m_Height = height;

            MOTION_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Capture Framebuffer is not complete!");
        }
    }

    /**
     * @brief Creates a shared pointer to a GL_CaptureFrameBuffer with the specified width and height.
     *
     * This factory method allocates a new GL_CaptureFrameBuffer object and returns it wrapped in a shared pointer.
     * The GL_CaptureFrameBuffer is initialized with the provided width and height, setting up the framebuffer and
     * renderbuffer for capturing depth information.
     *
     * @param width  The width of the framebuffer in pixels.
     * @param height The height of the framebuffer in pixels.
     * @return std::shared_ptr<GL_CaptureFrameBuffer> A shared pointer to the created GL_CaptureFrameBuffer object.
     */
    std::shared_ptr<GL_CaptureFrameBuffer> GL_CaptureFrameBuffer::Create(std::int32_t width, std::int32_t height)
    {
        return std::make_shared<GL_CaptureFrameBuffer>(width, height);
    }
}