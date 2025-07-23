#include "CorePCH.hpp"
#include "SceneCamera.hpp"

namespace Motion
{
    SceneCamera::SceneCamera(float viewportWidth, float viewportHeight, bool rotationEnabled)
    {
        Camera3D.AspectRatio = viewportWidth / viewportHeight;
        Camera3D.ViewportWidth = viewportWidth;
        Camera3D.ViewportHeight = viewportHeight;
        Camera3D.RotationEnabled = rotationEnabled;
        Camera3D.RefreshCameraMatrix();
    }

    void SceneCamera::SetAspectRatio(float width, float height)
    {
        Camera3D.AspectRatio = width / height;
        Camera3D.ViewportWidth = width;
        Camera3D.ViewportHeight = height;
        Camera3D.RefreshCameraMatrix();
    }

    void SceneCamera::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        if (InputsHandler::GetMouseButtonState(handle, MouseButton::MOUSE_BUTTON_RIGHT) & (KeyState::KEY_PRESSED | KeyState::KEY_REPEAT))
        {
            if (InputsHandler::GetKeyState(handle, KeyCode::KEY_W) &
                (KeyState::KEY_PRESSED | KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * Camera3D.Oriantaion;
                Camera3D.RefreshCameraMatrix();
            }

            if (InputsHandler::GetKeyState(handle, KeyCode::KEY_S) &
                (KeyState::KEY_PRESSED | KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * -Camera3D.Oriantaion;
                Camera3D.RefreshCameraMatrix();
            }

            if (InputsHandler::GetKeyState(handle, KeyCode::KEY_A) &
                (KeyState::KEY_PRESSED | KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * -glm::normalize(glm::cross(Camera3D.Oriantaion, Camera3D.WorldUp));
                Camera3D.RefreshCameraMatrix();
            }

            if (InputsHandler::GetKeyState(handle, KeyCode::KEY_D) &
                (KeyState::KEY_PRESSED | KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * glm::normalize(glm::cross(Camera3D.Oriantaion, Camera3D.WorldUp));
                Camera3D.RefreshCameraMatrix();
            }

            if (InputsHandler::GetKeyState(handle, KeyCode::KEY_LEFT_CONTROL) &
                (KeyState::KEY_PRESSED | KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * -Camera3D.WorldUp;
                Camera3D.RefreshCameraMatrix();
            }

            if (InputsHandler::GetKeyState(handle, KeyCode::KEY_SPACE) &
                (KeyState::KEY_PRESSED | KeyState::KEY_REPEAT))
            {
                Camera3D.Position += Camera3D.TranslationSpeed * Camera3D.WorldUp;
                Camera3D.RefreshCameraMatrix();
            }
        }
    }

    void SceneCamera::OnEvents(WindowHandle handle, IEvent& e)
    {
        EventHandler handler(handle, e);
        handler.Dispatch<EventMouseCursorMove<float>>(EVENT_CALLBACK(OnMouseCursorPosChange));
        handler.Dispatch<EventMouseWheelScroll<float>>(EVENT_CALLBACK(OnMouseWheelScrollEvent));
    }

    bool SceneCamera::OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove<float>& e)
    {
        if (InputsHandler::GetMouseButtonState(handle, MouseButton::MOUSE_BUTTON_RIGHT) &
            (KeyState::KEY_PRESSED | KeyState::KEY_REPEAT))
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
            if (m_Pitch > 89.0f)
                m_Pitch = 89.0f;
            if (m_Pitch < -89.0f)
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

    bool SceneCamera::OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll<float>& e)
    {
        Camera3D.PerspectiveFov -= (float)e.OffsetY();
        if (Camera3D.PerspectiveFov < 1.0f) Camera3D.PerspectiveFov = 1.0f;
        if (Camera3D.PerspectiveFov > 45.0f) Camera3D.PerspectiveFov = 45.0f;
        return false;
    }
}