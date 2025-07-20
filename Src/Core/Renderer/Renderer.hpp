#pragma once

#include <glm/glm.hpp>

#include "Base.hpp"
#include "Window.hpp"
#include "Texture.hpp"

namespace Motion::Core
{
    enum class RenderingAPI : std::uint8_t
    {
        OpenGL = Bits<1>::value,
        Vulkan = Bits<2>::value,
        DirectX = Bits<3>::value
    };

    enum DrawFlags : std::uint8_t
    {
        None = 0,
        SkipDepthMask = Bits<1>::value,
        Wireframe = Bits<2>::value,
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
        static void ResetDrawFlags();
        static void ApplyDrawFlags(DrawFlags flags);
        static void ClearColor(const glm::vec4& color);
        static void DrawIndexed(std::uint32_t indicesCount);
        static void SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);

        static std::int32_t GetMaxTextureSlots() noexcept;
        static void BindTextureUnit(std::uint32_t slot, TextureID textureID);
        static void UnbindTextureUnit();

        [[nodiscard]] static RenderingAPI GetAPI() noexcept;
    };
}

