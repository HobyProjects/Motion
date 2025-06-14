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
            static KeyState GetKeyState(WindowHandle whnd, KeyCode key);
            static MouseButtonState GetMouseButtonState(WindowHandle whnd, MouseButton button);
            static glm::vec2 GetCurrentMousePosition(WindowHandle whnd);
    };
}