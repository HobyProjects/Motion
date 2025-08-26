#pragma once

#include <glm/glm.hpp>

#include "Base.hpp"
#include "Window.hpp"
#include "Texture.hpp"

namespace Motion
{
    enum class RenderingAPI : std::uint32_t
    {
        OpenGL      = MOTION_BIT(1),
        Vulkan      = MOTION_BIT(2),
        DirectX     = MOTION_BIT(3)
    };

    inline std::uint32_t operator|(RenderingAPI a, RenderingAPI b) { return static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b); }
    inline std::uint32_t operator&(RenderingAPI a, RenderingAPI b) { return static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b); }

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

        static void ClearColor(const glm::vec4& color);
        static void DrawIndexed(std::int32_t indicesCount);
        static void SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);

        static std::int32_t GetMaxTextureSlots() noexcept;
        static void BindTextureUnit(std::int32_t slot, std::uint32_t textureID);
        static void UnbindTextureUnit(std::int32_t slot);

        [[nodiscard]] static RenderingAPI GetAPI() noexcept;
    };
}

