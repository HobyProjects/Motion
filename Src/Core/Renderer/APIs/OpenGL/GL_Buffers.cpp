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

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::mat4 & data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::mat3 & data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec4 & data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec3 & data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec2 & data)
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

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::mat4 & data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::mat3 & data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec4 & data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec3 & data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, const glm::vec2 & data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(uint32_t offset, uint32_t size, float data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, &data);
    }

    GL_FrameBuffer::GL_FrameBuffer(const FrameBufferSpecification& specification) : m_Specification(specification)
    {
        CreateFrame();
    }

    GL_FrameBuffer::~GL_FrameBuffer()
    {
        glDeleteFramebuffers(1, &m_FrameBufferID);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteTextures(1, &m_DepthAttachment);
    }

    void GL_FrameBuffer::Bind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
    }

    void GL_FrameBuffer::Unbind() const
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GL_FrameBuffer::ResizeFrame(uint32_t width, uint32_t height)
    {
        m_Specification.Width = width;
        m_Specification.Height = height;

        CreateFrame();
    }

    void GL_FrameBuffer::CreateFrame()
    {
        if (m_FrameBufferID)
        {
            glDeleteFramebuffers(1, &m_FrameBufferID);
            glDeleteTextures(1, &m_ColorAttachment);
            glDeleteTextures(1, &m_DepthAttachment);
        }

        glCreateFramebuffers(1, &m_FrameBufferID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorAttachment);
        glTextureStorage2D(m_ColorAttachment, 1, GL_RGBA8, m_Specification.Width, m_Specification.Height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_DepthAttachment);
        glTextureStorage2D(m_DepthAttachment, 1, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachment, 0);

        MOTION_ASSERT(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Framebuffer is not complete!");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    std::shared_ptr<GL_VertexBuffer> GL_CreateVertexBuffer(uint32_t alloca_size)
    {
        return std::make_shared<GL_VertexBuffer>(alloca_size);
    }

    std::shared_ptr<GL_VertexBuffer> GL_CreateVertexBuffer(float * data, uint32_t size)
    {
        return std::make_shared<GL_VertexBuffer>(data, size);
    }

    std::shared_ptr<GL_ElementBuffer> GL_CreateElementBuffer(uint32_t * data, uint32_t size)
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

    std::shared_ptr<GL_FrameBuffer> GL_CreateFrameBuffer(const FrameBufferSpecification & specification)
    {
        return std::make_shared<GL_FrameBuffer>(specification);
    }

}