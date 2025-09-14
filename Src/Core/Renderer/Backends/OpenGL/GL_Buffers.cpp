#include "CorePCH.hpp"
#include "Material.hpp"
#include "GL_Buffers.hpp"

namespace Motion
{

    GL_VertexBuffer::GL_VertexBuffer(std::int32_t allocatorSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, allocatorSize, nullptr, GL_DYNAMIC_DRAW);
    }

    GL_VertexBuffer::GL_VertexBuffer(const Vertex* data, std::uint32_t dataSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, sizeof(Vertex) * dataSize, data, GL_STATIC_DRAW);
    }

    GL_VertexBuffer::GL_VertexBuffer(const float* data, std::uint32_t dataSize)
    {
        glCreateBuffers(1, &m_VertexBufferID);
        glNamedBufferData(m_VertexBufferID, sizeof(float) * dataSize, data, GL_STATIC_DRAW);
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

    void GL_VertexBuffer::SetData(const void* data, std::int32_t size)
    {
        glNamedBufferSubData(m_VertexBufferID, 0, size, data);
    }

    void GL_VertexBuffer::SetLayout(const BufferLayout& layout)
    {
        m_Layout = layout;
    }

    std::shared_ptr<GL_VertexBuffer> GL_VertexBuffer::Create(std::int32_t allocatorSize)
    {
        return std::make_shared<GL_VertexBuffer>(allocatorSize);
    }

    std::shared_ptr<GL_VertexBuffer> GL_VertexBuffer::Create(const Vertex* data, std::uint32_t dataSize)
    {
        return std::make_shared<GL_VertexBuffer>(data, dataSize);
    }

    std::shared_ptr<GL_VertexBuffer> GL_VertexBuffer::Create(const float* data, std::uint32_t dataSize)
    {
        return std::make_shared<GL_VertexBuffer>(data, dataSize);
    }

    GL_ElementBuffer::GL_ElementBuffer(const std::uint32_t* data, std::uint32_t indicesCount) : m_Count(indicesCount)
    {
        glCreateBuffers(1, &m_ElementBufferID);
        glNamedBufferData(m_ElementBufferID, indicesCount * sizeof(std::uint32_t), data, GL_STATIC_DRAW);
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

    std::shared_ptr<GL_ElementBuffer> GL_ElementBuffer::Create(const std::uint32_t* data, std::uint32_t indicesCount)
    {
        return std::make_shared<GL_ElementBuffer>(data, indicesCount);
    }

    GL_ShaderBuffer::GL_ShaderBuffer(std::int32_t size, BindingPoint binding)
    {
        glCreateBuffers(1, &m_ShaderBufferID);
        glNamedBufferData(m_ShaderBufferID, size, nullptr, GL_DYNAMIC_DRAW);
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

    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat4& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat3& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec4& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec3& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec2& data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_ShaderBuffer::SetBufferData(std::int32_t offset, std::int32_t size, float data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, &data);
    }

    void GL_ShaderBuffer::SetRawBufferData(std::int32_t size, const void* data)
    {
        glNamedBufferSubData(m_ShaderBufferID, 0, size, data);
    }

    void GL_ShaderBuffer::SetRawBufferData(std::int32_t offset, std::int32_t size, const void * data)
    {
        glNamedBufferSubData(m_ShaderBufferID, offset, size, data);
    }

    void GL_ShaderBuffer::Resize(std::int32_t newSize)
    {
        glNamedBufferData(m_ShaderBufferID, newSize, nullptr, GL_DYNAMIC_DRAW);
    }

    void GL_ShaderBuffer::Orphan(std::int32_t newSize)
    {
        if (newSize > 0) 
        {
            glNamedBufferData(m_ShaderBufferID, newSize, nullptr, GL_DYNAMIC_DRAW);
            return;
        }

        GLint sz = 0;
        glGetNamedBufferParameteriv(m_ShaderBufferID, GL_BUFFER_SIZE, &sz);
        glNamedBufferData(m_ShaderBufferID, sz, nullptr, GL_DYNAMIC_DRAW);
    }

    std::shared_ptr<GL_ShaderBuffer> GL_ShaderBuffer::Create(std::int32_t size, BindingPoint binding)
    {
        return std::make_shared<GL_ShaderBuffer>(size, binding);
    }

    /******************************************************************************************
     *                         GL_UniformBuffer Implementation                                *
     ******************************************************************************************/

    GL_UniformBuffer::GL_UniformBuffer(std::int32_t size, BindingPoint binding)
    {
        glCreateBuffers(1, &m_UniformBufferID);
        glNamedBufferData(m_UniformBufferID, size, nullptr, GL_DYNAMIC_DRAW);
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

    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat4& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat3& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec4& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec3& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec2& data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, glm::value_ptr(data));
    }

    void GL_UniformBuffer::SetBufferData(std::int32_t offset, std::int32_t size, float data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, &data);
    }

    void GL_UniformBuffer::SetRawBufferData(std::int32_t size, const void* data)
    {
        glNamedBufferSubData(m_UniformBufferID, 0, size, data);
    }

    void GL_UniformBuffer::SetRawBufferData(std::int32_t offset, std::int32_t size, const void * data)
    {
        glNamedBufferSubData(m_UniformBufferID, offset, size, data);
    }

    void GL_UniformBuffer::Resize(std::int32_t newSize)
    {
        glNamedBufferData(m_UniformBufferID, newSize, nullptr, GL_DYNAMIC_DRAW);
    }

    void GL_UniformBuffer::Orphan(std::int32_t newSize)
    {
        if (newSize > 0)
        {
            glNamedBufferData(m_UniformBufferID, newSize, nullptr, GL_DYNAMIC_DRAW);
            return;
        }

        GLint sz = 0;
        glGetNamedBufferParameteriv(m_UniformBufferID, GL_BUFFER_SIZE, &sz);
        glNamedBufferData(m_UniformBufferID, sz, nullptr, GL_DYNAMIC_DRAW);
    }

    std::shared_ptr<GL_UniformBuffer> GL_UniformBuffer::Create(std::int32_t size, BindingPoint binding)
    {
        return std::make_shared<GL_UniformBuffer>(size, binding);
    }

    GL_FrameBuffer::GL_FrameBuffer(const FrameBufferSpecification& specification) : m_Specification(specification)
    {
        Invalidate();
    }

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

    void GL_FrameBuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
        glViewport(0, 0, m_Specification.Width, m_Specification.Height);
    }

    void GL_FrameBuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GL_FrameBuffer::ResizeFrame(std::int32_t width, std::int32_t Height)
    {
        m_Specification.Width = width;
        m_Specification.Height = Height;

        Invalidate();
    }

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

    static GLenum GetBlitFilter(FrameBufferBlitFilter filter)
    {
        switch (filter)
        {
        case FrameBufferBlitFilter::Nearest: return GL_NEAREST;
        case FrameBufferBlitFilter::Linear: return GL_LINEAR;
        default: MOTION_ASSERT(false, "Unsupported blit filter"); return GL_NEAREST; // Default to nearest if not recognized
        }
    }

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

    FrameTextureID GL_FrameBuffer::ResolveTo(IFrameBuffer* target)
    {
        if (m_Specification.Samples > 1)
        {
            BlitTo(target, FrameBufferBlitMask::Color, FrameBufferBlitFilter::Nearest);
            return GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
        }

        return GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
    }

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
        return 0;
    }

    std::shared_ptr<GL_FrameBuffer> GL_FrameBuffer::Create(const FrameBufferSpecification& specification)
    {
        return std::make_shared<GL_FrameBuffer>(specification);
    }

    using FB_CAS = FrameBufferColorAttachmentStandards;

    static GLenum GL_ColorAttachmentFormat(FB_CAS type)
    {
        switch (type)
        {
            case FB_CAS::Standard:             return GL_RGBA8;
            case FB_CAS::HighDynamicRange:     return GL_RGBA16F;
            case FB_CAS::LightweightHDR:       return GL_RGB10_A2;
            case FB_CAS::SingleChannelFloat16: return GL_R16F;
            case FB_CAS::SingleChannelFloat32: return GL_R32F;
            case FB_CAS::MultiChannelFloat16:  return GL_RG16F;
            case FB_CAS::MultiChannelFloat32:  return GL_RG32F;
            default:                           return GL_NONE;
        }
    }

    static GLenum GL_Texture2D_Format(FB_CAS type)
    {
        switch (type)
        {
            case FB_CAS::Standard:                 return GL_RGBA;
            case FB_CAS::HighDynamicRange:         return GL_RGBA;
            case FB_CAS::LightweightHDR:           return GL_RGB;
            case FB_CAS::SingleChannelFloat16:     return GL_RED;
            case FB_CAS::SingleChannelFloat32:     return GL_RED;
            case FB_CAS::MultiChannelFloat16:      return GL_RG;
            case FB_CAS::MultiChannelFloat32:      return GL_RG;
            default:                               return GL_NONE;
        }
    }

    static  GLenum GL_Texture2D_Type(FB_CAS type)
    {
        switch (type)
        {
            case FB_CAS::Standard:                 return GL_UNSIGNED_BYTE;
            case FB_CAS::HighDynamicRange:         return GL_HALF_FLOAT;
            case FB_CAS::LightweightHDR:           return GL_UNSIGNED_INT_2_10_10_10_REV;
            case FB_CAS::SingleChannelFloat16:     return GL_HALF_FLOAT;
            case FB_CAS::SingleChannelFloat32:     return GL_FLOAT;
            case FB_CAS::MultiChannelFloat16:      return GL_HALF_FLOAT;
            case FB_CAS::MultiChannelFloat32:      return GL_FLOAT;
            default:                               return GL_NONE;
        }
    }

    using FB_DAS = FrameBufferDepthAttachmentStandards;
    static GLenum GL_DepthAttachmentFormat(FB_DAS type)
    {
        switch (type)
        {
            case FB_DAS::Standard:
            case FB_DAS::StandardPrecision:        return GL_DEPTH_COMPONENT24;
            case FB_DAS::HighPrecision:            return GL_DEPTH_COMPONENT32F;
            case FB_DAS::CommonCombined:           return GL_DEPTH24_STENCIL8;
            case FB_DAS::HighPrecisionCombined:    return GL_DEPTH32F_STENCIL8;
            default:                               return GL_NONE;
        }
    }

    static GLenum GL_Texture2D_DepthFormat(FB_DAS type)
    {
        switch (type)
        {
            case FB_DAS::Standard:
            case FB_DAS::StandardPrecision:        return GL_DEPTH_COMPONENT;
            case FB_DAS::HighPrecision:            return GL_DEPTH_COMPONENT;
            case FB_DAS::CommonCombined:           return GL_DEPTH_STENCIL;
            case FB_DAS::HighPrecisionCombined:    return GL_DEPTH_STENCIL;
            default:                               return GL_NONE;
        }
    }

    static GLenum GL_Texture2D_DepthType(FB_DAS type)
    {
        switch (type)
        {
            case FB_DAS::Standard:
            case FB_DAS::StandardPrecision:        return GL_UNSIGNED_INT;
            case FB_DAS::HighPrecision:            return GL_FLOAT;
            case FB_DAS::CommonCombined:           return GL_UNSIGNED_INT_24_8;
            case FB_DAS::HighPrecisionCombined:    return GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
            default:                               return GL_NONE;
        }
    }

    static GLenum GetDepthAttachmentPoint(FB_DAS type)
    {
        switch (type)
        {
            case FB_DAS::CommonCombined:
            case FB_DAS::HighPrecisionCombined:    return GL_DEPTH_STENCIL_ATTACHMENT;
            case FB_DAS::Standard:
            case FB_DAS::StandardPrecision:
            case FB_DAS::HighPrecision:            return GL_DEPTH_ATTACHMENT;
            default:                               return GL_NONE;
        }
    }

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

    void GL_CaptureFrameBuffer::BindFrameBuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_CaptureFrameBufferID);
    }

    void GL_CaptureFrameBuffer::UnbindFrameBuffer()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void GL_CaptureFrameBuffer::BindRenderBuffer()
    {
        glBindRenderbuffer(GL_RENDERBUFFER, m_CaptureRenderTextureID);
    }

    void GL_CaptureFrameBuffer::UnbindRenderBuffer()
    {
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

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

    std::shared_ptr<GL_CaptureFrameBuffer> GL_CaptureFrameBuffer::Create(std::int32_t width, std::int32_t height)
    {
        return std::make_shared<GL_CaptureFrameBuffer>(width, height);
    }
}