#pragma once

#include "Window.hpp"

namespace Motion::Core
{
    class GLFW_GL_Context final : public IContext
    {
    public:
        GLFW_GL_Context();
        virtual ~GLFW_GL_Context() = default;

        virtual void Attach(NativeWindow window) noexcept override;
        virtual void Detach() noexcept override;
        virtual void SwapBuffers(NativeWindow window) noexcept override;

        [[nodiscard]] virtual bool IsContextCreated() const noexcept override;
        [[nodiscard]] virtual NativeWindow GetCurrentContext() const noexcept override;

    private:
        bool m_IsContextCreated{ false };
    };
}