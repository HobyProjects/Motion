#pragma once

#include <glm/glm.hpp>
#include "Window.hpp"
#include "KeyCodes.hpp"

namespace Motion::Core
{
    [[nodiscard]] KeyState GLFW_KeyState(NativeWindow nativeWindow, KeyCode key) noexcept;
    [[nodiscard]] MouseButtonState GLFW_MouseButtonState(NativeWindow nativeWindow, MouseButton button) noexcept;
    [[nodiscard]] glm::vec2 GLFW_CurrentMousePosition(NativeWindow nativeWindow) noexcept;
}