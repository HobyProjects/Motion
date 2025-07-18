#include "CorePCH.hpp"

namespace Motion::Core
{
    KeyState InputsHandler::GetKeyState(WindowHandle windowHandle, KeyCode key)
    {
        return GLFW_KeyState(windowHandle, key);
    }

    MouseButtonState InputsHandler::GetMouseButtonState(WindowHandle windowHandle, MouseButton button)
    {
        return GLFW_MouseButtonState(windowHandle, button);
    }

    glm::vec2 InputsHandler::GetCurrentMousePosition(WindowHandle windowHandle)
    {
        return GLFW_CurrentMousePosition(windowHandle);
    }
}