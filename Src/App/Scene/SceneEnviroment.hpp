#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Components.hpp"

namespace Motion::App
{
    struct DirectionalLight
    {
        glm::vec3 Direction{ 0.0f, -1.0f, 0.0f };
        glm::vec3 Color{ 1.0f, 1.0f, 1.0f };
        float AmbientIntensity{ 1.0f };

        DirectionalLight() = default;
        ~DirectionalLight() = default;
    };

    struct PhysicsAttributes
    {
        bool IsEnabled{ true };
        glm::vec3 Gravity{ 0.0f, -9.81f, 0.0f };
        float FixedTimeStep{ 0.016f };
    };

    class PhysicsWorld
    {
    public:
        PhysicsWorld() = default;

        void SetSettings(const PhysicsAttributes& settings) { m_Settings = settings; }
        PhysicsAttributes& GetSettings() { return m_Settings; }
        void Update(std::shared_ptr<Motion::Core::Entity> entity, float deltaTime);

    private:
        void EnvironmentIntegration(Motion::Core::TransformComponent& transform, Motion::Core::PhysicsBodyComponent& body, float deltaTime);

    private:
        PhysicsAttributes m_Settings{};
    };

    enum class SimulationMode
    {
        Realtime,
        ManualStep
    };

    struct SceneEnviroment
    {
        DirectionalLight DirectionalLight{};
        PhysicsWorld Physics{};
        SimulationMode SimMode{ SimulationMode::Realtime };
        bool StepModeEnabled{ false };
    };

}