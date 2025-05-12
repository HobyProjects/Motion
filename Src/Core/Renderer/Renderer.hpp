#pragma once

#include <glm/glm.hpp>

#include "Base.hpp"
#include "DrawCommand.hpp"

namespace Motion::Core
{
    enum class RenderingAPI
    {
        OpenGL = 0,
        Vulkan,
        DirectX
    };

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
            static uint32_t GetDrawCalls();

        private:
            static void DrawIndexed(uint32_t indicesCount);
    };
}

