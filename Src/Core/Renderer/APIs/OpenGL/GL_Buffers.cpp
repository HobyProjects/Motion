#include "CorePCH.hpp"
#include "GL_Buffers.hpp"

namespace Motion::Core
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
    GL_VertexBuffer::GL_VertexBuffer(std::uint32_t allocatorSize)
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
    GL_VertexBuffer::GL_VertexBuffer(float* data, std::uint32_t dataSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, sizeof(data[0]) * dataSize, data, GL_STATIC_DRAW);
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
    void GL_VertexBuffer::SetData(const void* data, std::uint32_t size)
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

    /******************************************************************************************
     *                         GL_ElementBuffer Implementation                                *
     *******************************************************************************************/

     /**
      * @brief Constructs a GL_ElementBuffer with the given index data.
      *
      * This constructor creates an OpenGL element buffer object (EBO) and uploads the provided
      * index data to the GPU. The buffer is initialized with the specified number of indices.
      *
      * @param data Pointer to the array of index data (of type std::uint32_t) to be uploaded.
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
    GL_ShaderBuffer::GL_ShaderBuffer(std::uint32_t size, BindingPoint binding)
    {
        glCreateBuffers(1, &m_ShaderBufferID);
        glNamedBufferData(m_ShaderBufferID, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
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
    void GL_ShaderBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::mat4& data)
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
    void GL_ShaderBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::mat3& data)
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
    void GL_ShaderBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec4& data)
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
    void GL_ShaderBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec3& data)
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
    void GL_ShaderBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec2& data)
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
    void GL_ShaderBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, float data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, &data);
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
    GL_UniformBuffer::GL_UniformBuffer(std::uint32_t size, BindingPoint binding)
    {
        glCreateBuffers(1, &m_UniformBufferID);
        glNamedBufferData(m_UniformBufferID, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
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
    void GL_UniformBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::mat4& data)
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
    void GL_UniformBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::mat3& data)
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
    void GL_UniformBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec4& data)
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
    void GL_UniformBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec3& data)
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
    void GL_UniformBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec2& data)
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
    void GL_UniformBuffer::SetBufferData(std::uint32_t offset, std::uint32_t size, float data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, &data);
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
        glDeleteTextures(m_Attachments.size(), m_Attachments.data());
        glDeleteTextures(1, &m_DepthAttachment);
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
    void GL_FrameBuffer::ResizeFrame(std::uint32_t width, std::uint32_t Height)
    {
        m_Specification.Width = width;
        m_Specification.Height = Height;

        Invalidate();
    }

    /**
     * @brief Clears a specific framebuffer attachment to a given integer value.
     *
     * This function clears the texture attached at the specified attachment index
     * using the provided integer value. Currently, it supports attachments with
     * the format FrameBufferTextureFormat::RedInteger.
     *
     * @param attachmentIndex The index of the attachment to clear.
     * @param value The integer value to clear the attachment with.
     */
    void GL_FrameBuffer::ClearAttachment(std::uint32_t attachmentIndex, std::int32_t value)
    {
        auto& format = m_Specification.Attachments[attachmentIndex];
        if (format == FrameBufferTextureFormat::R32I)
        {
            glClearTexImage(m_Attachments[attachmentIndex], 0, GL_RED_INTEGER, GL_INT, &value);
        }
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
     * @brief Retrieves the texture ID of the framebuffer attachment at the specified index.
     *
     * This function returns the texture ID associated with the framebuffer's attachment
     * at the given index. It asserts that the index is within the bounds of the attachments vector.
     *
     * @param index The zero-based index of the attachment to retrieve.
     * @return FrameTextureID The texture ID of the specified attachment.
     * @throws Assertion failure if the index is out of bounds.
     */
    FrameTextureID GL_FrameBuffer::GetAttachmentID(std::uint32_t index) const
    {
        MOTION_ASSERT(index < m_Attachments.size(), "Attachment index out of bounds");
        return m_Attachments[index];
    }

    /**
     * @brief Reads the value of a single pixel from a specified color attachment in the framebuffer.
     *
     * This function sets the read buffer to the specified color attachment and reads the integer value
     * of the pixel at the given (x, y) coordinates. The pixel data is read as a single integer value
     * using the GL_RED_INTEGER format.
     *
     * @param attachmentIndex The index of the color attachment to read from.
     * @param x The x-coordinate of the pixel to read.
     * @param y The y-coordinate of the pixel to read.
     * @return The integer value of the pixel at the specified coordinates.
     *
     * @note The function asserts that the attachment index is within bounds.
     */
    std::int32_t GL_FrameBuffer::ReadPixel(std::uint32_t attachmentIndex, std::int32_t x, std::int32_t y)
    {
        MOTION_ASSERT(attachmentIndex < m_Attachments.size(), "Attachment index out of bounds");
        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

        std::int32_t pixelData = 0;
        glReadPixels(x, y, 1, 1, GL_RED_INTEGER, GL_INT, &pixelData);

        return pixelData;
    }

    /**
     * @brief Converts a FrameBufferTextureFormat enum value to the corresponding OpenGL internal format (GLenum).
     *
     * This function maps custom framebuffer texture formats to their equivalent OpenGL internal format constants.
     * It is typically used when creating or configuring OpenGL textures or framebuffer attachments.
     *
     * @param format The FrameBufferTextureFormat value to convert.
     * @return GLenum The corresponding OpenGL internal format constant.
     *
     * @note If an unsupported format is provided, the function triggers an assertion and returns GL_RGBA8 as a fallback.
     */
    static GLenum ToGLFormat(FrameBufferTextureFormat format)
    {
        switch (format)
        {
        case FrameBufferTextureFormat::RGBA8: return GL_RGBA8;
        case FrameBufferTextureFormat::RGB8: return GL_RGB8;
        case FrameBufferTextureFormat::R16F: return GL_R16F;
        case FrameBufferTextureFormat::R32F: return GL_R32F;
        case FrameBufferTextureFormat::R16I: return GL_R16I;
        case FrameBufferTextureFormat::R32I: return GL_R32I;
        case FrameBufferTextureFormat::Depth24Stencil8: return GL_DEPTH24_STENCIL8;
        case FrameBufferTextureFormat::Depth32F: return GL_DEPTH_COMPONENT32F;
        case FrameBufferTextureFormat::Depth24: return GL_DEPTH_COMPONENT24;
        case FrameBufferTextureFormat::Depth32: return GL_DEPTH_COMPONENT32;
        default: MOTION_ASSERT(false, "Unsupported FrameBufferTextureFormat"); return GL_RGBA8; // Fallback to RGBA8
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
            glDeleteTextures(m_Attachments.size(), m_Attachments.data());
            glDeleteTextures(1, &m_DepthAttachment);

            m_Attachments.clear();
            m_DepthAttachment = 0;
        }

        glGenFramebuffers(1, &m_FrameBufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);

        const bool hasDepth = std::find(m_Specification.Attachments.begin(), m_Specification.Attachments.end(), FrameBufferTextureFormat::Depth24Stencil8) != m_Specification.Attachments.end();
        const bool useMultiSampling = m_Specification.Samples > 1;

        if (hasDepth)
        {
            glGenTextures(1, &m_DepthAttachment);
            glBindTexture(GL_TEXTURE_2D, m_DepthAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
        }


        for (auto format : m_Specification.Attachments)
        {
            if (format == FrameBufferTextureFormat::Depth24Stencil8)
                continue;

            GLuint textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);

            if (useMultiSampling)
            {
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specification.Samples, ToGLFormat(format), m_Specification.Width, m_Specification.Height, GL_TRUE);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_Attachments.size(), GL_TEXTURE_2D_MULTISAMPLE, textureID, 0);

                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_LOD, -1000);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAX_LOD, 1000);
                glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_BASE_LEVEL, 0);

            }
            else
            {
                glTexImage2D(GL_TEXTURE_2D, 0, ToGLFormat(format), m_Specification.Width, m_Specification.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + m_Attachments.size(), GL_TEXTURE_2D, textureID, 0);

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
            }

            m_Attachments.push_back(textureID);
        }

        if (m_Attachments.size() > 1)
        {
            std::vector<GLenum> drawBuffers;
            for (size_t i = 0; i < m_Attachments.size(); ++i)
                drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);

            glDrawBuffers((GLsizei)drawBuffers.size(), drawBuffers.data());
        }
        else
        {
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
        }

        MOTION_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is not complete!");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }












    std::shared_ptr<GL_VertexBuffer> GL_CreateVertexBuffer(std::uint32_t alloca_size)
    {
        return std::make_shared<GL_VertexBuffer>(alloca_size);
    }

    std::shared_ptr<GL_VertexBuffer> GL_CreateVertexBuffer(float* data, std::uint32_t size)
    {
        return std::make_shared<GL_VertexBuffer>(data, size);
    }

    std::shared_ptr<GL_ElementBuffer> GL_CreateElementBuffer(std::uint32_t* data, std::uint32_t size)
    {
        return std::make_shared<GL_ElementBuffer>(data, size);
    }

    std::shared_ptr<GL_ShaderBuffer> GL_CreateShaderBuffer(std::uint32_t size, BindingPoint binding)
    {
        return std::make_shared<GL_ShaderBuffer>(size, binding);
    }

    std::shared_ptr<GL_UniformBuffer> GL_CreateUniformBuffer(std::uint32_t size, BindingPoint binding)
    {
        return std::make_shared<GL_UniformBuffer>(size, binding);
    }

    std::shared_ptr<GL_FrameBuffer> GL_CreateFrameBuffer(const FrameBufferSpecification& specification)
    {
        return std::make_shared<GL_FrameBuffer>(specification);
    }

}