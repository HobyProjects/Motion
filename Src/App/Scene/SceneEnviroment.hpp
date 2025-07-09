#pragma once

#include <glm/glm.hpp>

namespace Motion::App
{
    struct DirectionalLight
    {
        glm::vec3 Direction{ 0.0f, -1.0f, 0.0f };
        glm::vec3 Color{ 1.0f, 1.0f, 1.0f };
        float AmbientIntensity{1.0f};

        DirectionalLight() = default;
        ~DirectionalLight() = default;
    };

    struct EnviromentPhysics
    {
        bool IsEnabled{ true };
        glm::vec3 Gravity{ 0.0f, -9.81f, 0.0f };
        float TimeStep{ 0.016f };
    };


    struct SceneEnviroment
    {
        DirectionalLight DirectionalLight{};
        EnviromentPhysics Physics{};
    };

}