#include "CorePCH.hpp"

namespace Motion::Core
{
    void GLFW_GL_Context::Attach(NativeWindow window)
    {
        MOTION_ASSERT(window, "The Native window is null");
        glfwMakeContextCurrent((GLFWwindow*) window);
        glfwSwapInterval(1);

        static bool glad_initialized = false;
        if (!glad_initialized)
        {
            int32_t glLoading = gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
            MOTION_ASSERT(glLoading, "Failed to load OpenGL!");
            m_IsContextCreated = true;
            MOTION_CORE_INFO("GLAD successfully initialized");
        }
    }

    void GLFW_GL_Context::Detach()
    {
        glfwMakeContextCurrent(nullptr);
    }

    bool GLFW_GL_Context::IsContextCreated() const
    {
        return m_IsContextCreated;
    }

    NativeWindow GLFW_GL_Context::GetCurrentContext() const
    {
        return glfwGetCurrentContext();
    }

    void GLFW_GL_Context::SwapBuffers(NativeWindow window)
    {
        MOTION_ASSERT(window, "The Native window is null");
        glfwSwapBuffers((GLFWwindow*) window);
    }
}