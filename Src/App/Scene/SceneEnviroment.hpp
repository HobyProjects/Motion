#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Components.hpp"

namespace Motion
{
    struct DirectLight
    {
        glm::vec3 Direction{ -100.0f, -100.0f, -50.0f };
        glm::vec3 Color{ 1.0f,  1.0f,  1.0f };
        float Intensity{ 1.0f };

        void SetFromPosition(const glm::vec3& position, const glm::vec3& target = glm::vec3(0.0f))
        {
            Direction = glm::normalize(target - position);
        }
    };

    struct SceneEnvironment
    {
        DirectLight Sun{};
        std::shared_ptr<IEnvironment> EnvironmentInstance;
    };
}