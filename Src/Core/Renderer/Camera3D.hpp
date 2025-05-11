#pragma once

#include "Camera.hpp"

namespace Motion::Core
{
    class Camera3D
    {
        public:
            Camera3D() = default;
            Camera3D(float width, float height);
            ~Camera3D() = default;

            glm::mat4 GetProjection() const { return m_Projection; }
            glm::mat4 GetView() const { return m_View; }
            glm::mat4 GetViewProjectionMatrix() const { return m_MVP; }
            glm::vec3 GetPosition() const { return m_Position; }
            float GetRotation() const { return m_Rotation; }
            float GetAspectRatio() const { return m_AspectRatio; }
            float GetPerspectiveFov() const { return m_PerspectiveFov; }
            float GetPerspectiveNear() const { return m_PerspectiveNear; }
            float GetPerspectiveFar() const { return m_PerspectiveFar; }       
            glm::vec3 GetOriantaion() const { return m_Oriantaion; }
            glm::vec3 GetWorldUp() const { return m_WorldUp; }

            void SetAspectRatio(float width, float height);
            void SetRotation(float rotation);
            void SetPosition(const glm::vec3& position);
            void SetFOV(float fov);
            void SetNearClip(float nearClip);
            void SetFarClip(float farClip);
            void SetOriantaion(const glm::vec3& oriantaion);
            void SetPerspective(float fov = 45.0f, float nearClip = 0.01f, float farClip = 100.0f);
            
        private:
            void RefreshProjectionMatrix();

        private:
            float m_AspectRatio{ 1.0f };
            float m_PerspectiveFov{ 45.0f };
            float m_PerspectiveNear{ 0.01f };
            float m_PerspectiveFar{ 10000.0f };
            float m_Rotation{ 0.0f };
            float m_TranslationSpeed{ 2.0f };
            float m_Sensitivity{ 0.2f };
            float m_ViewportWidth{ 0.0f };
            float m_ViewportHeight{ 0.0f };

            glm::vec3 m_Position{ 0.0f, 0.0f, 3.0f };
            glm::vec3 m_Oriantaion{ 0.0f, 0.0f, -1.0f };
            glm::vec3 m_WorldUp{ 0.0f, 1.0f, 0.0f };
            glm::mat4 m_Projection{ 1.0f };
            glm::mat4 m_View{ 1.0f };
            glm::mat4 m_MVP{ 1.0f };
    };
}