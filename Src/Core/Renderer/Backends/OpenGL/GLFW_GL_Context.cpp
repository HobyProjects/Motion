#include "CorePCH.hpp"

namespace Motion
{
    bool GLFW_GL_Context::Create() noexcept
    {
        std::int32_t glad = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
        if (!glad)
        {
            MOTION_CORE_CRITICAL("Failed to initialize GLAD");
            return false;
        }

        std::int32_t major{ 0 }, minor{ 0 };
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        MOTION_CORE_INFO("GLAD successfully initialized. OpenGL {0}.{1}", major, minor);

        return true;
    }

    void GLFW_GL_Context::MakeCurrent(NativeWindow window) noexcept
    {
        MOTION_ASSERT(window, "The Native window is null");
        glfwMakeContextCurrent((GLFWwindow*)window);
    }

    void GLFW_GL_Context::ClearCurrent() noexcept
    {
        glfwMakeContextCurrent(nullptr);
    }

    void GLFW_GL_Context::SwapBuffers(NativeWindow window) noexcept
    {
        MOTION_ASSERT(window, "The Native window is null");
        glfwSwapBuffers((GLFWwindow*)window);
    }
}