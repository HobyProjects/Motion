#pragma once

#include "Window.hpp"

namespace Motion
{
    class GLFW_GL_Context final : public IContext
    {
        public:
            GLFW_GL_Context() = default;
            virtual ~GLFW_GL_Context() = default;

            virtual bool Create() noexcept override;
            virtual void MakeCurrent(NativeWindow) noexcept override;
            virtual void ClearCurrent() noexcept override;
            virtual void SwapBuffers(NativeWindow) noexcept override;
    };
}