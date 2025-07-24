#pragma once

#include <glm/glm.hpp>

#include "Base.hpp"
#include "Window.hpp"
#include "Texture.hpp"

namespace Motion
{
    enum class RenderingAPI : std::uint8_t
    {
        OpenGL = Bits<1>::value,
        Vulkan = Bits<2>::value,
        DirectX = Bits<3>::value
    };

    enum DrawFlags : std::uint8_t
    {
        DepthTest = Bits<1>::value,
        SkipDepthMask = Bits<2>::value,
        Wireframe = Bits<3>::value,
    };

    enum class DepthFunction : GLenum
    {
        Never = Bits<1>::value,
        Less = Bits<2>::value,
        Equal = Bits<3>::value,
        LessEqual = Bits<4>::value,
        Greater = Bits<5>::value,
        NotEqual = Bits<6>::value,
        GreaterEqual = Bits<7>::value,
        Always = Bits<8>::value
    };

    inline std::uint8_t operator|(RenderingAPI a, RenderingAPI b) { return static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b); }
    inline std::uint8_t operator&(RenderingAPI a, RenderingAPI b) { return static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b); }
    inline std::uint8_t operator|(DrawFlags a, DrawFlags b) { return static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b); }
    inline std::uint8_t operator&(DrawFlags a, DrawFlags b) { return static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b); }

    class Renderer
    {
    public:
        Renderer() = default;
        ~Renderer() = default;

        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        Renderer(Renderer&&) = delete;

    public:
        static void Init();
        static void Quit();
        static void Clear();
        static void ResetDrawFlags(DrawFlags flags);
        static void ResetDepthFunction();
        static void ApplyDepthFunction(DepthFunction depthFunction);
        static void ApplyDrawFlags(DrawFlags flags);
        static void ClearColor(const glm::vec4& color);
        static void DrawIndexed(std::int32_t indicesCount);
        static void SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);

        static std::int32_t GetMaxTextureSlots() noexcept;
        static void BindTextureUnit(std::int32_t slot, std::uint32_t textureID);
        static void UnbindTextureUnit(std::int32_t slot);

        [[nodiscard]] static RenderingAPI GetAPI() noexcept;
    };
}

