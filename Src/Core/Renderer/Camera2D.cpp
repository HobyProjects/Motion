#include "CorePCH.hpp"

namespace Motion::Core
{
    Camera2D::Camera2D(float aspectRatio, bool enableRotation)
        : m_AspectRatio(aspectRatio), m_IsRotationEnabled(enableRotation)
    {
        RefreshProjectionMatrix();
    }

    void Camera2D::SetAspectRatio(float ratio)
    {
        m_AspectRatio = ratio;
        RefreshProjectionMatrix();
    }

    void Camera2D::SetRotation(float rotation)
    {
        m_Rotation = rotation;
        RefreshProjectionMatrix();
    }

    void Camera2D::SetPosition(const glm::vec3& position)
    {
        m_Position = position;
        RefreshProjectionMatrix();
    }

    void Camera2D::SetProjection(float left, float right, float bottom, float top)
    {
        RefreshProjectionMatrix(left, right, bottom, top);
    }

    void Camera2D::SetProjection(const CameraBounds& bounds)
    {
        m_CameraBounds = bounds;
        RefreshProjectionMatrix(bounds);
    }

    void Camera2D::SetProjection(const glm::mat4& projection)
    {
        m_Projection = projection;
        RefreshProjectionMatrix();
    }

    void Camera2D::UpdateProjectionMatrix()
    {
        RefreshProjectionMatrix();
    }

    void Camera2D::RefreshProjectionMatrix()
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) * glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), { 0.0f, 0.0f, 1.0f });
        m_View = inverse(transform);

        float left = -m_AspectRatio * m_ZoomLevel;
        float right = m_AspectRatio * m_ZoomLevel;
        float top = m_ZoomLevel;
        float bottom = -m_ZoomLevel;

        m_Projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
        m_MVP = m_View * m_Projection;
    }

    void Camera2D::RefreshProjectionMatrix(float left, float right, float bottom, float top)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) * glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), { 0.0f, 0.0f, 1.0f });
        m_View = inverse(transform);

        m_Projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
        m_MVP = m_View * m_Projection;
    }

    void Camera2D::RefreshProjectionMatrix(const CameraBounds& bounds)
    {
        m_CameraBounds = bounds;

        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) * glm::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), { 0.0f, 0.0f, 1.0f });
        m_View = inverse(transform);

        m_Projection = glm::ortho(bounds.Left, bounds.Right, bounds.Bottom, bounds.Top, -1.0f, 1.0f);
        m_MVP = m_View * m_Projection;
    }
}