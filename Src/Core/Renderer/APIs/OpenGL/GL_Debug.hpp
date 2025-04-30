#pragma once

#include <glad/glad.h>

namespace TE::Core
{
    #ifdef MOTION_DEBUG

        void GLAPIENTRY GL_DebugMessageCallBack(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
    
    #endif
}