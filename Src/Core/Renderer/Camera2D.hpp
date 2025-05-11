#pragma once

#include "Camera.hpp"

namespace Motion::Core
{
    class Camera2D
    {
        public:
            Camera2D() = default;
            Camera2D(float aspectRatio, bool enableRotation);
            ~Camera2D() = default;

            glm::mat4 GetProjection() const { return m_Projection; }
            glm::mat4 GetView() const { return m_View; }
            glm::mat4 GetViewProjectionMatrix() const { return m_MVP; }
            glm::vec3 GetPosition() const { return m_Position; }
            float GetRotation() const { return m_Rotation; }
            float GetAspectRatio() const { return m_AspectRatio; }
            CameraBounds GetCameraBounds() const { return m_CameraBounds; }
            CamerType GetCameraType() const { return CamerType::Camera2D; }

            void SetAspectRatio(float ratio);
            void SetRotation(float rotation);
            void SetPosition(const glm::vec3& position);
            void SetProjection(float left, float right, float bottom, float top);
            void SetProjection(const CameraBounds& bounds);
            void SetProjection(const glm::mat4& projection);

            void UpdateProjectionMatrix();

        private:
            void RefreshProjectionMatrix();
            void RefreshProjectionMatrix(float left, float right, float bottom, float top);
            void RefreshProjectionMatrix(const CameraBounds& bounds);

        private:
            float m_AspectRatio{ 1.0f };
            float m_Rotation{ 0.0f };

            float m_TranslationSpeed{ 1.0f };
            float m_RotationSpeed{ 10.0f };
            float m_ZoomLevel{ 1.0f };

            glm::mat4 m_View{ 1.0f };
            glm::mat4 m_Projection{ 1.0f };
            glm::mat4 m_MVP{ 1.0f };
            glm::vec3 m_Position{ 0.0f, 0.0f, 0.0f };

            CameraBounds m_CameraBounds;
            bool m_IsRotationEnabled{ false };
    };
}