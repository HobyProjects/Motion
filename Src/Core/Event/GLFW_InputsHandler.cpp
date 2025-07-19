#include "CorePCH.hpp"

namespace Motion::Core
{
    /**
     * @brief Retrieves the current state of a specified key for a given window.
     *
     * This function queries the GLFW input system to determine the state of the specified key
     * (pressed, released, repeated, or none) for the window identified by the provided handle.
     * If the window is no longer valid or has been destroyed, a critical log message is generated
     * and KeyState::KEY_NONE is returned.
     *
     * @param windowHandle The handle identifying the window to query.
     * @param key The key code to check the state for.
     * @return KeyState The current state of the specified key (KEY_PRESSED, KEY_RELEASED, KEY_REPEAT, or KEY_NONE).
     */
    KeyState GLFW_KeyState(WindowHandle windowHandle, KeyCode key) noexcept
    {
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);

        if (!window.expired())
        {
            auto windowPtr = window.lock();
            int state = glfwGetKey((GLFWwindow*)windowPtr->GetNativeWindow(), static_cast<int>(key));

            if (state == GLFW_PRESS)
                return KeyState::KEY_PRESSED;
            if (state == GLFW_RELEASE)
                return KeyState::KEY_RELEASED;
            if (state == GLFW_REPEAT)
                return KeyState::KEY_REPEAT;

            return KeyState::KEY_NONE;
        }

        MOTION_CORE_CRITICAL("Input handling from destroyed window. HANDLE: {:X}", windowHandle);
        return KeyState::KEY_NONE;
    }

    /**
     * @brief Retrieves the current state of a specified mouse button for a given window.
     *
     * This function queries the GLFW library to determine whether the specified mouse button
     * is pressed, released, or in an undefined state for the window identified by the given handle.
     * If the window associated with the handle has been destroyed or is otherwise unavailable,
     * the function logs a critical error and returns MouseButtonState::MOUSE_BUTTON_NONE.
     *
     * @param windowHandle The handle identifying the window to query.
     * @param button The mouse button to check (typically left, right, or middle).
     * @return MouseButtonState The current state of the specified mouse button:
     *         - MOUSE_BUTTON_PRESSED if the button is currently pressed,
     *         - MOUSE_BUTTON_RELEASED if the button is currently released,
     *         - MOUSE_BUTTON_NONE if the state is undefined or the window is invalid.
     */
    MouseButtonState GLFW_MouseButtonState(WindowHandle windowHandle, MouseButton button) noexcept
    {
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);

        if (!window.expired())
        {
            auto windowPtr = window.lock();
            int state = glfwGetMouseButton((GLFWwindow*)windowPtr->GetNativeWindow(), static_cast<int>(button));

            if (state == GLFW_PRESS)
                return MouseButtonState::MOUSE_BUTTON_PRESSED;
            if (state == GLFW_RELEASE)
                return MouseButtonState::MOUSE_BUTTON_RELEASED;

            return MouseButtonState::MOUSE_BUTTON_NONE;
        }

        MOTION_CORE_CRITICAL("Input handling from destroyed window. HANDLE: {:X}", windowHandle);
        return MouseButtonState::MOUSE_BUTTON_NONE;
    }

    /**
     * @brief Retrieves the current mouse cursor position for the specified window.
     *
     * This function queries the current position of the mouse cursor relative to the client area
     * of the window identified by the given window handle. If the window is valid, it returns
     * the cursor position as a glm::vec2, where x and y are the cursor's coordinates in pixels.
     * If the window is invalid or destroyed, it logs a critical error and returns (0.0f, 0.0f).
     *
     * @param windowHandle The handle identifying the window to query.
     * @return glm::vec2 The current mouse position in window coordinates, or (0.0f, 0.0f) if the window is invalid.
     */
    glm::vec2 GLFW_CurrentMousePosition(WindowHandle windowHandle) noexcept
    {
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);

        if (!window.expired())
        {
            auto windowPtr = window.lock();
            double posX{ 0.0 }, posY{ 0.0 };
            glfwGetCursorPos((GLFWwindow*)windowPtr->GetNativeWindow(), &posX, &posY);
            return glm::vec2(static_cast<float>(posX), static_cast<float>(posY));
        }

        MOTION_CORE_CRITICAL("Input handling from destroyed window. HANDLE: {:X}", windowHandle);
        return glm::vec2(0.0f, 0.0f);
    }
}
