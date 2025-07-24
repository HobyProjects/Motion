#pragma once

#include "Camera.hpp"

namespace Motion
{
    struct Camera3D
    {
    public:
        float AspectRatio{ 1.0f };
        float PerspectiveFov{ 45.0f };
        float PerspectiveNear{ 0.01f };
        float PerspectiveFar{ 10000.0f };
        float Rotation{ 0.0f };
        float TranslationSpeed{ 1.0f };
        float Sensitivity{ 0.2f };
        float ViewportWidth{ 0.0f };
        float ViewportHeight{ 0.0f };
        float ZoomLevel{ 1.0f };
        bool RotationEnabled{ false };

        glm::vec3 Position{ 0.0f, 0.0f, 3.0f };
        glm::vec3 Oriantaion{ 0.0f, 0.0f, -1.0f };
        glm::vec3 WorldUp{ 0.0f, 1.0f, 0.0f };
        glm::mat4 Projection{ 1.0f };
        glm::mat4 View{ 1.0f };
        glm::mat4 MVP{ 1.0f };

    public:
        Camera3D() = default;
        ~Camera3D() = default;

        glm::mat4 GetProjection() const { return Projection; }
        glm::mat4 GetView() const { return View; }
        glm::mat4 GetCameraMatrix() const { return MVP; }
        void RefreshCameraMatrix();
    };
}