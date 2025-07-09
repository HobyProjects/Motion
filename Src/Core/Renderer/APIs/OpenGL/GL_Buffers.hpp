#pragma once

#include "Mesh.hpp"
#include "Buffers.hpp"

namespace Motion::Core
{
    class GL_VertexBuffer final : public IVertexBuffer
    {
        public:
            GL_VertexBuffer() = default;
            GL_VertexBuffer(uint32_t allocatorSize);
            GL_VertexBuffer(float* data, uint32_t dataSize);
            virtual ~GL_VertexBuffer();

            virtual void Bind() const override;
            virtual void Unbind() const override;
            virtual BufferID GetID() const override { return m_VertexBufferID; }
            virtual void SetData(const void* data, uint32_t size) override;
            virtual void SetLayout(const BufferLayout& layout) override;
            virtual const BufferLayout& GetLayout() const override { return m_Layout; }

        private:
            BufferID m_VertexBufferID{ 0 };
            BufferLayout m_Layout;
    };

    class GL_ElementBuffer final : public IElementBuffer
    {
        public:
            GL_ElementBuffer() = default;
            GL_ElementBuffer(uint32_t* data, uint32_t indicesCount);
            virtual ~GL_ElementBuffer();

            virtual void Bind() const override;
            virtual void Unbind() const override;
            virtual BufferID GetID() const override { return m_ElementBufferID; }
            virtual uint32_t GetElementCount() const override { return m_Count; }

        private:
            BufferID m_ElementBufferID{ 0 };
            uint32_t m_Count{ 0 };
    };

    class GL_ShaderBuffer final : public IShaderBuffer
    {
        public:
            GL_ShaderBuffer() = default;
            GL_ShaderBuffer(uint32_t size, BindingPoint binding);
            virtual ~GL_ShaderBuffer();

            virtual void Bind() const override;
            virtual void Unbind() const override;
            virtual BufferID GetID() const override { return m_ShaderBufferID; }
            
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::mat4& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::mat3& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec4& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec3& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec2& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, float data) override;

        private:
            BufferID m_ShaderBufferID{ 0 };
            BindingPoint m_BindingPoint{ 0 };
    };

    class GL_UniformBuffer final : public IUniformBuffer
    {
        public:
            GL_UniformBuffer() = default;
            GL_UniformBuffer(uint32_t size, BindingPoint binding);
            virtual ~GL_UniformBuffer();

            virtual void Bind() const override;
            virtual void Unbind() const override;
            virtual BufferID GetID() const override { return m_UniformBufferID; }
            
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::mat4& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::mat3& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec4& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec3& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec2& data) override;
            virtual void SetBufferData(uint32_t offset, uint32_t size, float data) override;

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

            virtual void BindAttachment() override;
            virtual void UnbindAttachment() override;

            virtual void ResizeFrame(uint32_t width, uint32_t Height) override;
            virtual BufferID GetFrameBufferID() const override { return m_FrameBufferID; }
            virtual BufferID GetColorAttachment() const override { return m_ColorAttachment; }
            virtual FrameBufferSpecification& GetFrameSpecification() override { return m_Specification; }

            virtual bool IsMSAA() const override { return m_Specification.Samples > 1; }
            virtual BufferID GetResolvedFrameBufferID() const override { return m_ResolvedFBOID; }
            virtual BufferID GetResolvedColorAttachment() const override { return m_ResolvedColorAttachment; }
            virtual void Resolve() override;
            virtual void Render() override;

        protected:
            virtual void CreateFrame() override;

        private:
            void DeleteFrameBuffers();

        private:
            BufferID m_FrameBufferID{ 0 };
            TextureID m_ColorAttachment{ 0 };
            TextureID m_DepthAttachment{ 0 };

            // For MSAA resolve
            BufferID m_ResolvedFBOID{ 0 };
            TextureID m_ResolvedColorAttachment{ 0 };
            TextureID m_ResolvedDepthAttachment{ 0 };

            FrameBufferSpecification m_Specification{};
            std::shared_ptr<Mesh> m_PostProcessingQuad{ nullptr };
    };

    std::shared_ptr<GL_VertexBuffer> GL_CreateVertexBuffer(uint32_t alloca_size);
    std::shared_ptr<GL_VertexBuffer> GL_CreateVertexBuffer(float* data, uint32_t size);
    std::shared_ptr<GL_ElementBuffer> GL_CreateElementBuffer(uint32_t* data, uint32_t size);
    std::shared_ptr<GL_ShaderBuffer> GL_CreateShaderBuffer(uint32_t size, BindingPoint binding);
    std::shared_ptr<GL_UniformBuffer> GL_CreateUniformBuffer(uint32_t size, BindingPoint binding);
    std::shared_ptr<GL_FrameBuffer> GL_CreateFrameBuffer(const FrameBufferSpecification& specification);
}