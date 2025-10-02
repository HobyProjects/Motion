#include "CorePCH.hpp"

namespace Motion
{
    void Camera3D::RefreshCameraMatrix()
    {
        View = Projection = { 1.0f };
        View = glm::lookAt(Position, Position + Oriantaion, WorldUp);
        Projection = glm::perspective(glm::radians(PerspectiveFov), AspectRatio, PerspectiveNear, PerspectiveFar);
        MVP = Projection * View;
    }
    void Camera3D::SetAspectRatio(float width, float height)
    {
        if (height == 0) height = 1;
        AspectRatio = width / height;
        ViewportWidth = width;
        ViewportHeight = height;
        RefreshCameraMatrix();
    }

    void Camera3D::LookAt(const glm::vec3& target, const glm::vec3& eye)
    {
        if (eye != glm::vec3(-1))
            Position = eye;
        Oriantaion = glm::normalize(target - Position);
        RefreshCameraMatrix();
    }

}