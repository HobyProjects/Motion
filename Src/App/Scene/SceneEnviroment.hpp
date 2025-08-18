#pragma once

#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Components.hpp"

namespace Motion
{
    struct SunLight
    {
        glm::vec3 Direction{ 0.0f, 0.0f, 0.0f };
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
        std::shared_ptr<IEnvironment> Env;

        float    Exposure{ 1.0f };
        float    Gamma{ 2.2f };
        glm::vec3 AmbientTint{ 1.0f, 1.0f, 1.0f };

        struct Fog
        {
            bool Enabled{ false };
            float Density{ 0.0f };
            glm::vec3 Color{ 0.6f, 0.7f, 0.8f };

        } FogSettings{};

        inline void SetIBLIntensity(float diffuse, float specular) noexcept { if (Env) Env->SetIntensity({ diffuse, specular }); }
        inline EnvIntensity GetIBLIntensity() const noexcept { return Env ? Env->GetIntensity() : EnvIntensity{}; }
        inline void SetSkyboxRotationY(float radians) noexcept { if (Env) Env->SetSkyboxRotationY(radians); }
        inline float GetSkyboxRotationY() const noexcept { return Env ? Env->GetSkyboxRotationY() : 0.0f; }
        inline void BindIBLAll(std::uint32_t irrSlot, std::uint32_t preSlot, std::uint32_t brdfSlot) const noexcept { if (Env) Env->BindIBLAll(irrSlot, preSlot, brdfSlot); }
    };
}