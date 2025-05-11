#include "CorePCH.hpp"

namespace Motion::Core
{
    Camera3D::Camera3D(float width, float height)
    {
        m_AspectRatio = width / height;
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        RefreshProjectionMatrix();
    }

    void Camera3D::SetAspectRatio(float width, float height)
    {
        m_AspectRatio = width / height;
        m_ViewportWidth = (uint32_t)width;
        m_ViewportHeight = (uint32_t)height;
        RefreshProjectionMatrix();
    }

    void Camera3D::SetRotation(float rotation)
    {
        m_Rotation = rotation;
        RefreshProjectionMatrix();
    }

    void Camera3D::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        RefreshProjectionMatrix();
    }

    void Camera3D::SetFOV(float fov)
    {
        m_PerspectiveFov = fov;
        RefreshProjectionMatrix();
    }

    void Camera3D::SetNearClip(float nearClip)
    {
        m_PerspectiveNear = nearClip;
        RefreshProjectionMatrix();
    }

    void Camera3D::SetFarClip(float farClip)
    {
        m_PerspectiveFar = farClip;
        RefreshProjectionMatrix();
    }

    void Camera3D::SetOriantaion(const glm::vec3& oriantaion)
    {
        m_Oriantaion = oriantaion;
        RefreshProjectionMatrix();
    }

    void Camera3D::SetPerspective(float fov, float nearClip, float farClip)
    {
        m_PerspectiveFov	= fov;
        m_PerspectiveNear	= nearClip;
        m_PerspectiveFar	= farClip;
        RefreshProjectionMatrix();
    }

    void Camera3D::RefreshProjectionMatrix()
    {
        m_View = m_Projection = { 1.0f };
        m_View = glm::lookAt(m_Position, m_Position + m_Oriantaion, m_WorldUp);
        m_Projection = glm::perspective(glm::radians(m_PerspectiveFov), m_AspectRatio, m_PerspectiveNear, m_PerspectiveFar);
        m_MVP = m_Projection * m_View;
    }
}