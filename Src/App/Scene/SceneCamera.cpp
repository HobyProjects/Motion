#include "CorePCH.hpp"
#include "SceneCamera.hpp"

namespace Motion
{
    SceneCamera::SceneCamera(float viewportWidth, float viewportHeight, bool rotationEnabled)
    {
        SceneViewCamera.AspectRatio = viewportWidth / viewportHeight;
        SceneViewCamera.ViewportWidth = viewportWidth;
        SceneViewCamera.ViewportHeight = viewportHeight;
        SceneViewCamera.RotationEnabled = rotationEnabled;
        SceneViewCamera.Position = glm::vec3(0.0f, 0.0f, 0.0f);
        SceneViewCamera.RefreshCameraMatrix();
    }

    void SceneCamera::SetAspectRatio(float width, float height)
    {
        SceneViewCamera.AspectRatio = width / height;
        SceneViewCamera.ViewportWidth = width;
        SceneViewCamera.ViewportHeight = height;
        SceneViewCamera.RefreshCameraMatrix();
    }

    void SceneCamera::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            KeyState wKeyPressed = InputsHandler::GetKeyState(handle, KEY_W);
            if ((wKeyPressed & KEY_PRESSED) || (wKeyPressed & KEY_REPEAT))
            {
                SceneViewCamera.Position += SceneViewCamera.TranslationSpeed * SceneViewCamera.Oriantaion;
                SceneViewCamera.RefreshCameraMatrix();
            }

            KeyState sKeyPressed = InputsHandler::GetKeyState(handle, KEY_S);
            if ((sKeyPressed & KEY_PRESSED) || (sKeyPressed & KEY_REPEAT))
            {
                SceneViewCamera.Position += SceneViewCamera.TranslationSpeed * -SceneViewCamera.Oriantaion;
                SceneViewCamera.RefreshCameraMatrix();
            }

            KeyState aKeyPressed = InputsHandler::GetKeyState(handle, KEY_A);
            if ((aKeyPressed & KEY_PRESSED) || (aKeyPressed & KEY_REPEAT))
            {
                SceneViewCamera.Position += SceneViewCamera.TranslationSpeed * -glm::normalize(glm::cross(SceneViewCamera.Oriantaion, SceneViewCamera.WorldUp));
                SceneViewCamera.RefreshCameraMatrix();
            }

            KeyState dKeyPressed = InputsHandler::GetKeyState(handle, KEY_D);
            if ((dKeyPressed & KEY_PRESSED) || (dKeyPressed & KEY_REPEAT))
            {
                SceneViewCamera.Position += SceneViewCamera.TranslationSpeed * glm::normalize(glm::cross(SceneViewCamera.Oriantaion, SceneViewCamera.WorldUp));
                SceneViewCamera.RefreshCameraMatrix();
            }

            KeyState ctrlKeyPressed = InputsHandler::GetKeyState(handle, KEY_LEFT_CONTROL);
            if ((ctrlKeyPressed & KEY_PRESSED) || (ctrlKeyPressed & KEY_REPEAT))
            {
                SceneViewCamera.Position += SceneViewCamera.TranslationSpeed * -SceneViewCamera.WorldUp;
                SceneViewCamera.RefreshCameraMatrix();
            }

            KeyState spaceKeyPressed = InputsHandler::GetKeyState(handle, KEY_SPACE);
            if ((spaceKeyPressed & KEY_PRESSED) || (spaceKeyPressed & KEY_REPEAT))
            {
                SceneViewCamera.Position += SceneViewCamera.TranslationSpeed * SceneViewCamera.WorldUp;
                SceneViewCamera.RefreshCameraMatrix();
            }
        }
    }

    void SceneCamera::OnEvents(WindowHandle handle, IEvent& e)
    {
        EventHandler handler(handle, e);
        handler.Dispatch<EventMouseCursorMove>(EVENT_CALLBACK(OnMouseCursorPosChange));
        handler.Dispatch<EventMouseWheelScroll>(EVENT_CALLBACK(OnMouseWheelScrollEvent));
    }

    bool SceneCamera::OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e)
    {
        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            float xOffset = e.GetX() - m_MouseX;
            float yOffset = e.GetY() - m_MouseY;  // Change this to invert camera control

            m_MouseX = e.GetX();
            m_MouseY = e.GetY();

            xOffset *= SceneViewCamera.Sensitivity;
            yOffset *= SceneViewCamera.Sensitivity;

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
            SceneViewCamera.Oriantaion = glm::normalize(direction);
            SceneViewCamera.RefreshCameraMatrix();
        }

        return false;
    }

    bool SceneCamera::OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e)
    {
        SceneViewCamera.PerspectiveFov -= (float)e.OffsetY();
        if (SceneViewCamera.PerspectiveFov < 1.0f) SceneViewCamera.PerspectiveFov = 1.0f;
        if (SceneViewCamera.PerspectiveFov > 45.0f) SceneViewCamera.PerspectiveFov = 45.0f;
        SceneViewCamera.RefreshCameraMatrix();

        return false;
    }
}