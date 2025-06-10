#include "CorePCH.hpp"

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

    GLFW_Window::GLFW_Window(WindowHandle windowHandle, const std::string& title, const std::shared_ptr<IContext> context)
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
        m_Properties.Handle = windowHandle;

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
            SetEventsCallBacks();
            RegisterEventsCallBacks();

            if(m_Context != nullptr)
            {
                m_Context->Attach(m_Window);
            }
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

    static void glfwSetKeyCharEvent(GLFWwindow* window, unsigned int codepoint)
    {
        EventRegistry<GLFWwindow*, unsigned int>::Invoke(EventType::KeybaordKeyChar, window, codepoint);
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

    void GLFW_Window::SetEventsCallBacks() 
    {
        glfwSetWindowCloseCallback(m_Window, glfwWindowCloseEvent);
        glfwSetWindowSizeCallback(m_Window, glfwWindowResizeEvent);
        glfwSetWindowFocusCallback(m_Window, glfwWindowFocusEvent);
        glfwSetWindowIconifyCallback(m_Window, glfwWindowIconifyEvent);
        glfwSetWindowMaximizeCallback(m_Window, glfwWindowMaximizeEvent);
        glfwSetWindowPosCallback(m_Window, glfwWindowPosEvent);
        glfwSetFramebufferSizeCallback(m_Window, glfwWindowFrameBufferSizeEvent);
        glfwSetKeyCallback(m_Window, glfwKeyEvent);
        glfwSetCharCallback(m_Window, glfwSetKeyCharEvent);
        glfwSetMouseButtonCallback(m_Window, glfwMouseButtonEvent);
        glfwSetCursorPosCallback(m_Window, glfwMouseCursorPosEvent);
        glfwSetCursorEnterCallback(m_Window, glfwMouseCursorEnterEvent);
        glfwSetScrollCallback(m_Window, glfwMouseScrollEvent);
    }

    void GLFW_Window::RegisterEventsCallBacks() 
    {
        EventRegistry<GLFWwindow*>::Register(EventType::WindowClose, [this](GLFWwindow* window)
        {
            EventWindowClose windowCloseEvent;
            m_CallbackFunc(m_Properties.Handle, windowCloseEvent);
        });

        EventRegistry<GLFWwindow*, int, int>::Register(EventType::WindowResize, [this](GLFWwindow* window, int width, int height)
        {
            EventWindowResize windowResizeEvent(width, height);
            m_Properties.Width = width;
            m_Properties.Height = height;
            m_CallbackFunc(m_Properties.Handle, windowResizeEvent);
        });

        EventRegistry<GLFWwindow*, int>::Register(EventType::WindowFocusGain, [this](GLFWwindow* window, int focused)
        {
            if(focused)
            {
                EventWindowFocusGain windowFocusEvent;
                m_Properties.IsFocused = true;
                m_CallbackFunc(m_Properties.Handle, windowFocusEvent);
            }
            else
            {
                EventWindowFocusLost windowLostFocusEvent;
                m_Properties.IsFocused = false;
                m_CallbackFunc(m_Properties.Handle, windowLostFocusEvent);
            }
        });

        EventRegistry<GLFWwindow*, int>::Register(EventType::WindowMinimize, [this](GLFWwindow* window, int iconified)
        {
            if(iconified)
            {
                EventWindowMinimized windowMinimizeEvent;
                m_Properties.State = WindowState::Minimized;
                m_CallbackFunc(m_Properties.Handle, windowMinimizeEvent);
            }
        });

        EventRegistry<GLFWwindow*, int>::Register(EventType::WindowMaximize, [this](GLFWwindow* window, int maximized)
        {
            if(maximized)
            {
                EventWindowMaximized windowMaximizeEvent;
                m_Properties.State = WindowState::Maximized;
                m_CallbackFunc(m_Properties.Handle, windowMaximizeEvent);
            }
        });

        EventRegistry<GLFWwindow*, int, int>::Register(EventType::WindowPosChange, [this](GLFWwindow* window, int x, int y)
        {
            EventWindowPosChange windowMoveEvent(x, y);
            m_Properties.PosX = x;
            m_Properties.PosY = y;
            m_CallbackFunc(m_Properties.Handle, windowMoveEvent);
        });

        EventRegistry<GLFWwindow*, int, int>::Register(EventType::WindowFrameBufferSizeChange, [this](GLFWwindow* window, int width, int height)
        {
            EventWindowFrameBufferSizeChange windowPixelSizeEvent(width, height);
            m_Properties.PixelWidth = width;
            m_Properties.PixelHeight = height;
            m_CallbackFunc(m_Properties.Handle, windowPixelSizeEvent);
        });

        EventRegistry<GLFWwindow*, int, int, int, int>::Register(EventType::KeyboardKeyPress, [this](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            if(action == KeyState::KEY_PRESSED)
            {
                EventKeyboardKeyPress<KeyCode> keyPressEvent(static_cast<KeyCode>(key));
                m_CallbackFunc(m_Properties.Handle, keyPressEvent);
            }

            if(action == KeyState::KEY_RELEASED)
            {
                EventKeyboardKeyRelease<KeyCode> keyReleaseEvent(static_cast<KeyCode>(key));
                m_CallbackFunc(m_Properties.Handle, keyReleaseEvent);
            }

            if(action == KeyState::KEY_REPEAT)
            {
                EventKeyboardKeyRepeate<KeyCode> keyPressEvent(static_cast<KeyCode>(key));
                m_CallbackFunc(m_Properties.Handle, keyPressEvent);
            }
        });

        EventRegistry<GLFWwindow*, unsigned int>::Register(EventType::KeybaordKeyChar, [this](GLFWwindow* window, unsigned int codepoint)
        {
            EventKeyboardKeyChar keyCharEvent(codepoint);
            m_CallbackFunc(m_Properties.Handle, keyCharEvent);
        });

        EventRegistry<GLFWwindow*, int, int, int>::Register(EventType::MouseButtonDown, [this](GLFWwindow* window, int button, int action, int mods)
        {
            if(action == MouseButtonState::MOUSE_BUTTON_PRESSED)
            {
                EventMouseButtonDown mouseButtonPressEvent(static_cast<MouseButton>(button));
                m_CallbackFunc(m_Properties.Handle, mouseButtonPressEvent);
            }

            if(action == MouseButtonState::MOUSE_BUTTON_RELEASED)
            {
                EventMouseButtonUp mouseButtonReleaseEvent(static_cast<MouseButton>(button));
                m_CallbackFunc(m_Properties.Handle, mouseButtonReleaseEvent);
            }
        });

        EventRegistry<GLFWwindow*, double, double>::Register(EventType::MouseCursorPosChange, [this](GLFWwindow* window, double x, double y)
        {
            EventMouseCursorMove<double> mouseCursorPosEvent(x, y);
            m_CallbackFunc(m_Properties.Handle, mouseCursorPosEvent);
        });

        EventRegistry<GLFWwindow*, int>::Register(EventType::MouseCursorWindowEnter, [this](GLFWwindow* window, int entered)
        {
            if(entered)
            {
                EventMouseCursorWindowEnter mouseCursorEnterEvent;
                m_CallbackFunc(m_Properties.Handle, mouseCursorEnterEvent);
            }
            else
            {
                EventMouseCursorWindowLeave mouseCursorLeaveEvent;
                m_CallbackFunc(m_Properties.Handle, mouseCursorLeaveEvent);
            }
        });

        EventRegistry<GLFWwindow*, double, double>::Register(EventType::MouseWheelScroll, [this](GLFWwindow* window, double x, double y)
        {
            EventMouseWheelScroll<double> mouseWheelScrollEvent(x, y);
            m_CallbackFunc(m_Properties.Handle, mouseWheelScrollEvent);
        });
    }

    void GLFW_Window::PollEvents()
    {
        glfwWaitEvents();
    }

    void GLFW_Window::SwapBuffers()
    {
        if (m_Context != nullptr)
        {
            m_Context->SwapBuffers(m_Window);
        }
    }

    void GLFW_Window::SetContext(const std::shared_ptr<IContext>& context)
    {
        m_Context = context;
        if (m_Context != nullptr)
        {
            m_Context->Attach(m_Window);
        }
    }

    std::shared_ptr<IContext> GLFW_Window::GetContext() const
    {
        return m_Context;
    }

    void GLFW_Window::SetEventsCallbackFunc(const ApplicationCallbackFunction& callbackFunc)
    {
        m_CallbackFunc = callbackFunc;
    }
}