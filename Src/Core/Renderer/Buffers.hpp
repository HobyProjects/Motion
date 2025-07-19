#pragma once

#include <vector>
#include <initializer_list>
#include <string>
#include <cstdint>
#include <memory>

#include "Base.hpp"

namespace Motion::Core
{
    inline constexpr std::uint32_t SHADER_BUFFER_DEFAULT_SIZE = 1024;
    inline constexpr std::uint32_t SHADER_BUFFER_MAX_SIZE = 65536;
    inline constexpr std::uint32_t SHADER_BUFFER_MAX_BINDING = 16;
    inline constexpr std::uint32_t UNIFORM_BUFFER_MAX_BINDING = 16;
    inline constexpr std::uint32_t UNIFORM_BUFFER_MAX_SIZE = 65536;

    enum class BufferComponents : std::uint32_t
    {
        X = 1,
        XY = 2,
        UV = 2,
        XYZ = 3,
        RGB = 3,
        RGBA = 4,
        XYZW = 4,
        MAT3 = 3,
        MAT4 = 4,
        NAN_ = 0,
    };

    enum class BufferStride : std::uint32_t
    {
        BOOLEAN = sizeof(bool),
        F1 = sizeof(float),
        F2 = sizeof(float) * 2,
        F3 = sizeof(float) * 3,
        F4 = sizeof(float) * 4,
        MAT3 = sizeof(float) * 3 * 3,
        MAT4 = sizeof(float) * 4 * 4,
        NAN_ = 0,
    };

    struct BufferElements
    {
        std::string_view AttributeName;
        int32_t Offset{ -1 };
        BufferComponents Components{ BufferComponents::NAN_ };
        BufferStride Stride{ BufferStride::NAN_ };
        bool Normalized{ false };

        BufferElements(const std::string_view attributeName, BufferComponents components, BufferStride stride, bool normalized, int32_t offset)
            : AttributeName(attributeName), Components(components), Stride(stride), Normalized(normalized), Offset(offset) {
        }
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

    using BufferID = std::uint32_t;
    using BindingPoint = std::uint32_t;
    using BufferLayoutPtr = std::shared_ptr<BufferLayout>;

    class IVertexBuffer
    {
    public:
        IVertexBuffer() = default;
        virtual ~IVertexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual BufferID GetID() const = 0;

        virtual void SetData(const void* data, std::uint32_t size) = 0;
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
        virtual std::uint32_t GetElementCount() const = 0;
    };

    class IShaderBuffer
    {
    public:
        IShaderBuffer() = default;
        virtual ~IShaderBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual BufferID GetID() const = 0;

        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::mat4& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::mat3& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec4& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec3& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec2& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, float data) = 0;
    };

    class IUniformBuffer
    {
    public:
        IUniformBuffer() = default;
        virtual ~IUniformBuffer() = default;

