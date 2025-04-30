#pragma once

#include <glad/glad.h>

namespace Motion::Core
{
    void GLAPIENTRY GL_MessageCallBack(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);
}