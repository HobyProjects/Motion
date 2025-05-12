#pragma once

#include "Renderer.hpp"

namespace Motion::Core
{
    class GL_Renderer 
    {
        private:
            GL_Renderer() = default;
            ~GL_Renderer() = default;
            
            GL_Renderer(const GL_Renderer&) = delete;
            GL_Renderer& operator=(const GL_Renderer&) = delete;
            GL_Renderer& operator=(GL_Renderer&&) = delete;
            GL_Renderer(GL_Renderer&&) = delete;

        public:
            static void Init();
            static void Quit();
            static RenderingAPI GetAPI();
            static void Clear();
            static void ClearColor(const glm::vec4& color);
            static void SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
            static void DrawIndexed(uint32_t indicesCount);
    };
}