#pragma once

#include <vector>
#include <initializer_list>
#include <string>
#include <cstdint>
#include <memory>

#include "Base.hpp"

namespace Motion::Core
{
    inline constexpr uint32_t SHADER_BUFFER_DEFAULT_SIZE   = 1024;
    inline constexpr uint32_t SHADER_BUFFER_MAX_SIZE       = 65536; 
    inline constexpr uint32_t SHADER_BUFFER_MAX_BINDING    = 16; 
    inline constexpr uint32_t UNIFORM_BUFFER_MAX_BINDING   = 16;
    inline constexpr uint32_t UNIFORM_BUFFER_MAX_SIZE      = 65536;

    enum class BufferComponents : uint32_t
    {
        X       = 1,
        XY      = 2,
        UV      = 2,
        XYZ     = 3,
        RGB     = 3,
        RGBA    = 4,
        XYZW    = 4,
        MAT3    = 3,
        MAT4    = 4,
        NAN_    = 0,
    };

    enum class BufferStride : uint32_t
    {
        BOOLEAN = sizeof(bool),
        F1      = sizeof(float),
        F2      = sizeof(float) * 2,
        F3      = sizeof(float) * 3,
        F4      = sizeof(float) * 4,
        MAT3    = sizeof(float) * 3 * 3,
        MAT4    = sizeof(float) * 4 * 4,
        NAN_    = 0,
    };

    struct BufferElements
    {
        std::string Name{"Unknown"};
        int32_t Offset{-1};
        BufferComponents Components{BufferComponents::NAN_};
        BufferStride Stride{BufferStride::NAN_};
        bool Normalized{false};

        BufferElements(const std::string& name, BufferComponents components, BufferStride stride, bool normalized, int32_t offset)
            : Name(name), Components(components), Stride(stride), Normalized(normalized), Offset(offset) {}
        ~BufferElements() = default;
    };

    class BufferLayout
    {
        public:
            BufferLayout() = default;
            BufferLayout(const std::initializer_list<BufferElements>& elements) : m_Elements(elements) {}
            ~BufferLayout() = default;

            const std::vector<BufferElements>& GetElements() const { return m_Elements; }
            std::vector<BufferElements>::iterator begin() { return m_Elements.begin(); }
            std::vector<BufferElements>::iterator end() { return m_Elements.end(); }

        private:
            std::vector<BufferElements> m_Elements{};
    };

    using BufferID = uint32_t;
    using BindingPoint = uint32_t;
    using BufferLayoutPtr = std::shared_ptr<BufferLayout>;

    class IVertexBuffer
    {
        public:
            IVertexBuffer() = default;
            virtual ~IVertexBuffer() = default;

            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;
            virtual BufferID GetID() const = 0;

            virtual void SetData(const void* data, uint32_t size) = 0;
            virtual void SetLayout(const BufferLayout& layout) = 0;
            virtual const BufferLayout& GetLayout() const = 0;
    };

    class IElementBuffer
    {
        public:
            IElementBuffer() = default;
            virtual ~IElementBuffer() = default;

            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;
            virtual BufferID GetID() const = 0;
            virtual uint32_t GetElementCount() const = 0;
    };

    class IShaderBuffer
    {
        public:
            IShaderBuffer() = default;
            virtual ~IShaderBuffer() = default;

            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;
            virtual BufferID GetID() const = 0;
            
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::mat4& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::mat3& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec4& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec3& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec2& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, float data) = 0;
    };

    class IUniformBuffer
    {
        public:
            IUniformBuffer() = default;
            virtual ~IUniformBuffer() = default;

            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;
            virtual BufferID GetID() const = 0;
            
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::mat4& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::mat3& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec4& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec3& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, const glm::vec2& data) = 0;
            virtual void SetBufferData(uint32_t offset, uint32_t size, float data) = 0;
    };

    struct FrameBufferSpecification
	{
		uint32_t Width{ 0 };
		uint32_t Height{ 0 };
		bool SwapChainTarget{ false };
        uint32_t Samples{ 1 };
	};
    
    class IFrameBuffer
    {
        public:
            IFrameBuffer() = default;
            virtual ~IFrameBuffer() = default;

            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;

            virtual void ResizeFrame(uint32_t width, uint32_t height) = 0;
            virtual BufferID GetFrameBufferID() const = 0;
            virtual BufferID GetColorAttachment() const = 0;
            virtual FrameBufferSpecification& GetFrameSpecification() = 0;

            virtual bool IsMSAA() const = 0;
            virtual BufferID GetResolvedFBO() const = 0;
            virtual BufferID GetResolvedColorAttachment() const = 0;
            virtual void Resolve() = 0;

        protected:
            virtual void CreateFrame() = 0;
    };

    class BufferFactory
    {
        private:
            BufferFactory() = default;
            ~BufferFactory() = default;

            BufferFactory(const BufferFactory&) = delete;
            BufferFactory& operator=(const BufferFactory&) = delete;
            BufferFactory(BufferFactory&&) = delete;
            BufferFactory& operator=(BufferFactory&&) = delete;

        public:
            static std::shared_ptr<IVertexBuffer> CreateVertexBuffer(uint32_t alloca_size);
            static std::shared_ptr<IVertexBuffer> CreateVertexBuffer(float* data, uint32_t size);
            static std::shared_ptr<IElementBuffer> CreateElementBuffer(uint32_t* data, uint32_t size);
            static std::shared_ptr<IShaderBuffer> CreateShaderBuffer(uint32_t size, BindingPoint binding);
            static std::shared_ptr<IUniformBuffer> CreateUniformBuffer(uint32_t size, BindingPoint binding);
            static std::shared_ptr<IFrameBuffer> CreateFrameBuffer(const FrameBufferSpecification& specification);
    };
}