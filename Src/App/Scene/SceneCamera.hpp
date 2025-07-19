#pragma once

#include "Camera.hpp"
#include "Camera3D.hpp"
#include "Timer.hpp"
#include "Event.hpp"

namespace Motion::App
{
    class SceneCamera
    {
    public:
        SceneCamera(float viewportWidth, float viewportHeight, bool rotationEnabled = false);
        ~SceneCamera() = default;

        void SetAspectRatio(float width, float height);
        void OnUpdate(Motion::Core::WindowHandle handle, Motion::Core::Timer deltaTime);
        void OnEvents(Motion::Core::WindowHandle handle, Motion::Core::IEvent& e);
        glm::mat4 GetCameraMatrix() { return Camera3D.GetCameraMatrix(); }

    private:
        bool OnMouseCursorPosChange(Motion::Core::WindowHandle handle, Motion::Core::EventMouseCursorMove<float>& e);
        bool OnMouseWheelScrollEvent(Motion::Core::WindowHandle handle, Motion::Core::EventMouseWheelScroll<float>& e);

    public:
        Motion::Core::Camera3D Camera3D;

    private:
        float m_MouseX = 0.0f, m_MouseY = 0.0f;
        float m_Yaw = -90.0f, m_Pitch = 0.0f;
    };
}