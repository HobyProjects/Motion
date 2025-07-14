#pragma once

#include <glm/glm.hpp>

#include "Base.hpp"
#include "DrawCommand.hpp"
#include "Window.hpp"

namespace Motion::Core
{
    enum class RenderingAPI : uint32_t
    {
        OpenGL = Bits<1>::value,
        Vulkan = Bits<2>::value,
        DirectX = Bits<3>::value
    };

    inline uint32_t operator|(RenderingAPI a, RenderingAPI b) { return static_cast<uint32_t>(a) | static_cast<uint32_t>(b); }
    inline uint32_t operator&(RenderingAPI a, RenderingAPI b) { return static_cast<uint32_t>(a) & static_cast<uint32_t>(b); }
    inline uint32_t operator^(RenderingAPI a, RenderingAPI b) { return static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b); }
    inline uint32_t operator~(RenderingAPI a) { return ~static_cast<uint32_t>(a); }

    class Renderer
    {
    private:
        Renderer() = default;
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        Renderer(Renderer&&) = delete;
        ~Renderer() = default;

    public:
        static void Init();
        static void Quit();
        static RenderingAPI GetAPI();
        static void Clear();
        static void ClearColor(const glm::vec4& color);
        static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
        static void Submit(const DrawCommand& drawCommand);
        static void Flush();
        static void BeginFrame();
        static void EndFrame();
        static void DrawIndexed(uint32_t indicesCount);
        static uint32_t GetDrawCalls();
    };
}

