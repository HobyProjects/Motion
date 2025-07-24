#include "CorePCH.hpp"

namespace Motion
{
    /**
     * @brief Retrieves the current state of a specific key for the given window.
     *
     * This function queries the state of the specified key (pressed, released, etc.)
     * associated with the provided window handle.
     *
     * @param windowHandle The handle to the window for which the key state is queried.
     * @param key The key code representing the key to check.
     * @return KeyState The current state of the specified key.
     */
    KeyState InputsHandler::GetKeyState(WindowHandle windowHandle, KeyCode key) noexcept
    {
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);
        if (!window.expired())
        {
            return GLFW_KeyState(window.lock()->GetNativeWindow(), key);
        }

        MOTION_CORE_ERROR("Input handling from a non-existing window: {}", windowHandle);
        return KEY_STATE_UNKNOWN;
    }

    /**
     * @brief Retrieves the current state of a specified mouse button for a given window.
     *
     * This function queries the state (pressed, released, etc.) of the specified mouse button
     * associated with the provided window handle.
     *
     * @param windowHandle The handle to the window for which the mouse button state is queried.
     * @param button The mouse button whose state is to be retrieved.
     * @return MouseButtonState The current state of the specified mouse button.
     */
    MouseButtonState InputsHandler::GetMouseButtonState(WindowHandle windowHandle, MouseButton button) noexcept
    {
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);
        if (!window.expired())
        {
            return GLFW_MouseButtonState(window.lock()->GetNativeWindow(), button);
        }

        MOTION_CORE_ERROR("Input handling from a non-existing window: {}", windowHandle);
        return MOUSE_BUTTON_STATE_UNKNOWN;
    }

    /**
     * @brief Retrieves the current mouse position relative to the specified window.
     *
     * This function queries the current position of the mouse cursor within the given window
     * and returns it as a 2D vector (x, y) in screen coordinates.
     *
     * @param windowHandle The handle to the window for which the mouse position is requested.
     * @return glm::vec2 The current mouse position in screen coordinates.
     */
    glm::vec2 InputsHandler::GetCurrentMousePosition(WindowHandle windowHandle) noexcept
    {
        auto& windowManager = WindowManager::GetInstance();
        std::weak_ptr<IWindow> window = windowManager.GetWindow(windowHandle);
        if (!window.expired())
        {
            return GLFW_CurrentMousePosition(window.lock()->GetNativeWindow());
        }

        MOTION_CORE_ERROR("Input handling from a non-existing window: {}", windowHandle);
        return glm::vec2(0.0f);
    }
}