#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Components.hpp"

namespace Motion
{
    struct DirectionalLight
    {
        UUID LightID{ UniqueIdentity::GetUniqueID() };
        glm::vec3 Direction{ 0.0f, -1.0f, 0.0f };
        glm::vec3 Color{ 1.0f, 1.0f, 1.0f };
        float AmbientIntensity{ 1.0f };

        DirectionalLight() = default;
        ~DirectionalLight() = default;
    };

    struct SceneEnvironment
    {
        DirectionalLight DirectionalLight{};
    };

}