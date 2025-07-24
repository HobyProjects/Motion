#include "CorePCH.hpp"

namespace Motion
{
    /**
     * Initializes the GLFW library for use within the application.
     *
     * This function attempts to initialize the GLFW library. If successful, it logs
     * the GLFW version and sets the initialization flag to true.
     *
     * @return true if GLFW is initialized successfully; false if initialization fails.
     *
     * This function is essential as it sets up the necessary environment for
     * creating windows and handling input using GLFW. Make sure to call this
     * function before using any other GLFW-related functionalities.
     */
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

    /**
     * Terminates the GLFW library and releases any allocated resources.
     *
     * This function must be called before the application terminates. It is
     * important to call this function after all windows have been destroyed
     * using DestroyWindow to ensure that all resources are properly released.
     *
     * @return None.
     *
     * This function is essential as it releases any resources allocated by GLFW
     * and ensures proper cleanup. Make sure to call this function before the
     * application terminates.
     */
    void GLFW_BaseAPI::Quit() noexcept
    {
        if (m_Initialized)
        {
            glfwTerminate();
            m_Initialized = false;
        }
    }

    /**
     * Constructs a GLFW_Window object with the specified window handle and title.
     *
     * This constructor initializes the GLFW window with the given handle and title,
     * setting up the necessary properties and context for rendering.
     *
     * @param windowHandle The unique identifier for the window.
     * @param title The title of the window to be displayed.
     */
    GLFW_Window::GLFW_Window(WindowHandle windowHandle, const std::string& title)
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

        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_RED_BITS, m_Properties.ColorBits.RedBit);
        glfwWindowHint(GLFW_GREEN_BITS, m_Properties.ColorBits.GreenBit);
        glfwWindowHint(GLFW_BLUE_BITS, m_Properties.ColorBits.BlueBit);
        glfwWindowHint(GLFW_ALPHA_BITS, m_Properties.ColorBits.AlphaBit);
        glfwWindowHint(GLFW_REFRESH_RATE, m_Properties.RefreshRate);
        glfwWindowHint(GLFW_DEPTH_BITS, m_Properties.ColorBits.DepthBit);
        glfwWindowHint(GLFW_STENCIL_BITS, m_Properties.ColorBits.DepthStencilBit);

        m_Window = glfwCreateWindow(m_Properties.Width, m_Properties.Height, m_Properties.Title.c_str(), nullptr, nullptr);
        if (m_Window != nullptr)
        {
            switch (Renderer::GetAPI())
            {
            case RenderingAPI::OpenGL:
                m_Context = std::make_shared<GLFW_GL_Context>();
                break;
            case RenderingAPI::Vulkan:
                MOTION_ASSERT(false, "Vulkan is not supported yet");
                break;
            case RenderingAPI::DirectX:
                MOTION_ASSERT(false, "DirectX is not supported yet");
                break;
            default:
                MOTION_ASSERT(false, "Unknown Rendering API");
                break;
            };

            m_Context->Attach(m_Window);
            if (!m_Context->Activate())
            {
                MOTION_CORE_CRITICAL("Failed to activate graphics context");
                return;
            }


            glfwSetWindowSizeLimits(m_Window, m_Properties.MinWidth, m_Properties.MinHeight, GLFW_DONT_CARE, GLFW_DONT_CARE);
            glfwGetFramebufferSize(m_Window, &m_Properties.PixelWidth, &m_Properties.PixelHeight);

            m_Properties.IsActive = true;
            m_Properties.IsFocused = glfwGetWindowAttrib(m_Window, GLFW_FOCUSED);
            m_Properties.IsVSyncEnabled = true;

            glfwSetWindowUserPointer(m_Window, this);
            SetEventsCallBacks();
            RegisterEventsCallBacks();
        }
        else
        {
            const char* lastError{ nullptr };
            int32_t errorCode = glfwGetError(&lastError);
            MOTION_ASSERT(m_Window, "Failed to create GLFW Window | GLFW Error Code:{0} | Error: {1}", errorCode, lastError);
            return;
        }
    }

    /**
     * @brief Destructor for the GLFW_Window class.
     *
     * This destructor is responsible for properly destroying the GLFW window
     * associated with this object by calling glfwDestroyWindow on the internal
     * window handle (m_Window). This ensures that all resources allocated for
     * the window are released when the GLFW_Window object is destroyed.
     */
    GLFW_Window::~GLFW_Window()
    {
        if (m_Context->GetCurrentContext() == m_Window)
            m_Context->Detach();

        glfwDestroyWindow(m_Window);
    }


    /**
     * GLFW callback function for window close events.
     *
     * When the user closes a window, GLFW will call this function. This function
     * will then invoke the WindowClose event on the EventRegistry.
     *
     * @param [in] window The window that was closed.
     *
     * @ingroup window
     */
    static void glfwWindowCloseEvent(GLFWwindow* window)
    {
        EventRegistry<GLFWwindow*>::Invoke(EventType::WindowClose, window);
    }

    /**
     * GLFW callback function for window resize events.
     *
     * When the user resizes a window, GLFW will call this function. This function
     * will then invoke the WindowResize event on the EventRegistry with the new
     * width and height of the window.
     *
     * @param [in] window The window that was resized.
     * @param [in] width The new width of the window, in pixels.
     * @param [in] height The new height of the window, in pixels.
     *
     * @ingroup window
     */
    static void glfwWindowResizeEvent(GLFWwindow* window, int width, int height)
    {
        EventRegistry<GLFWwindow*, int, int>::Invoke(EventType::WindowResize, window, width, height);
    }

    /**
     * GLFW callback function for window focus events.
     *
     * When the user brings a window into focus or takes it out of focus, GLFW
     * will call this function. This function will then invoke either the
     * WindowFocusGain event or the WindowFocusLost event on the EventRegistry
     * depending on the focused value.
     *
     * @param [in] window The window that gained or lost focus.
     * @param [in] focused The new focus state of the window. 1 if the window
     * gained focus, 0 if it lost focus.
     *
     * @ingroup window
     */
    static void glfwWindowFocusEvent(GLFWwindow* window, int focused)
    {
        EventRegistry<GLFWwindow*, int>::Invoke((focused) ? EventType::WindowFocusGain : EventType::WindowFocusLost, window, focused);
    }

    /**
     * GLFW callback function for window iconify events.
     *
     * This function is called when a window is iconified (minimized) or restored.
     * It will then invoke the WindowMinimize event on the EventRegistry with the
     * iconification state of the window.
     *
     * @param [in] window The window that was iconified or restored.
     * @param [in] iconified The iconification state of the window. 1 if the
     * window was iconified, 0 if it was restored.
     *
     * @ingroup window
     */
    static void glfwWindowIconifyEvent(GLFWwindow* window, int iconified)
    {
        EventRegistry<GLFWwindow*, int>::Invoke(EventType::WindowMinimize, window, iconified);
    }

    /**
     * GLFW callback function for window maximize events.
     *
     * This function is called when a window is maximized or restored to its
     * original size. It will then invoke the WindowMaximize event on the
     * EventRegistry with the maximization state of the window.
     *
     * @param [in] window The window that was maximized or restored.
     * @param [in] maximized The maximization state of the window. 1 if the
     * window was maximized, 0 if it was restored.
     *
     * @ingroup window
     */
    static void glfwWindowMaximizeEvent(GLFWwindow* window, int maximized)
    {
        EventRegistry<GLFWwindow*, int>::Invoke(EventType::WindowMaximize, window, maximized);
    }

    /**
     * GLFW callback function for window position events.
     *
     * This function is called when the position of a window is changed. It will
     * then invoke the WindowPosChange event on the EventRegistry with the
     * window and the new position coordinates.
     *
     * @param [in] window The window whose position was changed.
     * @param [in] x The new x-coordinate of the window.
     * @param [in] y The new y-coordinate of the window.
     *
     * @ingroup window
     */
    static void glfwWindowPosEvent(GLFWwindow* window, int x, int y)
    {
        EventRegistry<GLFWwindow*, int, int>::Invoke(EventType::WindowPosChange, window, x, y);
    }

    /**
     * GLFW callback function for window frame buffer size events.
     *
     * This function is called when the frame buffer size of a window is changed.
     * It will then invoke the WindowFrameBufferSizeChange event on the EventRegistry
     * with the window and the new frame buffer size coordinates.
     *
     * @param [in] window The window whose frame buffer size was changed.
     * @param [in] width The new width of the frame buffer, in pixels.
     * @param [in] height The new height of the frame buffer, in pixels.
     *
     * @ingroup window
     */
    static void glfwWindowFrameBufferSizeEvent(GLFWwindow* window, int width, int height)
    {
        EventRegistry<GLFWwindow*, int, int>::Invoke(EventType::WindowFrameBufferSizeChange, window, width, height);
    }

    /**
     * GLFW callback function for key events.
     *
     * This function is called when a key is pressed, repeated, or released in a
     * window. It will then invoke the KeyboardKeyPress event on the EventRegistry
     * with the window and the key, scancode, action, and modifiers of the key
     * press.
     *
     * @param [in] window The window that received the key event.
     * @param [in] key The key that was pressed, repeated, or released.
     * @param [in] scancode The scancode of the key that was pressed, repeated, or
     * released.
     * @param [in] action The action that was performed on the key. One of GLFW_PRESS,
     * GLFW_REPEAT, or GLFW_RELEASE.
     * @param [in] mods The modifier keys that were held down while the key was
     * pressed, repeated, or released. This is a bit mask of the following
     * constants: GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, GLFW_MOD_ALT, GLFW_MOD_SUPER.
     *
     * @ingroup window
     */
    static void glfwKeyEvent(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        EventRegistry<GLFWwindow*, int, int, int, int>::Invoke(EventType::KeyboardKeyPress, window, key, scancode, action, mods);
    }

    /**
     * GLFW callback function for character input events.
     *
     * This function is called when a Unicode character is input in a window.
     * It will then invoke the KeybaordKeyChar event on the EventRegistry
     * with the window and the Unicode code point of the character.
     *
     * @param [in] window The window that received the character input event.
     * @param [in] codepoint The Unicode code point of the input character.
     *
     * @ingroup window
     */

    static void glfwSetKeyCharEvent(GLFWwindow* window, unsigned int codepoint)
    {
        EventRegistry<GLFWwindow*, unsigned int>::Invoke(EventType::KeyboardKeyChar, window, codepoint);
    }

    /**
     * GLFW callback function for mouse button events.
     *
     * This function is called when a mouse button is pressed, repeated, or released
     * in a window. It will then invoke the MouseButtonDown event on the
     * EventRegistry with the window, mouse button, action, and modifiers of the
     * button press.
     *
     * @param [in] window The window that received the mouse button event.
     * @param [in] button The mouse button that was pressed, repeated, or released.
     * @param [in] action The action that was performed on the mouse button. One of
     * GLFW_PRESS, GLFW_REPEAT, or GLFW_RELEASE.
     * @param [in] mods The modifier keys that were held down while the mouse
     * button was pressed, repeated, or released. This is a bit mask of the
     * following constants: GLFW_MOD_SHIFT, GLFW_MOD_CONTROL, GLFW_MOD_ALT,
     * GLFW_MOD_SUPER.
     *
     * @ingroup window
     */
    static void glfwMouseButtonEvent(GLFWwindow* window, int button, int action, int mods)
    {
        EventRegistry<GLFWwindow*, int, int, int>::Invoke(EventType::MouseButtonDown, window, button, action, mods);
    }

    /**
     * GLFW callback function for mouse cursor position events.
     *
     * This function is called when the mouse cursor is moved in a window.
     * It will then invoke the MouseCursorPosChange event on the EventRegistry
     * with the window and the new (x, y) position of the mouse cursor.
     *
     * @param [in] window The window that received the mouse cursor position event.
     * @param [in] x The new x-coordinate of the mouse cursor.
     * @param [in] y The new y-coordinate of the mouse cursor.
     *
     * @ingroup window
     */
    static void glfwMouseCursorPosEvent(GLFWwindow* window, double x, double y)
    {
        EventRegistry<GLFWwindow*, double, double>::Invoke(EventType::MouseCursorPosChange, window, x, y);
    }

    /**
     * GLFW callback function for mouse cursor enter/leave events.
     *
     * This function is called when the mouse cursor enters or leaves the client
     * area of a window. It will then invoke the MouseCursorWindowEnter event on
     * the EventRegistry with the window and the entered state.
     *
     * @param [in] window The window that received the cursor enter/leave event.
     * @param [in] entered GLFW_TRUE if the cursor entered the window's client
     * area, or GLFW_FALSE if it left it.
     *
     * @ingroup window
     */
    static void glfwMouseCursorEnterEvent(GLFWwindow* window, int entered)
    {
        EventRegistry<GLFWwindow*, int>::Invoke(EventType::MouseCursorWindowEnter, window, entered);
    }

    /**
     * GLFW callback function for mouse scroll events.
     *
     * This function is called when the user scrolls the mouse wheel in a window.
     * It will then invoke the MouseWheelScroll event on the EventRegistry with
     * the window, x offset, and y offset of the scroll.
     *
     * @param [in] window The window that received the mouse scroll event.
     * @param [in] x The x offset of the scroll, positive for scrolling to the right
     * and negative for scrolling to the left.
     * @param [in] y The y offset of the scroll, positive for scrolling up and
     * negative for scrolling down.
     *
     * @ingroup window
     */
    static void glfwMouseScrollEvent(GLFWwindow* window, double x, double y)
    {
        EventRegistry<GLFWwindow*, double, double>::Invoke(EventType::MouseWheelScroll, window, x, y);
    }

    /**
     * Sets up the GLFW event callbacks for the window. These callbacks are
     * required for the window to receive events from the operating system.
     *
     * @ingroup window
     */
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

    /**
     * Registers the GLFW callbacks for the window. These callbacks are required
     * for the window to receive events from the operating system.
     *
     * @ingroup window
     */
    void GLFW_Window::RegisterEventsCallBacks()
    {
        EventRegistry<GLFWwindow*>::Register(EventType::WindowClose,
            [this](GLFWwindow* window)
            {
                EventWindowClose windowCloseEvent;
                m_CallbackFunc(m_Properties.Handle, windowCloseEvent);
            }
        );

        EventRegistry<GLFWwindow*, int, int>::Register(EventType::WindowResize,
            [this](GLFWwindow* window, int width, int height)
            {
                EventWindowResize windowResizeEvent(width, height);
                m_Properties.Width = width;
                m_Properties.Height = height;
                m_CallbackFunc(m_Properties.Handle, windowResizeEvent);
            }
        );

        EventRegistry<GLFWwindow*, int>::Register(EventType::WindowFocusGain,
            [this](GLFWwindow* window, int focused)
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
            [this](GLFWwindow* window, int iconified)
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
            [this](GLFWwindow* window, int maximized)
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
            [this](GLFWwindow* window, int x, int y)
            {
                EventWindowPosChange windowMoveEvent(x, y);
                m_Properties.PosX = x;
                m_Properties.PosY = y;
                m_CallbackFunc(m_Properties.Handle, windowMoveEvent);
            }
        );

        EventRegistry<GLFWwindow*, int, int>::Register(EventType::WindowFrameBufferSizeChange,
            [this](GLFWwindow* window, int width, int height)
            {
                EventWindowFrameBufferSizeChange windowPixelSizeEvent(width, height);
                m_Properties.PixelWidth = width;
                m_Properties.PixelHeight = height;
                m_CallbackFunc(m_Properties.Handle, windowPixelSizeEvent);
            }
        );

        EventRegistry<GLFWwindow*, int, int, int, int>::Register(EventType::KeyboardKeyPress,
            [this](GLFWwindow* window, int key, int scancode, int action, int mods)
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
            [this](GLFWwindow* window, unsigned int codepoint)
            {
                EventKeyboardKeyChar keyCharEvent(codepoint);
                m_CallbackFunc(m_Properties.Handle, keyCharEvent);
            }
        );




        EventRegistry<GLFWwindow*, int, int, int>::Register(EventType::MouseButtonDown,
            [this](GLFWwindow* window, int button, int action, int mods)
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
            [this](GLFWwindow* window, double x, double y)
            {
                EventMouseCursorMove mouseCursorPosEvent(x, y);
                m_CallbackFunc(m_Properties.Handle, mouseCursorPosEvent);
            }
        );

        EventRegistry<GLFWwindow*, int>::Register(EventType::MouseCursorWindowEnter,
            [this](GLFWwindow* window, int entered)
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
            [this](GLFWwindow* window, double x, double y)
            {
                EventMouseWheelScroll mouseWheelScrollEvent(x, y);
                m_CallbackFunc(m_Properties.Handle, mouseWheelScrollEvent);
            }
        );
    }

    /**
     * Polls events from the operating system and dispatches them to the event
     * callback function set with SetEventsCallbackFunc().
     *
     * This function is a wrapper around the GLFW function glfwWaitEvents().
     * Therefore, it is not necessary to call glfwPollEvents() if you have set up
     * your own event handling loop.
     *
     * @sa glfwWaitEvents
     * @sa SetEventsCallbackFunc
     */
    void GLFW_Window::PollEvents() noexcept
    {
        glfwWaitEvents();
    }

    /**
     * Swaps the front and back buffers of the window.
     *
     * This function is a wrapper around the GLFW function glfwSwapBuffers() and
     * the OpenGL function wglSwapLayerBuffers() or glXSwapBuffers(), depending on
     * the context of the window.
     *
     * @note If no context is attached to the window, this function does nothing.
     *
     * @sa glfwSwapBuffers
     * @sa wglSwapLayerBuffers
     * @sa glXSwapBuffers
     */
    void GLFW_Window::SwapBuffers() noexcept
    {
        if (m_Context != nullptr)
        {
            m_Context->SwapBuffers(m_Window);
        }
    }

    /**
     * Sets the context for the GLFW window.
     *
     * This function assigns the provided context to the window and, if the context
     * is valid (non-null), attaches it to the window. This is necessary for the
     * window to utilize the context for rendering operations.
     *
     * @param context A shared pointer to the IContext instance to be set.
     */
    void GLFW_Window::SetContext(const std::shared_ptr<IContext>& context) noexcept
    {
        m_Context = context;
        if (m_Context != nullptr)
        {
            m_Context->Attach(m_Window);
        }
    }

    /**
     * Retrieves the context of the GLFW window.
     *
     * This function provides access to the context service currently in use by
     * the window. It returns a shared pointer to the IContext instance, allowing
     * for further interaction with the context-specific implementation details.
     *
     * @return A shared pointer to the IContext instance.
     */
    std::shared_ptr<IContext> GLFW_Window::GetContext() const noexcept
    {
        return m_Context;
    }

    /**
     * Sets the callback function to be invoked when window events occur.
     *
     * This function sets the callback function to be invoked when window events
     * occur, such as resizing, moving, or closing the window. The function takes
     * two parameters: the first parameter is the type of event, and the second
     * parameter is the event data. The callback function is expected to handle
     * the event accordingly.
     *
     * @param callbackFunc The callback function to be invoked when window events occur.
     */
    void GLFW_Window::SetEventsCallbackFunc(const EventProcessingFunction& callbackFunc) noexcept
    {
        m_CallbackFunc = callbackFunc;
    }
}