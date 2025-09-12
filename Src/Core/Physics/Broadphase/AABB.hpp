#pragma once

#include "Types.hpp"

namespace Motion
{
    inline AABB FromCenterExtent(const glm::vec3& c, const glm::vec3& e)
    {
        return {c - e, c + e};
    };

    inline glm::vec3 TransformPoint(const glm::mat4& M, const glm::vec3& p) 
    {
        return glm::vec3(M * glm::vec4(p, 1.0f));
    }

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
}
