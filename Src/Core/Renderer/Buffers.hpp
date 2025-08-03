#pragma once

#include <vector>
#include <initializer_list>
#include <string>
#include <cstdint>
#include <memory>

#include "Base.hpp"

namespace Motion
{
    inline constexpr std::int32_t SHADER_BUFFER_DEFAULT_SIZE = 1024;
    inline constexpr std::int32_t SHADER_BUFFER_MAX_SIZE = 65536;
    inline constexpr std::int32_t SHADER_BUFFER_MAX_BINDING = 16;
    inline constexpr std::int32_t UNIFORM_BUFFER_MAX_BINDING = 16;
    inline constexpr std::int32_t UNIFORM_BUFFER_MAX_SIZE = 65536;

    enum class BufferComponents : std::int32_t
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

    enum class BufferStride : std::int32_t
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

#pragma pack(push, 1)
    struct Vertex
    {
        glm::vec3 Position{ 0.0f, 0.0f, 0.0f };
        glm::vec2 TexCoord{ 0.0f, 0.0f };
        glm::vec3 Normal{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Tangent{ 0.0f, 0.0f, 0.0f };
        glm::vec3 Bitangent{ 0.0f, 0.0f, 0.0f };
    };
#pragma pack(pop)

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
    using FrameTextureID = std::uint32_t;
    using BindingPoint = std::int32_t;
    using BufferLayoutPtr = std::shared_ptr<BufferLayout>;

    class IVertexBuffer
    {
    public:
        IVertexBuffer() = default;
        virtual ~IVertexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual BufferID GetID() const = 0;

        virtual void SetData(const void* data, std::int32_t size) = 0;
        virtual void SetLayout(const BufferLayout& layout) = 0;
        virtual const BufferLayout& GetLayout() const = 0;

        static std::shared_ptr<IVertexBuffer> Create(std::int32_t allocatorSize);
        static std::shared_ptr<IVertexBuffer> Create(Vertex* data, std::uint32_t dataSize);
        static std::shared_ptr<IVertexBuffer> Create(float* data, std::uint32_t dataSize);
    };

    class IElementBuffer
    {
    public:
        IElementBuffer() = default;
        virtual ~IElementBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual BufferID GetID() const = 0;
        virtual std::int32_t GetElementCount() const = 0;

        static std::shared_ptr<IElementBuffer> Create(std::uint32_t* data, std::uint32_t indicesCount);
    };

    class IShaderBuffer
    {
    public:
        IShaderBuffer() = default;
        virtual ~IShaderBuffer() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual BufferID GetID() const = 0;

        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat4& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat3& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec4& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec3& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec2& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, float data) = 0;
        virtual void SetRawBufferData(std::int32_t size, const void* data) = 0;

        static std::shared_ptr<IShaderBuffer> Create(std::int32_t size, BindingPoint binding);
    };

    class IUniformBuffer
    {
    public:
        IUniformBuffer() = default;
        virtual ~IUniformBuffer() = default;

        [[nodiscard]] virtual BufferID GetID() const = 0;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;

        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat4& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::mat3& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec4& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec3& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, const glm::vec2& data) = 0;
        virtual void SetBufferData(std::int32_t offset, std::int32_t size, float data) = 0;
        virtual void SetRawBufferData(std::int32_t size, const void* data) = 0;

