#pragma once

#include "Camera.hpp"
#include "Camera3D.hpp"
#include "Timer.hpp"
#include "Event.hpp"

namespace Motion
{
    class SceneCamera
    {
    public:
        SceneCamera(float viewportWidth, float viewportHeight, bool rotationEnabled = false);
        ~SceneCamera() = default;

        void SetAspectRatio(float width, float height);
        void OnUpdate(WindowHandle handle, Timer deltaTime);
        void OnEvents(WindowHandle handle, IEvent& e);

    private:
        bool OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove<float>& e);
        bool OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll<float>& e);

    public:
        Camera3D Camera3D;

    private:
        float m_MouseX = 0.0f, m_MouseY = 0.0f;
        float m_Yaw = -90.0f, m_Pitch = 0.0f;
    };
}