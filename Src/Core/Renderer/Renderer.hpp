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

    class Renderer
    {
    private:
        Renderer(const Renderer&) = delete;
        Renderer& operator=(const Renderer&) = delete;
        Renderer& operator=(Renderer&&) = delete;
        Renderer(Renderer&&) = delete;

    public:
        Renderer() = default;
        ~Renderer() = default;

        static Renderer& GetInstance();

        void Init();
        void Quit();

        void Clear();
        void ClearColor(const glm::vec4& color);
        void SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);
        void ApplyDrawFlags(DrawFlags flags);
        void ResetDrawFlags();

        void BeginFrame();
        void Submit(const DrawCommand& drawCommand);
        void EndFrame();

        void DrawIndexed(std::uint32_t indicesCount);

        RenderingAPI GetAPI();
        uint32_t GetDrawCalls();

    private:
        void Flush(const DrawCommand& drawCommand);
    };
}

