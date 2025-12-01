#include "CorePCH.hpp"

namespace Motion
{
    bool GLFW_BaseAPI::Init() noexcept
    {
        if (glfwInit() == GLFW_FALSE)
        {
            return false;
        }

        MOTION_CORE_INFO("GLFW initialized successfully. VERSION: {0}", glfwGetVersionString());
        m_Initialized = true;
        return true;
    }

    void GLFW_BaseAPI::Quit() noexcept
    {
        if (m_Initialized)
        {
            glfwTerminate();
            m_Initialized = false;
        }
    }

    GLFW_Window::GLFW_Window(WindowHandle windowHandle, const std::string& title, bool isVisible, NativeWindow sharedWindow)
    {
        if(isVisible)
        {
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
        }
        else
        {
            m_Properties.Width = 1;
            m_Properties.Height = 1;
            m_Properties.FixedWidth = 0;
            m_Properties.FixedHeight = 0;
            m_Properties.MinWidth = 1;
            m_Properties.MinHeight = 1;
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

        if (Renderer::GetAPI() & RenderingAPI::OpenGL)
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
            glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);

#if defined(MOTION_BUILD_DEBUG)
            glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
        }

        if(isVisible)
        {
            glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        }
        else
        {
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        }

        glfwWindowHint(GLFW_RED_BITS, m_Properties.ColorBits.RedBit);
        glfwWindowHint(GLFW_GREEN_BITS, m_Properties.ColorBits.GreenBit);
        glfwWindowHint(GLFW_BLUE_BITS, m_Properties.ColorBits.BlueBit);
        glfwWindowHint(GLFW_ALPHA_BITS, m_Properties.ColorBits.AlphaBit);
        glfwWindowHint(GLFW_REFRESH_RATE, m_Properties.RefreshRate);
        glfwWindowHint(GLFW_DEPTH_BITS, m_Properties.ColorBits.DepthBit);
        glfwWindowHint(GLFW_STENCIL_BITS, m_Properties.ColorBits.DepthStencilBit);

