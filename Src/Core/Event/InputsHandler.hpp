#pragma once

#include <glm/glm.hpp>
#include "Window.hpp"
#include "KeyCodes.hpp"

namespace Motion::Core
{
    class InputsHandler
    {
    private:
        InputsHandler() = default;
        ~InputsHandler() = default;

        InputsHandler(InputsHandler const&) = delete;
        InputsHandler& operator=(InputsHandler const&) = delete;
        InputsHandler(InputsHandler&&) = delete;
        InputsHandler& operator=(InputsHandler&&) = delete;

    public:
        [[nodiscard]] static KeyState GetKeyState(WindowHandle windowHandle, KeyCode key) noexcept;
        [[nodiscard]] static MouseButtonState GetMouseButtonState(WindowHandle windowHandle, MouseButton button) noexcept;
        [[nodiscard]] static glm::vec2 GetCurrentMousePosition(WindowHandle windowHandle) noexcept;
    };
}