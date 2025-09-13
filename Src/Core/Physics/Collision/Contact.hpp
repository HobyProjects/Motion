#pragma once

#include <glm/glm.hpp>
#include "PhyCore.hpp"

namespace Motion
{
    struct ContactPoint
    {
        glm::vec3 PositionWS{0.0f};
        glm::vec3 NormalWS{0.0f};
        float Penetration{0.0f};
        MaterialProperties MProps{};
    };

    struct ContactManifold
    {
        std::int32_t Count{4};
        ContactPoint Points[4];
        glm::vec3 SharedNormalWS;
        float SharedFriction{0.6f};
        float SharedRestitution{0.1f};
    };

    struct NarrowPhaseContext
    {
        float LinearSlop{1e-3};
        float AngularSlop{1e-3};
    };
}