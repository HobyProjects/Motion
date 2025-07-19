#include "CorePCH.hpp"

namespace Motion::Core
{
    /**
     * @brief Recalculates the camera's view, projection, and MVP matrices.
     *
     * This function updates the camera's View matrix using the current position, orientation, and world up vector.
     * It also recalculates the Projection matrix based on the camera's field of view, aspect ratio, and near/far planes.
     * Finally, it computes the Model-View-Projection (MVP) matrix as the product of the Projection and View matrices.
     *
     * This should be called whenever the camera's position, orientation, or projection parameters change.
     */
    void Camera3D::RefreshCameraMatrix()
    {
        View = Projection = { 1.0f };
        View = glm::lookAt(Position, Position + Oriantaion, WorldUp);
        Projection = glm::perspective(glm::radians(PerspectiveFov), AspectRatio, PerspectiveNear, PerspectiveFar);
        MVP = Projection * View;
    }
}