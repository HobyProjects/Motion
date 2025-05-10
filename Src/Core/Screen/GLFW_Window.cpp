#include "CorePCH.hpp"
#include "GLFW_Window.hpp"

namespace Motion::Core
{
    bool GLFW_BaseAPI::Init()
    {
        if (glfwInit() == GLFW_FALSE)
        {
            return false;
        }

        m_Initialized = true;
        return true;
    }

    void GLFW_BaseAPI::Quit()
    {
        if (m_Initialized)
        {
            glfwTerminate();
            m_Initialized = false;
        }
    }

    GLFW_Window::GLFW_Window(const std::string& title, const std::shared_ptr<IContext> context)
    {
        m_Context = context;
        const GLFWvidmode* mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        if (mode != nullptr)
        {
            m_Properties.Width = mode->width;
            m_Properties.Height = mode->height;
            m_Properties.FixedWidth = mode->width;
            m_Properties.FixedHeight = mode->height;
            m_Properties.MinWidth = 1024;
            m_Properties.MinHeight = 720;
            m_Properties.ColorBits.RedBit = mode->redBits;
            m_Properties.ColorBits.GreenBit = mode->greenBits;
            m_Properties.ColorBits.BlueBit = mode->blueBits;
            m_Properties.ColorBits.AlphaBit = 8;
            m_Properties.ColorBits.DepthStencilBit = 8;
            m_Properties.ColorBits.DepthBit = 24;
            m_Properties.RefreshRate = mode->refreshRate;

        }
        else
        {
            MOTION_CORE_WARN("Failed to get video mode, using default values");
            m_Properties.Width = 1280;
            m_Properties.Height = 720;
            m_Properties.FixedWidth = 0;
            m_Properties.FixedHeight = 0;
            m_Properties.MinWidth = 1024;
            m_Properties.MinHeight = 720;
            m_Properties.ColorBits.RedBit = 8;
            m_Properties.ColorBits.GreenBit = 8;
            m_Properties.ColorBits.BlueBit = 8;
            m_Properties.ColorBits.AlphaBit = 8;
            m_Properties.ColorBits.DepthStencilBit = 8;
            m_Properties.ColorBits.DepthBit = 24;
            m_Properties.RefreshRate = 60;
        }

        m_Properties.Title = title;

        if( Renderer::GetAPI() == RenderingAPI::OpenGL)
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
			glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

			#if defined(TE_DEBUG)
				glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
			#endif
		}

		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		glfwWindowHint(GLFW_RED_BITS, m_Properties.ColorBits.RedBit);
		glfwWindowHint(GLFW_GREEN_BITS, m_Properties.ColorBits.GreenBit);
		glfwWindowHint(GLFW_BLUE_BITS, m_Properties.ColorBits.BlueBit);
		glfwWindowHint(GLFW_ALPHA_BITS, m_Properties.ColorBits.AlphaBit);
		glfwWindowHint(GLFW_REFRESH_RATE, m_Properties.RefreshRate);
		glfwWindowHint(GLFW_DEPTH_BITS, m_Properties.ColorBits.DepthBit);
		glfwWindowHint(GLFW_STENCIL_BITS, m_Properties.ColorBits.DepthStencilBit);

		m_Window = glfwCreateWindow(m_Properties.Width, m_Properties.Height, m_Properties.Title.c_str(), nullptr, nullptr);
		if( m_Window != nullptr )
		{
			MOTION_CORE_INFO("GLFW window created successfully");
			glfwSetWindowSizeLimits(m_Window, m_Properties.MinWidth, m_Properties.MinHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
			glfwGetFramebufferSize(m_Window, &m_Properties.PixelWidth, &m_Properties.PixelHeight);

            if(m_Context != nullptr)
                m_Context->Attach(m_Window);

			m_Properties.IsActive = true;
			m_Properties.IsFocused = glfwGetWindowAttrib(m_Window, GLFW_FOCUSED);
			m_Properties.IsVSyncEnabled = true;
            glfwSetWindowUserPointer(m_Window, this);
		}
		else
		{
			const char* lastError{ nullptr };
			int32_t errorCode = glfwGetError(&lastError);
			MOTION_ASSERT(m_Window, "Failed to create GLFW Window | GLFW Error Code:{0} | Error: {1}", errorCode, lastError);
			return;
		}
    }

    GLFW_Window::~GLFW_Window()
    {
        if( m_Window != nullptr )
        {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }
    }

    static void glfwWindowCloseEvent(GLFWwindow* window)
    {
        EventRegistry<GLFWwindow*>::Invoke(EventType::WindowClose, window);
    }

    static void glfwWindowResizeEvent(GLFWwindow* window, int width, int height)
    {
        EventRegistry<GLFWwindow*, int, int>::Invoke(EventType::WindowResize, window, width, height);
    }

    static void glfwWindowFocusEvent(GLFWwindow* window, int focused)
    {
        EventRegistry<GLFWwindow*, int>::Invoke((focused) ? EventType::WindowFocusGain : EventType::WindowFocusLost, window, focused);
    }

    static void glfwWindowIconifyEvent(GLFWwindow* window, int iconified)
    {
        EventRegistry<GLFWwindow*, int>::Invoke(EventType::WindowMinimize, window, iconified);
    }

    static void glfwWindowMaximizeEvent(GLFWwindow* window, int maximized)
    {
        EventRegistry<GLFWwindow*, int>::Invoke(EventType::WindowMaximize, window, maximized);
    }

    static void glfwWindowPosEvent(GLFWwindow* window, int x, int y)
    {
        EventRegistry<GLFWwindow*, int, int>::Invoke(EventType::WindowPosChange, window, x, y);
    }

    static void glfwWindowFrameBufferSizeEvent(GLFWwindow* window, int width, int height)
    {
        EventRegistry<GLFWwindow*, int, int>::Invoke(EventType::WindowFrameBufferSizeChange, window, width, height);
    }

    static void glfwKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        EventRegistry<GLFWwindow*, int, int, int, int>::Invoke(EventType::KeyboardKeyPress, window, key, scancode, action, mods);
    }

    static void glfwMouseButtonEvent(GLFWwindow* window, int button, int action, int mods)
    {
        EventRegistry<GLFWwindow*, int, int, int>::Invoke(EventType::MouseButtonDown, window, button, action, mods);
    }

    static void glfwMouseCursorPosEvent(GLFWwindow* window, double x, double y)
    {
        EventRegistry<GLFWwindow*, double, double>::Invoke(EventType::MouseCursorPosChange, window, x, y);
    }

    static void glfwMouseCursorEnterEvent(GLFWwindow* window, int entered)
    {
        EventRegistry<GLFWwindow*, int>::Invoke(EventType::MouseCursorWindowEnter, window, entered);
    }

    static void glfwMouseScrollEvent(GLFWwindow* window, double x, double y)
    {
        EventRegistry<GLFWwindow*, double, double>::Invoke(EventType::MouseWheelScroll, window, x, y);
    }




}