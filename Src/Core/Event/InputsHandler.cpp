#include "CorePCH.hpp"

namespace Motion::Core
{
    KeyState InputsHandler::GetKeyState(WindowHandle whnd, KeyCode key)
    {
        return GLFW3_InputsHandler::KeyState(whnd, key);
    }

    MouseButtonState InputsHandler::GetMouseButtonState(WindowHandle whnd, MouseButton button)
    {
        return GLFW3_InputsHandler::MouseButtonState(whnd, button);
    }

    glm::vec2 InputsHandler::GetCurrentMousePosition(WindowHandle whnd)
    {
        return GLFW3_InputsHandler::CurrentMousePosition(whnd);
    }
}