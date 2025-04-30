#pragma once

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
}