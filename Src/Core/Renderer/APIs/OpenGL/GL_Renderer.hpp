#pragma once

#include "Renderer.hpp"

namespace Motion::Core
{
    void GL_Init();
    void GL_Quit();
    RenderingAPI GL_GetAPI();
    void GL_Clear();
    void GL_ClearColor(const glm::vec4& color);
    void GL_SetViewport(int32_t x, int32_t y, int32_t width, int32_t height);
    void GL_DrawIndexed(uint32_t indicesCount);
    void GL_ApplyDrawFlags(DrawFlags flags);
    void GL_ResetDrawFlags();
}