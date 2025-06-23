#include "CorePCH.hpp"

namespace Motion::Core
{
    void Camera3D::RefreshCameraMatrix()
    {
        View = Projection = { 1.0f };
        View = glm::lookAt(Position, Position + Oriantaion, WorldUp);
        Projection = glm::perspective(glm::radians(PerspectiveFov), AspectRatio, PerspectiveNear, PerspectiveFar);
        MVP = Projection * View;
    }
}