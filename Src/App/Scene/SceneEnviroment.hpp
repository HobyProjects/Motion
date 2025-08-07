#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Components.hpp"

namespace Motion
{
    struct DirectionalLight
    {
        static constexpr std::int32_t LIGHT_COUNT = 4;
        UUID LightID{ UniqueIdentity::GetUniqueID() };
        glm::vec3 LightPosition[LIGHT_COUNT]
        {
            { 100.0f,  400.0f, 100.0f },
            { -300.0f, 200.0f, 50.0f },
            { 500.0f, 100.0f, -100.0f },
            { 0.0f,    1000.0f, 0.0f }
        };

        glm::vec3 LightColor[LIGHT_COUNT]
        {
            { 1.0f, 0.8f, 0.6f },
            { 0.6f, 0.8f, 1.0f },
            { 0.8f, 1.0f, 0.6f },
            { 1.0f, 1.0f, 1.0f }
        };

        float LightIntensity[LIGHT_COUNT]
        {
            1.0f, 0.8f, 0.6f, 0.5f
        };

        DirectionalLight() = default;
        ~DirectionalLight() = default;
    };

    struct SceneEnvironment
    {
        DirectionalLight DirectionalLight{};
    };

}