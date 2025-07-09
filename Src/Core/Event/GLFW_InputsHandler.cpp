#include "CorePCH.hpp"

namespace Motion::Core
{
    KeyState GLFW3_InputsHandler::KeyState(WindowHandle whnd, KeyCode key)
    {
        std::weak_ptr<IWindow> window = WindowManager::GetWindow(whnd);
        if(!window.expired())
        {
            auto windowPtr = window.lock();
            int state = glfwGetKey((GLFWwindow*) windowPtr->GetNativeWindow(), static_cast<int>( key ));
            
            if( state == GLFW_PRESS )
                return KeyState::KEY_PRESSED;
            if( state == GLFW_RELEASE )
                return KeyState::KEY_RELEASED;
            if( state == GLFW_REPEAT )
                return KeyState::KEY_REPEAT;

            return KeyState::KEY_NONE;
        }

        MOTION_CORE_CRITICAL("Input handling from destroyed window. HANDLE: {:X}", whnd);
        return KeyState::KEY_NONE;
    }

    MouseButtonState GLFW3_InputsHandler::MouseButtonState(WindowHandle whnd, MouseButton button)
    {
        std::weak_ptr<IWindow> window = WindowManager::GetWindow(whnd);
        if(!window.expired())
        {
            auto windowPtr = window.lock();
            int state = glfwGetMouseButton((GLFWwindow*) windowPtr->GetNativeWindow(), static_cast<int>( button ));
            
            if( state == GLFW_PRESS )
                return MouseButtonState::MOUSE_BUTTON_PRESSED;
            if( state == GLFW_RELEASE )
                return MouseButtonState::MOUSE_BUTTON_RELEASED;

            return MouseButtonState::MOUSE_BUTTON_NONE;
        }

        MOTION_CORE_CRITICAL("Input handling from destroyed window. HANDLE: {:X}", whnd);
        return MouseButtonState::MOUSE_BUTTON_NONE;
    }

    glm::vec2 GLFW3_InputsHandler::CurrentMousePosition(WindowHandle whnd)
    {
        std::weak_ptr<IWindow> window = WindowManager::GetWindow(whnd);
        if(!window.expired())
        {
            auto windowPtr = window.lock();
            double posX{ 0.0 }, posY{ 0.0 };
            glfwGetCursorPos((GLFWwindow*) windowPtr->GetNativeWindow(), &posX, &posY);
            return glm::vec2(static_cast<float>( posX ), static_cast<float>( posY ));
        }

        MOTION_CORE_CRITICAL("Input handling from destroyed window. HANDLE: {:X}", whnd);
        return glm::vec2(0.0f, 0.0f);
    }
}
