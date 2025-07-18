#pragma once

#include <glm/glm.hpp>
#include "Window.hpp"
#include "KeyCodes.hpp"

namespace Motion::Core
{
    KeyState GLFW_KeyState(WindowHandle windowHandle, KeyCode key);
    MouseButtonState GLFW_MouseButtonState(WindowHandle windowHandle, MouseButton button);
    glm::vec2 GLFW_CurrentMousePosition(WindowHandle windowHandle);
}