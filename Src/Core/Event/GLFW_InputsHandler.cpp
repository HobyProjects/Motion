#include "CorePCH.hpp"

namespace Motion::Core
{
    /**
     * @brief Retrieves the current state of a specific key for the given window.
     *
     * This function queries the state of the specified key (pressed, released, etc.)
     * associated with the provided window handle.
     *
     * @param nativeWindow The handle to the window for which the key state is queried.
     * @param key The key code representing the key to check.
     * @return KeyState The current state of the specified key.
     */
    KeyState GLFW_KeyState(NativeWindow nativeWindow, KeyCode key) noexcept
    {
        std::int32_t state = glfwGetKey((GLFWwindow*)nativeWindow, static_cast<std::int32_t>(key));

        if (state == GLFW_PRESS)
            return KeyState::KEY_PRESSED;
        if (state == GLFW_RELEASE)
            return KeyState::KEY_RELEASED;
        if (state == GLFW_REPEAT)
            return KeyState::KEY_REPEAT;

        return KeyState::KEY_NONE;
    }

    /**
     * @brief Retrieves the current state of a specified mouse button for a given window.
     *
     * This function queries the state (pressed, released, etc.) of the specified mouse button
     * associated with the provided window handle.
     *
     * @param nativeWindow The handle to the window for which the mouse button state is queried.
     * @param button The mouse button whose state is to be retrieved.
     * @return MouseButtonState The current state of the specified mouse button.
     */
    MouseButtonState GLFW_MouseButtonState(NativeWindow nativeWindow, MouseButton button) noexcept
    {
        std::int32_t state = glfwGetMouseButton((GLFWwindow*)nativeWindow, static_cast<std::int32_t>(button));

        if (state == GLFW_PRESS)
            return MouseButtonState::MOUSE_BUTTON_PRESSED;
        if (state == GLFW_RELEASE)
            return MouseButtonState::MOUSE_BUTTON_RELEASED;

        return MouseButtonState::MOUSE_BUTTON_NONE;
    }

    /**
     * @brief Retrieves the current mouse position relative to the specified window.
     *
     * This function queries the current position of the mouse cursor within the given window
     * and returns it as a 2D vector (x, y) in screen coordinates.
     *
     * @param nativeWindow The handle to the window for which the mouse position is requested.
     * @return glm::vec2 The current mouse position in screen coordinates.
     */
    glm::vec2 GLFW_CurrentMousePosition(NativeWindow nativeWindow) noexcept
    {
        double posX{ 0.0 }, posY{ 0.0 };
        glfwGetCursorPos((GLFWwindow*)nativeWindow, &posX, &posY);
        return glm::vec2(static_cast<float>(posX), static_cast<float>(posY));
    }
}
