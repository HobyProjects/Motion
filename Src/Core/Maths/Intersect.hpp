#pragma once

#include <cfloat>
#include <glm/glm.hpp>

namespace Motion
{
    inline bool RayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir, const glm::vec3& aabbMin, const glm::vec3& aabbMax, float& tmin, float& tmax)
    {
        tmin = 0.0f;
        tmax = FLT_MAX;

        for (int i = 0; i < 3; ++i)
        {
            if (fabs(rayDir[i]) < 1e-8f)
            {
                // Ray is parallel to slab. No hit if origin not within slab
                if (rayOrigin[i] < aabbMin[i] || rayOrigin[i] > aabbMax[i])
                    return false;
            }
            else
            {
                float ood = 1.0f / rayDir[i];
                float t1 = (aabbMin[i] - rayOrigin[i]) * ood;
                float t2 = (aabbMax[i] - rayOrigin[i]) * ood;
                if (t1 > t2) std::swap(t1, t2);
                tmin = t1 > tmin ? t1 : tmin;
                tmax = t2 < tmax ? t2 : tmax;
                if (tmin > tmax) return false;
            }
        }

        return true;
    }
}