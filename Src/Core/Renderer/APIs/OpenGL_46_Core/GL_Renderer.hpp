#pragma once
#include "Renderer.hpp"

namespace Motion 
{
    void GL_Init();
    void GL_Quit();
    void GL_Clear();
    void GL_ClearColor(const glm::vec4& color);
    void GL_ClearEx(const ClearParams& p);

    void GL_SetViewport(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);

    void GL_DrawIndexed(std::int32_t indicesCount);      
    void GL_DrawIndexed(const DrawIndexedArgs& args); 

    void GL_BindTextureUnit(std::int32_t slot, std::uint32_t textureID);
    void GL_UnbindTextureUnit(std::int32_t slot);
    std::int32_t GL_GetMaxTextureSlots() noexcept;

    void GL_PushDebugGroup(const char* label);
    void GL_PopDebugGroup();

    void GL_QueryCaps(GpuCaps& outCaps);
} 
