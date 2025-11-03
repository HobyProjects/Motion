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

        glGetIntegerv(GL_MAJOR_VERSION, &m_Version.Major);
        glGetIntegerv(GL_MINOR_VERSION, &m_Version.Minor);
        
        const GLubyte* vendor       = glGetString(GL_VENDOR);
        const GLubyte* renderer     = glGetString(GL_RENDERER);
        const GLubyte* version      = glGetString(GL_VERSION);
        const GLubyte* glslVersion  = glGetString(GL_SHADING_LANGUAGE_VERSION);
        
        MOTION_CORE_INFO("-----------------------------------------------");
        MOTION_CORE_INFO(" OpenGL Context Information:");
        MOTION_CORE_INFO("-----------------------------------------------");
        MOTION_CORE_INFO(" Vendor:        {0}", (const char*)vendor);
        MOTION_CORE_INFO(" Renderer:      {0}", (const char*)renderer);
        MOTION_CORE_INFO(" Version:       {0}", (const char*)version);
        MOTION_CORE_INFO(" GLSL Version:  {0}", (const char*)glslVersion);
        MOTION_CORE_INFO("-----------------------------------------------");
        MOTION_CORE_INFO(" OpenGL {0}.{1}", m_Version.Major, m_Version.Minor);
        MOTION_CORE_INFO("-----------------------------------------------");
        
        if (!IsVersionSupported(4, 6))
        {
            MOTION_CORE_ERROR("OpenGL 4.6 is required but only {0}.{1} is available", 
                             m_Version.Major, m_Version.Minor);
            return false;
        }
        
        MOTION_CORE_INFO("OpenGL 4.6 support verified!");
        return true;
    }

    bool GLFW_GL_Context::IsVersionSupported(int major, int minor) const noexcept
    {
        return m_Version.IsAtLeast(major, minor);
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