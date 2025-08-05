#include "CorePCH.hpp"

namespace Motion
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

    /**
     * @brief Sets the camera to look at a target position from a specified eye position.
     *
     * Updates the camera's position and orientation so that it faces the given target.
     * If the provided eye position is not equal to (-1, -1, -1), the camera's position is set to the eye.
     * The orientation is recalculated as the normalized vector from the camera's position to the target.
     * Finally, the camera matrix is refreshed to reflect the new orientation and position.
     *
     * @param target The position in world space to look at.
     * @param eye The position in world space from which to look at the target.
     */
    void Camera3D::LookAt(const glm::vec3& target, const glm::vec3& eye)
    {
        if (eye != glm::vec3(-1))
            Position = eye;
        Oriantaion = glm::normalize(target - Position);
        RefreshCameraMatrix();
    }

}