        static std::shared_ptr<IUniformBuffer> Create(std::int32_t size, BindingPoint binding);
    };


    enum class FrameBufferBlitMask : std::int32_t
    {
        None = 0,
        Color = 1,
        Depth = 2,
        Stencil = 4,
        All = Color | Depth | Stencil
    };

    inline std::int32_t operator|(FrameBufferBlitMask lhs, FrameBufferBlitMask rhs) { return static_cast<std::int32_t>(lhs) | static_cast<std::int32_t>(rhs); }
    inline std::int32_t operator&(FrameBufferBlitMask lhs, FrameBufferBlitMask rhs) { return static_cast<std::int32_t>(lhs) & static_cast<std::int32_t>(rhs); }

    enum class FrameBufferBlitFilter : std::int32_t
    {
        Nearest = 1,
        Linear = 2
    };

    enum class FrameBufferColorAttachmentStandards : std::int32_t
    {
        None = 0,
        Standard = 1,
        HighDynamicRange = 2,
        LightweightHDR = 3,
        SingleChannelFloat16 = 4,
        SingleChannelFloat32 = 5,
        MultiChannelFloat16 = 6,
        MultiChannelFloat32 = 7,
    };

    enum class FrameBufferDepthAttachmentStandards : std::int32_t
    {
        None = 0,
        Standard = 1,
        StandardPrecision = 2,
        HighPrecision = 3,
        CommonCombined = 4,
        HighPrecisionCombined = 5,
    };

    inline std::int32_t operator|(FrameBufferBlitFilter lhs, FrameBufferBlitFilter rhs) { return static_cast<std::int32_t>(lhs) | static_cast<std::int32_t>(rhs); }
    inline std::int32_t operator&(FrameBufferBlitFilter lhs, FrameBufferBlitFilter rhs) { return static_cast<std::int32_t>(lhs) & static_cast<std::int32_t>(rhs); }
    inline std::int32_t operator|(FrameBufferColorAttachmentStandards lhs, FrameBufferColorAttachmentStandards rhs) { return static_cast<std::int32_t>(lhs) | static_cast<std::int32_t>(rhs); }
    inline std::int32_t operator&(FrameBufferColorAttachmentStandards lhs, FrameBufferColorAttachmentStandards rhs) { return static_cast<std::int32_t>(lhs) & static_cast<std::int32_t>(rhs); }

    struct ColorAttachments
    {
        FrameTextureID ID{ 0 };
        std::int32_t AttachmentPoint{ 0 };
        FrameBufferColorAttachmentStandards Format{ FrameBufferColorAttachmentStandards::Standard };
    };
    struct DepthAttachment
    {
        FrameTextureID ID{ 0 };
        std::int32_t AttachmentPoint{ 0 };
        FrameBufferDepthAttachmentStandards Format{ FrameBufferDepthAttachmentStandards::CommonCombined };
    };

    struct FrameBufferSpecification
    {
        std::string Name;
        std::int32_t Width{ 0 }, Height{ 0 };
        std::int32_t Samples{ 1 };
        bool SwapChainTarget{ false };

        std::vector<ColorAttachments> Colors
        {
            { 0, 0, FrameBufferColorAttachmentStandards::Standard },
            { 0, 1, FrameBufferColorAttachmentStandards::HighDynamicRange },
            { 0, 2, FrameBufferColorAttachmentStandards::SingleChannelFloat16 },
            { 0, 3, FrameBufferColorAttachmentStandards::MultiChannelFloat16 }
        };

        DepthAttachment Depth{ 0, 0, FrameBufferDepthAttachmentStandards::CommonCombined };
    };

    class IFrameBuffer
    {
    public:
        IFrameBuffer() = default;
        virtual ~IFrameBuffer() = default;

        virtual void Bind() = 0;
        virtual void Unbind() = 0;
        virtual void ResizeFrame(std::int32_t width, std::int32_t height) = 0;
        virtual void BlitTo(IFrameBuffer* targetFrameBuffer, FrameBufferBlitMask mask, FrameBufferBlitFilter filter) = 0;

        [[nodiscard]] virtual BufferID GetFrameBufferID() const = 0;
        [[nodiscard]] virtual std::int32_t GetAttachmentCount() const = 0;
        [[nodiscard]] virtual FrameBufferSpecification& GetFrameSpecification() = 0;
        [[nodiscard]] virtual FrameTextureID ResolveTo(IFrameBuffer* target) = 0;
        [[nodiscard]] virtual ColorAttachments GetAttachment(FrameBufferColorAttachmentStandards attachment) const = 0;
        [[nodiscard]] virtual std::int32_t ReadPixel(FrameBufferColorAttachmentStandards attachment, std::int32_t x, std::int32_t y) = 0;

        static std::shared_ptr<IFrameBuffer> Create(const FrameBufferSpecification& specification);
    };

    class ICaptureFrameBuffer
    {
    public:
        ICaptureFrameBuffer() = default;
        virtual ~ICaptureFrameBuffer() = default;

        virtual void BindFrameBuffer() = 0;
        virtual void UnbindFrameBuffer() = 0;
        virtual void BindRenderBuffer() = 0;
        virtual void UnbindRenderBuffer() = 0;

        virtual void ResizeFrame(std::int32_t width, std::int32_t height) = 0;

        [[nodiscard]] virtual BufferID GetCaptureFrameBufferID() const = 0;
        [[nodiscard]] virtual FrameTextureID GetCaptureTextureID() const = 0;
        [[nodiscard]] virtual std::int32_t GetWidth() const = 0;
        [[nodiscard]] virtual std::int32_t GetHeight() const = 0;

        static std::shared_ptr<ICaptureFrameBuffer> Create(std::int32_t width, std::int32_t height);
    };
}