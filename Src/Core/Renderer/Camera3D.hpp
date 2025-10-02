#pragma once

#include "Camera.hpp"

namespace Motion
{
    struct Camera3D
    {
        public:
            float AspectRatio{ 1.0f };                      // Square viewport by default
            float PerspectiveFov{ 45.0f };                  // 45 degrees is a common field of view for perspective cameras
            float PerspectiveNear{ 0.01f };                 // Very close near plane (prevents clipping artifacts at close range)
            float PerspectiveFar{ 10000.0f };               // Very far, covers most scene sizes
            float Rotation{ 0.0f };                         // No rotation by default
            float TranslationSpeed{ 0.005f };               // Standard move speed (can be tuned for your needs)
            float Sensitivity{ 0.2f };                      // Mouse or input sensitivity (tunable)
            float ViewportWidth{ 0.0f };                    // To be set when viewport is resized or set
            float ViewportHeight{ 0.0f };                   // To be set when viewport is resized or set
            float ZoomLevel{ 1.0f };                        // Default zoom (orthographic, or used to control FOV)
            bool  RotationEnabled{ false };                 // User cannot rotate by default

            glm::vec3 Position{ 0.0f, 0.0f, 3.0f };         // Typical default: 3 units forward in +Z
            glm::vec3 Oriantaion{ 0.0f, 0.0f, -1.0f };      // Looking towards -Z (standard OpenGL convention)
            glm::vec3 WorldUp{ 0.0f, 1.0f, 0.0f };          // Y is up in most engines

            glm::mat4 Projection{ 1.0f };                   // Identity, to be set by projection calculations
            glm::mat4 View{ 1.0f };                         // Identity, to be set by view calculation
            glm::mat4 MVP{ 1.0f };                          // Identity, to be set as Projection * View * Model


        public:
            Camera3D() = default;
            ~Camera3D() = default;

            glm::mat4 GetProjection() const { return Projection; }
            glm::mat4 GetView() const { return View; }
            glm::mat4 GetCameraMatrix() const { return MVP; }
            glm::vec3 GetPosition() const { return Position; }
            glm::vec3 GetOrientation() const { return Oriantaion; }
            void SetAspectRatio(float width, float height);
            void LookAt(const glm::vec3& target, const glm::vec3& eye = glm::vec3(-1));
            void RefreshCameraMatrix();
    };
}