#pragma once

#include <glm/glm.hpp>

namespace Motion
{
    struct ContactPoint
    {
        glm::vec3 PositionWS{0.0f};
        glm::vec3 NormalWS{0.0f};
        float Penetration{0.0f};
    };

    struct ContactManifold
    {
        ContactPoint Points[4];
        std::int32_t Count{4};
    };

    struct NarrowPhaseContext
    {
        float LinearSlop{1e-3};
        float AngularSlop{1e-3};
    };
}