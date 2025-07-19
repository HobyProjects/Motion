#pragma once

#include <glm/glm.hpp>
#include "Window.hpp"
#include "KeyCodes.hpp"

namespace Motion::Core
{
    [[nodiscard]] KeyState GLFW_KeyState(WindowHandle windowHandle, KeyCode key) noexcept;
    [[nodiscard]] MouseButtonState GLFW_MouseButtonState(WindowHandle windowHandle, MouseButton button) noexcept;
    [[nodiscard]] glm::vec2 GLFW_CurrentMousePosition(WindowHandle windowHandle) noexcept;
}