        m_Window = glfwCreateWindow(m_Properties.Width, m_Properties.Height, m_Properties.Title.c_str(), nullptr, (GLFWwindow*)sharedWindow);
        if (m_Window != nullptr)
        {
            glfwSetWindowSizeLimits(m_Window, m_Properties.MinWidth, m_Properties.MinHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
            glfwGetFramebufferSize(m_Window, &m_Properties.PixelWidth, &m_Properties.PixelHeight);

            m_Properties.IsActive = true;
            m_Properties.IsFocused = glfwGetWindowAttrib(m_Window, GLFW_FOCUSED);
            m_Properties.IsVSyncEnabled = true;

            std::int32_t width{0}, height{0}, channels{0};
            std::uint8_t* pixels = stbi_load("Assets/Icon/MotionEngine.png", &width, &height, &channels, 4); 
            if(pixels)
            {
                GLFWimage image[1];
                image[0].width = width;
                image[0].height = height;
                image[0].pixels = pixels;

                glfwSetWindowIcon(m_Window, 1, image);
                stbi_image_free(pixels);
            }
            else
            {
                MOTION_CORE_ERROR("Unable to load the window Icon");
            }

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
        glfwDestroyWindow(m_Window);
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
        EventRegistry<GLFWwindow*, unsigned int>::Invoke(EventType::KeyboardKeyChar, window, codepoint);
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
        EventRegistry<GLFWwindow*>::Register(EventType::WindowClose,
            [this]([[maybe_unused]] GLFWwindow* window)
            {
                EventWindowClose windowCloseEvent;
                m_CallbackFunc(m_Properties.Handle, windowCloseEvent);
            }
        );

        EventRegistry<GLFWwindow*, int, int>::Register(EventType::WindowResize,
            [this]([[maybe_unused]] GLFWwindow* window, int width, int height)
            {
                EventWindowResize windowResizeEvent(width, height);
                m_Properties.Width = width;
                m_Properties.Height = height;
                m_CallbackFunc(m_Properties.Handle, windowResizeEvent);
            }
        );

        EventRegistry<GLFWwindow*, int>::Register(EventType::WindowFocusGain,
            [this]([[maybe_unused]] GLFWwindow* window, int focused)
            {
                if (focused)
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
            }
        );

        EventRegistry<GLFWwindow*, int>::Register(EventType::WindowMinimize,
            [this]([[maybe_unused]] GLFWwindow* window, int iconified)
            {
                if (iconified)
                {
                    EventWindowMinimized windowMinimizeEvent;
                    m_Properties.State = WindowState::Minimized;
                    m_CallbackFunc(m_Properties.Handle, windowMinimizeEvent);
                }
            }
        );

        EventRegistry<GLFWwindow*, int>::Register(EventType::WindowMaximize,
            [this]([[maybe_unused]] GLFWwindow* window, int maximized)
            {
                if (maximized)
                {
                    EventWindowMaximized windowMaximizeEvent;
                    m_Properties.State = WindowState::Maximized;
                    m_CallbackFunc(m_Properties.Handle, windowMaximizeEvent);
                }
            }
        );

        EventRegistry<GLFWwindow*, int, int>::Register(EventType::WindowPosChange,
            [this]([[maybe_unused]] GLFWwindow* window, int x, int y)
            {
                EventWindowPosChange windowMoveEvent(x, y);
                m_Properties.PosX = x;
                m_Properties.PosY = y;
                m_CallbackFunc(m_Properties.Handle, windowMoveEvent);
            }
        );

        EventRegistry<GLFWwindow*, int, int>::Register(EventType::WindowFrameBufferSizeChange,
            [this]([[maybe_unused]] GLFWwindow* window, int width, int height)
            {
                EventWindowFrameBufferSizeChange windowPixelSizeEvent(width, height);
                m_Properties.PixelWidth = width;
                m_Properties.PixelHeight = height;
                m_CallbackFunc(m_Properties.Handle, windowPixelSizeEvent);
            }
        );

        EventRegistry<GLFWwindow*, int, int, int, int>::Register(EventType::KeyboardKeyPress,
            [this]([[maybe_unused]] GLFWwindow* window, int key, int scancode, int action, int mods)
            {
                if (action == KeyState::KEY_PRESSED)
                {
                    EventKeyboardKeyPress keyPressEvent(static_cast<KeyCode>(key));
                    m_CallbackFunc(m_Properties.Handle, keyPressEvent);
                }

                if (action == KeyState::KEY_RELEASED)
                {
                    EventKeyboardKeyRelease keyReleaseEvent(static_cast<KeyCode>(key));
                    m_CallbackFunc(m_Properties.Handle, keyReleaseEvent);
                }

                if (action == KeyState::KEY_REPEAT)
                {
                    EventKeyboardKeyRepeat keyPressEvent(static_cast<KeyCode>(key));
                    m_CallbackFunc(m_Properties.Handle, keyPressEvent);
                }
            }
        );

        EventRegistry<GLFWwindow*, unsigned int>::Register(EventType::KeyboardKeyChar,
            [this]([[maybe_unused]] GLFWwindow* window, unsigned int codepoint)
            {
                EventKeyboardKeyChar keyCharEvent(codepoint);
                m_CallbackFunc(m_Properties.Handle, keyCharEvent);
            }
        );

        EventRegistry<GLFWwindow*, int, int, int>::Register(EventType::MouseButtonDown,
            [this]([[maybe_unused]] GLFWwindow* window, int button, int action, int mods)
            {
                if (action == MouseButtonState::MOUSE_BUTTON_PRESSED)
                {
                    EventMouseButtonDown mouseButtonPressEvent(static_cast<MouseButton>(button));
                    m_CallbackFunc(m_Properties.Handle, mouseButtonPressEvent);
                }

                if (action == MouseButtonState::MOUSE_BUTTON_RELEASED)
                {
                    EventMouseButtonUp mouseButtonReleaseEvent(static_cast<MouseButton>(button));
                    m_CallbackFunc(m_Properties.Handle, mouseButtonReleaseEvent);
                }
            }
        );

        EventRegistry<GLFWwindow*, double, double>::Register(EventType::MouseCursorPosChange,
            [this]([[maybe_unused]] GLFWwindow* window, double x, double y)
            {
                EventMouseCursorMove mouseCursorPosEvent(x, y);
                m_CallbackFunc(m_Properties.Handle, mouseCursorPosEvent);
            }
        );

        EventRegistry<GLFWwindow*, int>::Register(EventType::MouseCursorWindowEnter,
            [this]([[maybe_unused]] GLFWwindow* window, int entered)
            {
                if (entered)
                {
                    EventMouseCursorWindowEnter mouseCursorEnterEvent;
                    m_CallbackFunc(m_Properties.Handle, mouseCursorEnterEvent);
                }
                else
                {
                    EventMouseCursorWindowLeave mouseCursorLeaveEvent;
                    m_CallbackFunc(m_Properties.Handle, mouseCursorLeaveEvent);
                }
            }
        );

        EventRegistry<GLFWwindow*, double, double>::Register(EventType::MouseWheelScroll,
            [this]([[maybe_unused]] GLFWwindow* window, double x, double y)
            {
                EventMouseWheelScroll mouseWheelScrollEvent(x, y);
                m_CallbackFunc(m_Properties.Handle, mouseWheelScrollEvent);
            }
        );
    }

    void GLFW_Window::PollEvents() noexcept
    {
        glfwPollEvents();
    }

    void GLFW_Window::SetEventsCallbackFunc(const EventProcessingFunction& callbackFunc) noexcept
    {
        m_CallbackFunc = callbackFunc;

        if(m_CallbackFunc)
        {
            SetEventsCallBacks();
            RegisterEventsCallBacks();
        }
    }
}