        [[nodiscard]] virtual BufferID GetID() const = 0;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::mat4& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::mat3& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec4& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec3& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, const glm::vec2& data) = 0;
        virtual void SetBufferData(std::uint32_t offset, std::uint32_t size, float data) = 0;
    };


    enum class FrameBufferBlitMask : std::uint32_t
    {
        None = 0,
        Color = 1,
        Depth = 2,
        Stencil = 4,
        All = Color | Depth | Stencil
    };

    inline std::uint32_t operator|(FrameBufferBlitMask lhs, FrameBufferBlitMask rhs) { return static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs); }
    inline std::uint32_t operator&(FrameBufferBlitMask lhs, FrameBufferBlitMask rhs) { return static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs); }

    enum class FrameBufferBlitFilter : std::uint32_t
    {
        Nearest = 1,
        Linear = 2
    };

    enum class FrameBufferColorAttachments : std::uint32_t
    {
        None = 0,
        Standard = 1,
        HighDynamicRange = 2,
        LightweightHDR = 3,
        SingleChannelFloat16 = 4,
        SingleChannelFloat32 = 5,
        MultiChannelFloat16 = 6,
        MultiChannelFloat32 = 7,
        ToneMapped = 8,
    };

    enum class FrameBufferDepthAttachments : std::uint32_t
    {
        None = 0,
        Standard = 1,
        StandardPrecision = 2,
        HighPrecision = 3,
        CommonCombined = 4,
        HighPrecisionCombined = 5,
    };


    inline std::uint32_t operator|(FrameBufferBlitFilter lhs, FrameBufferBlitFilter rhs) { return static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs); }
    inline std::uint32_t operator&(FrameBufferBlitFilter lhs, FrameBufferBlitFilter rhs) { return static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs); }
    inline std::uint32_t operator|(FrameBufferColorAttachments lhs, FrameBufferColorAttachments rhs) { return static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs); }
    inline std::uint32_t operator&(FrameBufferColorAttachments lhs, FrameBufferColorAttachments rhs) { return static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs); }

    struct ColorAttachments
    {
        FrameTextureID TextureID{ 0 };
        std::uint32_t AttachmentPoint{ 0 };
        FrameBufferColorAttachments Format{ FrameBufferColorAttachments::Standard };
    };
    struct DepthAttachment
    {
        FrameTextureID TextureID{ 0 };
        std::uint32_t AttachmentPoint{ 0 };
        FrameBufferDepthAttachments Format{ FrameBufferDepthAttachments::CommonCombined };
    };

    struct FrameBufferSpecification
    {
        std::string Name;
        std::uint32_t Width{ 0 }, Height{ 0 };
        std::uint32_t Samples{ 1 };
        bool SwapChainTarget{ false };

        std::vector<ColorAttachments> Colors
        {
            { 0, 0, FrameBufferColorAttachments::Standard },
            { 0, 1, FrameBufferColorAttachments::HighDynamicRange },
            { 0, 2, FrameBufferColorAttachments::SingleChannelFloat16 },
            { 0, 3, FrameBufferColorAttachments::MultiChannelFloat16 }
        };

        DepthAttachment Depth{ 0, 0, FrameBufferDepthAttachments::CommonCombined };
    };

    class IFrameBuffer
    {
    public:
        IFrameBuffer() = default;
        virtual ~IFrameBuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual void BindTextureUnit(std::uint32_t slot, FrameTextureID textureID) = 0;
        virtual void UnbindTextureUnit() = 0;

        virtual void ResizeFrame(std::uint32_t width, std::uint32_t height) = 0;
        virtual void BlitTo(IFrameBuffer* targetFrameBuffer, FrameBufferBlitMask mask, FrameBufferBlitFilter filter) = 0;

        [[nodiscard]] virtual BufferID GetFrameBufferID() const = 0;
        [[nodiscard]] virtual std::uint32_t GetAttachmentCount() const = 0;
        [[nodiscard]] virtual FrameBufferSpecification& GetFrameSpecification() = 0;
        [[nodiscard]] virtual FrameTextureID ResolveTo(IFrameBuffer* target) = 0;
        [[nodiscard]] virtual ColorAttachments GetAttachment(FrameBufferColorAttachments attachment) const = 0;
        [[nodiscard]] virtual std::int32_t ReadPixel(FrameBufferColorAttachments attachment, std::int32_t x, std::int32_t y) = 0;
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
        static std::shared_ptr<IVertexBuffer> CreateVertexBuffer(std::uint32_t allocatorSize);
        static std::shared_ptr<IVertexBuffer> CreateVertexBuffer(float* data, std::uint32_t size);
        static std::shared_ptr<IElementBuffer> CreateElementBuffer(std::uint32_t* data, std::uint32_t size);
        static std::shared_ptr<IShaderBuffer> CreateShaderBuffer(std::uint32_t size, BindingPoint binding);
        static std::shared_ptr<IUniformBuffer> CreateUniformBuffer(std::uint32_t size, BindingPoint binding);
        static std::shared_ptr<IFrameBuffer> CreateFrameBuffer(const FrameBufferSpecification& specification);
    };
}