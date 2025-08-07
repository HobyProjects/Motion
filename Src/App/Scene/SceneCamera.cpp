#include "CorePCH.hpp"
#include "SceneCamera.hpp"

namespace Motion
{
    SceneCamera::SceneCamera(float viewportWidth, float viewportHeight, bool rotationEnabled)
    {
        Camera.AspectRatio = viewportWidth / viewportHeight;
        Camera.ViewportWidth = viewportWidth;
        Camera.ViewportHeight = viewportHeight;
        Camera.RotationEnabled = rotationEnabled;
        Camera.Position = glm::vec3(0.0f, 0.0f, 50.0f);
        Camera.RefreshCameraMatrix();
    }

    void SceneCamera::SetAspectRatio(float width, float height)
    {
        Camera.AspectRatio = width / height;
        Camera.ViewportWidth = width;
        Camera.ViewportHeight = height;
        Camera.RefreshCameraMatrix();
    }

    void SceneCamera::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        if (!(InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED))
            return;

        glm::vec3 forward = glm::normalize(Camera.Oriantaion);
        glm::vec3 right = glm::normalize(glm::cross(forward, Camera.WorldUp));

        if (InputsHandler::GetKeyState(handle, KEY_W))
            Camera.Position += forward * Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        if (InputsHandler::GetKeyState(handle, KEY_S))
            Camera.Position -= forward * Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        if (InputsHandler::GetKeyState(handle, KEY_A))
            Camera.Position -= right * Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        if (InputsHandler::GetKeyState(handle, KEY_D))
            Camera.Position += right * Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        if (InputsHandler::GetKeyState(handle, KEY_LEFT_CONTROL))
            Camera.Position.y -= Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        if (InputsHandler::GetKeyState(handle, KEY_SPACE))
            Camera.Position.y += Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();

        Camera.RefreshCameraMatrix();
    }

    void SceneCamera::OnEvents(WindowHandle handle, IEvent& e)
    {
        EventHandler handler(handle, e);
        handler.Dispatch<EventMouseCursorMove>(EVENT_CALLBACK(OnMouseCursorPosChange));
        handler.Dispatch<EventMouseWheelScroll>(EVENT_CALLBACK(OnMouseWheelScrollEvent));
    }

    bool SceneCamera::OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e)
    {
        static bool firstMouseMovement = true;

        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            float currentX = e.GetX();
            float currentY = e.GetY();

            if (firstMouseMovement)
            {
                m_MouseX = currentX;
                m_MouseY = currentY;
                firstMouseMovement = false;
                return false; // Prevent jump
            }

            float xOffset = currentX - m_MouseX;
            float yOffset = currentY - m_MouseY;

            m_MouseX = currentX;
            m_MouseY = currentY;

            xOffset *= Camera.Sensitivity;
            yOffset *= Camera.Sensitivity;

            m_Yaw += xOffset;
            m_Pitch -= yOffset;

            m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);

            glm::vec3 direction;
            direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            direction.y = sin(glm::radians(m_Pitch));
            direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

            Camera.Oriantaion = glm::normalize(direction);
            Camera.RefreshCameraMatrix();
        }
        else
        {
            firstMouseMovement = true; // Reset when not holding RMB
        }

        return false;
    }

    bool SceneCamera::OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e)
    {
        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            Camera.PerspectiveFov -= (float)e.OffsetY();
            if (Camera.PerspectiveFov < 1.0f) Camera.PerspectiveFov = 1.0f;
            if (Camera.PerspectiveFov > 45.0f) Camera.PerspectiveFov = 45.0f;
            Camera.RefreshCameraMatrix();
        }

        return false;
    }
}