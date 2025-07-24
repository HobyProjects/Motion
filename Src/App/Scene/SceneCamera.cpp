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
        if (!(InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED))
            return;

        glm::vec3 forward = glm::normalize(SceneViewCamera.Oriantaion);
        glm::vec3 right = glm::normalize(glm::cross(forward, SceneViewCamera.WorldUp));

        if (InputsHandler::GetKeyState(handle, KEY_W))
            SceneViewCamera.Position += forward * SceneViewCamera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        if (InputsHandler::GetKeyState(handle, KEY_S))
            SceneViewCamera.Position -= forward * SceneViewCamera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        if (InputsHandler::GetKeyState(handle, KEY_A))
            SceneViewCamera.Position -= right * SceneViewCamera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        if (InputsHandler::GetKeyState(handle, KEY_D))
            SceneViewCamera.Position += right * SceneViewCamera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();

        SceneViewCamera.RefreshCameraMatrix();
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

            xOffset *= SceneViewCamera.Sensitivity;
            yOffset *= SceneViewCamera.Sensitivity;

            m_Yaw += xOffset;
            m_Pitch -= yOffset;

            m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);

            glm::vec3 direction;
            direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            direction.y = sin(glm::radians(m_Pitch));
            direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

            SceneViewCamera.Oriantaion = glm::normalize(direction);
            SceneViewCamera.RefreshCameraMatrix();
        }
        else
        {
            firstMouseMovement = true; // Reset when not holding RMB
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