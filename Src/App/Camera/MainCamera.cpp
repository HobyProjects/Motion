#include "MainCamera.hpp"
#include "InputsHandler.hpp"

using namespace Motion::Core;

namespace Motion::App
{
    MainCamera::MainCamera(float viewportWidth, float viewportHeight, bool rotationEnabled)
    {
        Camera3D.AspectRatio = viewportWidth / viewportHeight;
        Camera3D.ViewportWidth = viewportWidth;
        Camera3D.ViewportHeight = viewportHeight;
        Camera3D.RotationEnabled = rotationEnabled;
        Camera3D.RefreshCameraMatrix();
    }

    void MainCamera::SetAspectRatio(float width, float height)
    {
        Camera3D.AspectRatio = width / height;
        Camera3D.ViewportWidth = width;
        Camera3D.ViewportHeight = height;
        Camera3D.RefreshCameraMatrix();
    }

    void MainCamera::OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime)
    {
        if(Motion::Core::InputsHandler::GetMouseButtonState(handle, Motion::Core::MouseButton::MOUSE_BUTTON_RIGHT) & 
            (Motion::Core::KeyState::KEY_PRESSED | Motion::Core::KeyState::KEY_REPEAT))
        {
            if(Motion::Core::InputsHandler::GetKeyState(handle, Motion::Core::KeyCode::KEY_W) &
                (Motion::Core::KeyState::KEY_PRESSED | Motion::Core::KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * Camera3D.Oriantaion;
                Camera3D.RefreshCameraMatrix();
            }

            if(Motion::Core::InputsHandler::GetKeyState(handle, Motion::Core::KeyCode::KEY_S) &
                (Motion::Core::KeyState::KEY_PRESSED | Motion::Core::KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * -Camera3D.Oriantaion;
                Camera3D.RefreshCameraMatrix();
            }

            if(Motion::Core::InputsHandler::GetKeyState(handle, Motion::Core::KeyCode::KEY_A) &
                (Motion::Core::KeyState::KEY_PRESSED | Motion::Core::KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * -glm::normalize(glm::cross(Camera3D.Oriantaion, Camera3D.WorldUp));
                Camera3D.RefreshCameraMatrix();
            }

            if(Motion::Core::InputsHandler::GetKeyState(handle, Motion::Core::KeyCode::KEY_D) &
                (Motion::Core::KeyState::KEY_PRESSED | Motion::Core::KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * glm::normalize(glm::cross(Camera3D.Oriantaion, Camera3D.WorldUp));
                Camera3D.RefreshCameraMatrix();
            }

            if(Motion::Core::InputsHandler::GetKeyState(handle, Motion::Core::KeyCode::KEY_LEFT_CONTROL) &
                (Motion::Core::KeyState::KEY_PRESSED | Motion::Core::KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * -Camera3D.WorldUp;
                Camera3D.RefreshCameraMatrix();
            }

            if(Motion::Core::InputsHandler::GetKeyState(handle, Motion::Core::KeyCode::KEY_SPACE) &
                (Motion::Core::KeyState::KEY_PRESSED | Motion::Core::KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * Camera3D.WorldUp;
                Camera3D.RefreshCameraMatrix();
            }
        }
    }

    void MainCamera::OnEvents(Motion::Core::WindowHandle handle, Motion::Core::IEvent & e)
    {
        Motion::Core::EventHandler handler(handle, e);
        handler.Dispatch<Motion::Core::EventMouseCursorMove<float>>(EVENT_CALLBACK(OnMouseCursorPosChange));
        handler.Dispatch<Motion::Core::EventMouseWheelScroll<float>>(EVENT_CALLBACK(OnMouseWheelScrollEvent));
    }

    bool MainCamera::OnMouseCursorPosChange(Motion::Core::WindowHandle handle, Motion::Core::EventMouseCursorMove<float> & e)
    {
        if(Motion::Core::InputsHandler::GetMouseButtonState(handle, Motion::Core::MouseButton::MOUSE_BUTTON_RIGHT) &
            (Motion::Core::KeyState::KEY_PRESSED | Motion::Core::KeyState::KEY_REPEAT))
        {
            float xOffset = e.GetX() - m_MouseX;
			float yOffset = e.GetY() - m_MouseY;  // Change this to invert camera control

			m_MouseX = e.GetX();
			m_MouseY = e.GetY();

			xOffset *= Camera3D.Sensitivity;
			yOffset *= Camera3D.Sensitivity;

			m_Yaw += xOffset;
			m_Pitch -= yOffset;  // Flip the sign to fix inversion

			// Clamp Pitch to prevent flipping
			if( m_Pitch > 89.0f )
				m_Pitch = 89.0f;
			if( m_Pitch < -89.0f )
				m_Pitch = -89.0f;

			// Update camera direction
			glm::vec3 direction{};
			direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
			direction.y = sin(glm::radians(m_Pitch));
			direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            Camera3D.Oriantaion = glm::normalize(direction);
			Camera3D.RefreshCameraMatrix();
        }

        return false;
    }

    bool MainCamera::OnMouseWheelScrollEvent(Motion::Core::WindowHandle handle, Motion::Core::EventMouseWheelScroll<float> & e)
    {
        Camera3D.PerspectiveFov -= (float)e.OffsetY();
        if (Camera3D.PerspectiveFov < 1.0f) Camera3D.PerspectiveFov = 1.0f;
        if (Camera3D.PerspectiveFov > 45.0f) Camera3D.PerspectiveFov = 45.0f;
        return false;
    }
}