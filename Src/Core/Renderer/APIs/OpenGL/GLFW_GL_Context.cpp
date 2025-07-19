#include "CorePCH.hpp"

namespace Motion::Core
{
    /**
     * @brief Constructs a GLFW_GL_Context object and initializes the OpenGL context.
     *
     * This constructor initializes the OpenGL context using GLAD and sets the context version
     * to 4.6. It also logs the OpenGL version if successful.
     */
    GLFW_GL_Context::GLFW_GL_Context()
    {
        if (!m_IsContextCreated)
        {
            std::int32_t glad_version = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
            if (glad_version == 0)
            {
                MOTION_CORE_CRITICAL("Failed to initialize GLAD");
                return;
            }

            std::int32_t major{ 0 }, minor{ 0 };
            glGetIntegerv(GL_MAJOR_VERSION, &major);
            glGetIntegerv(GL_MINOR_VERSION, &minor);
            MOTION_CORE_INFO("GLAD successfully initialized. OpenGL {0}.{1}", major, minor);


            m_IsContextCreated = glad_version;
        }
    }

    /**
     * @brief Attaches an OpenGL context to the specified native window using GLFW.
     *
     * This function sets the current OpenGL context to the provided native window,
     * enables vertical synchronization (VSync) by setting the swap interval to 1,
     * and initializes the GLAD OpenGL loader if it has not been initialized yet.
     *
     * @param window The native window handle to which the OpenGL context will be attached.
     *               Must not be null.
     *
     * @note This function asserts if the provided window is null or if GLAD fails to load.
     *       GLAD initialization is performed only once per application lifetime.
     */
    void GLFW_GL_Context::Attach(NativeWindow window) noexcept
    {
        MOTION_ASSERT(window, "The Native window is null");
        glfwMakeContextCurrent((GLFWwindow*)window);
    }

    /**
     * @brief Detaches the current OpenGL context from the calling thread.
     *
     * This function makes no OpenGL context current in the calling thread by passing nullptr
     * to glfwMakeContextCurrent. After calling this method, the thread will not have an active
     * OpenGL context until another context is made current.
     *
     * @note This operation is typically used when cleaning up or switching contexts.
     * @see glfwMakeContextCurrent
     */
    void GLFW_GL_Context::Detach() noexcept
    {
        glfwMakeContextCurrent(nullptr);
    }

    /**
     * @brief Checks if the OpenGL context has been successfully created.
     *
     * @return true if the context is created, false otherwise.
     */
    bool GLFW_GL_Context::IsContextCreated() const noexcept
    {
        return m_IsContextCreated;
    }

    /**
     * @brief Retrieves the native window handle of the current OpenGL context.
     *
     * This function returns the native window associated with the currently active
     * OpenGL context managed by GLFW. It is a noexcept function and does not throw exceptions.
     *
     * @return NativeWindow The handle to the current GLFW window context.
     */
    NativeWindow GLFW_GL_Context::GetCurrentContext() const noexcept
    {
        return glfwGetCurrentContext();
    }

    /**
     * @brief Swaps the front and back buffers of the specified native window.
     *
     * This function presents the rendered image to the screen by swapping the front and back buffers
     * of the given native window. It asserts that the provided window handle is valid before proceeding.
     *
     * @param window The native window handle whose buffers are to be swapped.
     * @note This function should be called after rendering is complete for the current frame.
     * @see glfwSwapBuffers
     */
    void GLFW_GL_Context::SwapBuffers(NativeWindow window) noexcept
    {
        MOTION_ASSERT(window, "The Native window is null");
        glfwSwapBuffers((GLFWwindow*)window);
    }
}