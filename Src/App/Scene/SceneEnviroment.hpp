#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Components.hpp"

namespace Motion
{
    struct SunLight
    {
        glm::vec3 Direction{ -100.0f, -100.0f, -50.0f };
        glm::vec3 Color{ 1.0f,  1.0f,  1.0f };
        float Intensity{ 10.0f };

        void SetFromPosition(const glm::vec3& position, const glm::vec3& target = glm::vec3(0.0f))
        {
            Direction = glm::normalize(target - position);
        }
    };

    struct SceneEnvironment
    {
        SunLight Sun{};
        std::shared_ptr<IEnvironment> EnvironmentInstance;

        float Exposure{ 1.0f };
        float Gamma{ 2.2f };
        glm::vec3 AmbientTint{ 1.0f, 1.0f, 1.0f };

        struct Fog
        {
            bool Enabled{ false };
            float Density{ 0.0f };
            glm::vec3 Color{ 0.6f, 0.7f, 0.8f };

        } FogSettings{};
    };
}