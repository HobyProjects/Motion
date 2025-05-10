#pragma once

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <memory>

#include "Window.hpp"

namespace Motion::Core
{
    class GLFW_GL_Context final : public IContext
    {
        public:
            GLFW_GL_Context() = default;
            virtual ~GLFW_GL_Context() = default;

            virtual void Attach(NativeWindow window) override;
            virtual void Detach() override;

            virtual bool IsContextCreated() const override;
            virtual NativeWindow GetCurrentContext() const override;
            virtual void SwapBuffers(NativeWindow window) override;

        private:
            bool m_IsContextCreated{ false };
    };
}