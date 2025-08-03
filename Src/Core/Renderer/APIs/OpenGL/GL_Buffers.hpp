#pragma once

#include "Mesh.hpp"
#include "Buffers.hpp"

namespace Motion
{
    class GL_VertexBuffer final : public IVertexBuffer
    {
    public:
        GL_VertexBuffer() = default;
        GL_VertexBuffer(std::int32_t allocatorSize);
        GL_VertexBuffer(Vertex* data, std::uint32_t dataSize);
        GL_VertexBuffer(float* data, std::uint32_t dataSize);
        virtual ~GL_VertexBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;
        virtual BufferID GetID() const override { return m_VertexBufferID; }
        virtual void SetData(const void* data, std::int32_t size) override;
        virtual void SetLayout(const BufferLayout& layout) override;
        virtual const BufferLayout& GetLayout() const override { return m_Layout; }

        static std::shared_ptr<GL_VertexBuffer> Create(std::int32_t allocatorSize);
        static std::shared_ptr<GL_VertexBuffer> Create(Vertex* data, std::uint32_t dataSize);
        static std::shared_ptr<GL_VertexBuffer> Create(float* data, std::uint32_t dataSize);

    private:

        BufferID m_VertexBufferID{ 0 };
        BufferLayout m_Layout;
    };

    class GL_ElementBuffer final : public IElementBuffer
    {
    public:
        GL_ElementBuffer() = default;
        GL_ElementBuffer(std::uint32_t* data, std::uint32_t indicesCount);
        virtual ~GL_ElementBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;
        virtual BufferID GetID() const override { return m_ElementBufferID; }
        virtual std::int32_t GetElementCount() const override { return m_Count; }

        static std::shared_ptr<GL_ElementBuffer> Create(std::uint32_t* data, std::uint32_t indicesCount);

    private:
        BufferID m_ElementBufferID{ 0 };
        std::int32_t m_Count{ 0 };
    };

    class GL_ShaderBuffer final : public IShaderBuffer
    {
    public:
        GL_ShaderBuffer() = default;
        GL_ShaderBuffer(std::int32_t size, BindingPoint binding);
        virtual ~GL_ShaderBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;
        virtual BufferID GetID() const override { return m_ShaderBufferID; }

        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat4& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat3& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec4& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec3& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec2& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, float data) override;
        virtual void SetRawBufferData(std::int32_t size, const void* data) override;

        static std::shared_ptr<GL_ShaderBuffer> Create(std::int32_t size, BindingPoint binding);

    private:
        BufferID m_ShaderBufferID{ 0 };
        BindingPoint m_BindingPoint{ 0 };
    };

    class GL_UniformBuffer final : public IUniformBuffer
    {
    public:
        GL_UniformBuffer() = default;
        GL_UniformBuffer(std::int32_t size, BindingPoint binding);
        virtual ~GL_UniformBuffer();

        virtual void Bind() const override;
        virtual void Unbind() const override;
        virtual BufferID GetID() const override { return m_UniformBufferID; }

        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat4& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat3& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec4& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec3& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec2& data) override;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, float data) override;
        virtual void SetRawBufferData(std::int32_t size, const void* data) override;

        static std::shared_ptr<GL_UniformBuffer> Create(std::int32_t size, BindingPoint binding);

    private:
        BufferID m_UniformBufferID{ 0 };
        BindingPoint m_BindingPoint{ 0 };
    };

    class GL_FrameBuffer final : public IFrameBuffer
    {
    public:
        GL_FrameBuffer() = default;
        GL_FrameBuffer(const FrameBufferSpecification& specification);
        virtual ~GL_FrameBuffer();

        virtual void Bind() override;
        virtual void Unbind() override;
        virtual void ResizeFrame(std::int32_t width, std::int32_t height) override;
        virtual void BlitTo(IFrameBuffer* targetFrameBuffer, FrameBufferBlitMask mask, FrameBufferBlitFilter filter) override;

        [[nodiscard]] virtual BufferID GetFrameBufferID() const override { return m_FrameBufferID; }
        [[nodiscard]] virtual std::int32_t GetAttachmentCount() const override { return static_cast<std::int32_t>(m_ColorAttachments.size()); }
        [[nodiscard]] virtual FrameBufferSpecification& GetFrameSpecification() override { return m_Specification; }
        [[nodiscard]] virtual FrameTextureID ResolveTo(IFrameBuffer* target) override;
        [[nodiscard]] virtual ColorAttachments GetAttachment(FrameBufferColorAttachmentStandards attachment) const override;
        [[nodiscard]] virtual std::int32_t ReadPixel(FrameBufferColorAttachmentStandards attachment, std::int32_t x, std::int32_t y) override;

        static std::shared_ptr<GL_FrameBuffer> Create(const FrameBufferSpecification& specification);

    private:
        void Invalidate();

    private:
        FrameBufferSpecification m_Specification{};
        std::unordered_map<std::int32_t, ColorAttachments> m_ColorAttachments;
        DepthAttachment m_DepthAttachment{ 0 };
        BufferID m_FrameBufferID{ 0 };
    };

    class GL_CaptureFrameBuffer final : public ICaptureFrameBuffer
    {
    public:
        GL_CaptureFrameBuffer(std::int32_t width, std::int32_t height);
        virtual ~GL_CaptureFrameBuffer() = default;

        virtual void BindFrameBuffer() override;
        virtual void UnbindFrameBuffer() override;
        virtual void BindRenderBuffer() override;
        virtual void UnbindRenderBuffer() override;
        virtual void ResizeFrame(std::int32_t width, std::int32_t height) override;

        [[nodiscard]] virtual BufferID GetCaptureFrameBufferID() const override { return m_CaptureFrameBufferID; }
        [[nodiscard]] virtual FrameTextureID GetCaptureTextureID() const override { return m_CaptureTextureID; }
        [[nodiscard]] virtual std::int32_t GetWidth() const override { return m_Width; }
        [[nodiscard]] virtual std::int32_t GetHeight() const override { return m_Height; }

        static std::shared_ptr<GL_CaptureFrameBuffer> Create(std::int32_t width, std::int32_t height);

    private:
        BufferID m_CaptureFrameBufferID{ 0 };
        FrameTextureID m_CaptureTextureID{ 0 };
        FrameTextureID m_CaptureRenderTextureID{ 0 };
        std::int32_t m_Width{ 0 };
        std::int32_t m_Height{ 0 };
    };

}