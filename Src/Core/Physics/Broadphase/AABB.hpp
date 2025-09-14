#pragma once

#include "PhyCore.hpp"

namespace Motion
{
    inline AABB FromCenterExtent(const glm::vec3& c, const glm::vec3& e)
    {
        return {c - e, c + e};
    };

    inline glm::vec3 AABBExtent(const AABB& b) 
    {
        return (b.MAX - b.MIN) * 0.05f;
    }

    inline glm::vec3 AABBCenter(const AABB& b)
    {
        return (b.MAX + b.MIN) * 0.5f;
    }

    inline bool Overlap(const AABB& a, const AABB& b)
    {
        return (a.MIN.x <= b.MAX.x && a.MAX.x >= b.MIN.x) &&
               (a.MIN.y <= b.MAX.y && a.MAX.y >= b.MIN.y) &&
               (a.MIN.z <= b.MAX.z && a.MAX.z >= b.MIN.z);
    }

    inline AABB Merge(const AABB& a, const AABB& b) 
    {
        return { glm::min(a.MIN, b.MIN), glm::max(a.MAX, b.MAX) };
    }

    inline bool RayIntersectsAABB(const glm::vec3& ro, const glm::vec3& rd, const AABB& b, float tMin, float tMax) 
    {
        glm::vec3 invD  = 1.0f / rd;
        glm::vec3 t0    = (b.MIN - ro) * invD;
        glm::vec3 t1    = (b.MAX - ro) * invD;
        glm::vec3 tmin  = glm::min(t0, t1);
        glm::vec3 tmax  = glm::max(t0, t1);

        float enter = glm::max(glm::max(tmin.x, tmin.y), glm::max(tmin.z, tMin));
        float exit  = glm::min(glm::min(tmax.x, tmax.y), glm::min(tmax.z, tMax));
        return exit >= enter && exit >= 0.0f;
    }

}
