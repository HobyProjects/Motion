#include "CorePCH.hpp"
#include "GL_Buffers.hpp"

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

    void GL_ElementBuffer::Unbind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    GL_ShaderBuffer::GL_ShaderBuffer(uint32_t size, BindingPoint binding)
    {
        glCreateBuffers(1, &m_ShaderBufferID);
        glNamedBufferData(m_ShaderBufferID, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        m_BindingPoint = binding;
    }

    GL_ShaderBuffer::~GL_ShaderBuffer()
    {
        glDeleteBuffers(1, &m_ShaderBufferID);
    }

    void GL_ShaderBuffer::Bind() const
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, m_BindingPoint, m_ShaderBufferID);
    }

    void GL_ShaderBuffer::Unbind() const
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::mat4& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::mat3& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec4& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec3& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec2& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, float data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, &data);
    }

    GL_UniformBuffer::GL_UniformBuffer(uint32_t size, BindingPoint binding)
    {
        glCreateBuffers(1, &m_UniformBufferID);
        glNamedBufferData(m_UniformBufferID, size, nullptr, GL_DYNAMIC_STORAGE_BIT);
        m_BindingPoint = binding;
    }

    GL_UniformBuffer::~GL_UniformBuffer()
    {
        glDeleteBuffers(1, &m_UniformBufferID);
    }

    void GL_UniformBuffer::Bind() const
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, m_BindingPoint, m_UniformBufferID);
    }

    void GL_UniformBuffer::Unbind() const
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::mat4& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::mat3& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec4& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec3& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec2& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, float data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, &data);
    }

    GL_FrameBuffer::GL_FrameBuffer(const FrameBufferSpecification& specification) : m_Specification(specification)
    {
        //[FIXME] This is a temporary solution to create a post-processing quad.
        //m_PostProcessingQuad = QuickMesh::CreateQuad(std::format("{}_{}", m_Specification.Width, m_Specification.Height);
        CreateFrame();
    }

    GL_FrameBuffer::~GL_FrameBuffer()
    {
        glDeleteFramebuffers(1, &m_FrameBufferID);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteTextures(1, &m_DepthAttachment);
    }

    void GL_FrameBuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
    }

    void GL_FrameBuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        Resolve();
        Render();
    }

    void GL_FrameBuffer::BindAttachment()
    {
        if (IsMSAA())
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ResolvedColorAttachment);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
        }
    }

    void GL_FrameBuffer::UnbindAttachment()
    {
        if (IsMSAA())
        {
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    void GL_FrameBuffer::ResizeFrame(uint32_t width, uint32_t height)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;

        CreateFrame();
    }

    void GL_FrameBuffer::Resolve()
    {
        if (!IsMSAA()) return;

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FrameBufferID);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ResolvedFBOID);
        glBlitFramebuffer(
            0, 0, m_Specification.Width, m_Specification.Height,
            0, 0, m_Specification.Width, m_Specification.Height,
            GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST
        );
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GL_FrameBuffer::Render()
    {

    }

    void GL_FrameBuffer::CreateFrame()
    {
        DeleteFrameBuffers();
        bool useMSAA = m_Specification.Samples > 1;

        // --- Main FBO ---
        glCreateFramebuffers(1, &m_FrameBufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);

        // --- Color Attachment ---
        if (useMSAA) {
            glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_ColorAttachment);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specification.Samples, GL_RGBA8, m_Specification.Width, m_Specification.Height, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, m_ColorAttachment, 0);
        }
        else {
            glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorAttachment);
            glTextureStorage2D(m_ColorAttachment, 1, GL_RGBA8, m_Specification.Width, m_Specification.Height);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);
        }

        // --- Depth Attachment ---
        if (useMSAA) {
            glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 1, &m_DepthAttachment);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, m_DepthAttachment);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_Specification.Samples, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, m_DepthAttachment, 0);
        }
        else {
            glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthAttachment);
            glTextureStorage2D(m_DepthAttachment, 1, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);
        }

        MOTION_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is not complete!");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- Resolved (single-sample) FBO for MSAA ---
        if (useMSAA) {
            glCreateFramebuffers(1, &m_ResolvedFBOID);
            glBindFramebuffer(GL_FRAMEBUFFER, m_ResolvedFBOID);

            glCreateTextures(GL_TEXTURE_2D, 1, &m_ResolvedColorAttachment);
            glTextureStorage2D(m_ResolvedColorAttachment, 1, GL_RGBA8, m_Specification.Width, m_Specification.Height);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ResolvedColorAttachment, 0);

            glCreateTextures(GL_TEXTURE_2D, 1, &m_ResolvedDepthAttachment);
            glTextureStorage2D(m_ResolvedDepthAttachment, 1, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_ResolvedDepthAttachment, 0);

            MOTION_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Resolved FBO is not complete!");

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    void GL_FrameBuffer::DeleteFrameBuffers()
    {
        if (m_FrameBufferID) glDeleteFramebuffers(1, &m_FrameBufferID);
        if (m_ColorAttachment) glDeleteTextures(1, &m_ColorAttachment);
        if (m_DepthAttachment) glDeleteTextures(1, &m_DepthAttachment);
        if (m_ResolvedFBOID) glDeleteFramebuffers(1, &m_ResolvedFBOID);
        if (m_ResolvedColorAttachment) glDeleteTextures(1, &m_ResolvedColorAttachment);
        if (m_ResolvedDepthAttachment) glDeleteTextures(1, &m_ResolvedDepthAttachment);

        m_FrameBufferID = m_ColorAttachment = m_DepthAttachment = 0;
        m_ResolvedFBOID = m_ResolvedColorAttachment = m_ResolvedDepthAttachment = 0;
    }

    std::shared_ptr<GL_VertexBuffer> GL_CreateVertexBuffer(uint32_t alloca_size)
    {
        return std::make_shared<GL_VertexBuffer>(alloca_size);
    }

    std::shared_ptr<GL_VertexBuffer> GL_CreateVertexBuffer(float* data, uint32_t size)
    {
        return std::make_shared<GL_VertexBuffer>(data, size);
    }

    std::shared_ptr<GL_ElementBuffer> GL_CreateElementBuffer(uint32_t* data, uint32_t size)
    {
        return std::make_shared<GL_ElementBuffer>(data, size);
    }

    std::shared_ptr<GL_ShaderBuffer> GL_CreateShaderBuffer(uint32_t size, BindingPoint binding)
    {
        return std::make_shared<GL_ShaderBuffer>(size, binding);
    }

    std::shared_ptr<GL_UniformBuffer> GL_CreateUniformBuffer(uint32_t size, BindingPoint binding)
    {
        return std::make_shared<GL_UniformBuffer>(size, binding);
    }

    std::shared_ptr<GL_FrameBuffer> GL_CreateFrameBuffer(const FrameBufferSpecification& specification)
    {
        return std::make_shared<GL_FrameBuffer>(specification);
    }

}