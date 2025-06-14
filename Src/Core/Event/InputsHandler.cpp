#include "CorePCH.hpp"

namespace Motion::Core
{
    KeyState GetKeyState(WindowHandle whnd, KeyCode key)
    {
        return GLFW3_InputsHandler::GetKeyState(whnd, key);
    }

    MouseButtonState GetMouseButtonState(WindowHandle whnd, MouseButton button)
    {
        return GLFW3_InputsHandler::GetMouseButtonState(whnd, button);
    }

    glm::vec2 GetCurrentMousePosition(WindowHandle whnd)
    {
        return GLFW3_InputsHandler::GetCurrentMousePosition(whnd);
    }
}