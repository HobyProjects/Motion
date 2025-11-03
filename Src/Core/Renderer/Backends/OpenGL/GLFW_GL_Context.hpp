#pragma once

#include "Window.hpp"

namespace Motion
{
    struct OpenGLVersion
    {
        int Major{ 0 };
        int Minor{ 0 };
        
        bool IsAtLeast(int major, int minor) const noexcept
        {
            return (Major > major) || (Major == major && Minor >= minor);
        }
        
        std::string ToString() const noexcept
        {
            return std::to_string(Major) + "." + std::to_string(Minor);
        }
    };

    class GLFW_GL_Context final : public IContext
    {
        public:
            GLFW_GL_Context() = default;
            virtual ~GLFW_GL_Context() = default;

            virtual bool Create() noexcept override;
            virtual void MakeCurrent(NativeWindow) noexcept override;
            virtual void ClearCurrent() noexcept override;
            virtual void SwapBuffers(NativeWindow) noexcept override;
            virtual bool IsVersionSupported(int major, int minor) const noexcept override;
            [[nodiscard]] OpenGLVersion GetVersion() const noexcept { return m_Version; }
            
        private:
            OpenGLVersion m_Version{};
    };
}