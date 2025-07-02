#pragma once

#include <glm/glm.hpp>
#include "Window.hpp"
#include "KeyCodes.hpp"

namespace Motion::Core
{
    class GLFW3_InputsHandler
    {
        private:
            GLFW3_InputsHandler() = default;
            ~GLFW3_InputsHandler() = default;
            
            GLFW3_InputsHandler(GLFW3_InputsHandler const&) = delete;
            GLFW3_InputsHandler& operator=(GLFW3_InputsHandler const&) = delete;
            GLFW3_InputsHandler(GLFW3_InputsHandler&&) = delete;
            GLFW3_InputsHandler& operator=(GLFW3_InputsHandler&&) = delete;

        public:
            static KeyState KeyState(WindowHandle whnd, KeyCode key);
            static MouseButtonState MouseButtonState(WindowHandle whnd, MouseButton button);
            static glm::vec2 CurrentMousePosition(WindowHandle whnd);
    };
}