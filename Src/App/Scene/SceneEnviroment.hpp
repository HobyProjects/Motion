#pragma once

#include <memory>
#include <glm/glm.hpp>

#include "Components.hpp"

namespace Motion
{
    struct DirectLight
    {
        glm::vec3 Direction{ -100.0f, -100.0f, -50.0f };
        glm::vec3 Color{ 1.0f,  1.0f,  1.0f };
        float Intensity{ 1.0f };

        bool CastShadow{false};
        bool ShowLightDirectionGuizmo{false};

        void SetFromPosition(const glm::vec3& position, const glm::vec3& target = glm::vec3(0.0f))
        {
            Direction = glm::normalize(target - position);
        }
    };

    struct PhysicsMaterialDefaults 
    {
        float Friction            = 0.5f;    // unitless
        float Bounciness          = 0.2f;    // restitution [0..1]
        float RollingResistance   = 0.01f;   // small torque-like friction
        float Density             = 500.0f;  // kg/m^3 (wood-ish)
    };

    struct PhysicsSleepSettings 
    {
        bool  Allowed             = true;
        float LinearThreshold     = 0.05f;   // m/s
        float AngularThreshold    = 0.05f;   // rad/s
        float Time                = 0.5f;    // s under thresholds before sleep
    };

    struct PhysicsSolverSettings 
    {
        int   VelocityIterations  = 15;
        int   PositionIterations  = 6;
        float AllowedPenetration  = 0.005f;  // m
        float BaumgarteBias       = 0.2f;    // modest positional correction
        float BounceVelThreshold  = 0.5f;    // m/s below which bounce is suppressed
    };

    struct PhysicsClampSettings 
    {
        float MaxLinearSpeed      = 200.0f;  // m/s
        float MaxAngularSpeed     = 200.0f;  // rad/s
    };

    struct PhysicsTimingSettings 
    {
        float FixedTimeStep       = 1.0f / 120.0f; // s
        int   MaxSubsteps         = 4;
    };

    struct PhysicsLayerSettings 
    {
        std::array<const char*, 32> Names{};
        std::array<uint32_t,    32> Masks{};
        uint8_t DefaultLayer = 0;
        uint32_t DefaultMask = 0xFFFFFFFFu;
    };

    struct PhysicsCCDSettings 
    {
        bool  Enabled             = false;
        float MinSpeed            = 25.0f;   // m/s -> treat as fast mover
        float RadiusBias          = 0.05f;   // expand sweep radius slightly
        int   MaxIterations       = 2;
    };

    struct ScenePhysics 
    {
        glm::vec3               Gravity{Units::SI_GRAVITY};
        PhysicsTimingSettings   timing{};
        PhysicsSolverSettings   solver{};
        PhysicsSleepSettings    sleep{};
        PhysicsMaterialDefaults material{};
        PhysicsClampSettings    clamps{};
        PhysicsLayerSettings    layers{};
        PhysicsCCDSettings      ccd{};

        // Debug toggles
        bool DrawContacts       = false;
        bool DrawAABBs          = false;
        bool DrawSleeping       = false;
    };

    struct SceneEnvironment
    {
        DirectLight Sun{};
        std::shared_ptr<IEnvironment> EnvironmentInstance;
        ScenePhysics  Physics;
    };
}