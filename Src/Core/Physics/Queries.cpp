#include "CorePCH.hpp"

namespace Motion
{

    RayCastHit RayCast(const std::vector<std::shared_ptr<Entity>>& e, const Ray& ray)
    {
        RayCastHit best{};
        float bestT = ray.MaxDistance;

        // Ensure AABBs are up-to-date (Broadphase does this each physics step)
        // If you need immediate freshness after a transform tweak outside the step,
        // recompute col.worldAABB here similarly to Broadphase.

        for(auto& entt : e)
        {
            if(entt->HasComponent<ColliderComponent>())
            {
                RayCastHit tmp{};
                if(RayCastEntity(entt, ray, tmp) && tmp.Distance < bestT)
                {
                    best    = tmp;
                    bestT   = tmp.Distance;
                }
            }
        }

        return best;
    }

    bool RayCastEntity(const std::shared_ptr<Entity>& entt, const Ray & ray, RayCastHit & out)
    {
        if(!entt->HasComponent<ColliderComponent>() || entt->HasComponent<TransformComponent>()) return false;

        auto& tr    = entt->GetComponent<TransformComponent>();
        auto& col   = entt->GetComponent<ColliderComponent>();

        // Fast reject by AABB
        float tmin;
        if (!RayAABB(ray.Origin, ray.Direction, ray.MaxDistance, col.WorldAABB, tmin))
            return false;

        float tHit = std::numeric_limits<float>::infinity();
        glm::vec3 nHit{0};
        bool hit = false;

        switch (col.Type) 
        {
            case ColliderType::Sphere: 
            {
                hit = RaySphere(ray.Origin, ray.Direction, ray.MaxDistance, tr.Translation, col.Sphere.Radius, tHit, nHit);
            } break;

            case ColliderType::Box: 
            {
                // Ray vs AABB is already done; here we refine using that t.
                // Compute normal by picking the dominant face at the hit point.
                float t;
                if (RayAABB(ray.Origin, ray.Direction, ray.MaxDistance, col.WorldAABB, t)) 
                {
                    tHit                    = t;
                    glm::vec3 p             = ray.Origin + tHit * ray.Direction;
                    const auto& aabb        = col.WorldAABB;
                    glm::vec3 c             = 0.5f * (aabb.MIN + aabb.MAX);
                    glm::vec3 he            = 0.5f * (aabb.MAX - aabb.MIN);
                    glm::vec3 local         = p - c;
                    glm::vec3 d             = glm::abs(local) - he;

                    // normal from the face we hit (axis of max penetration)
                    if (std::abs(d.x) > std::abs(d.y) && std::abs(d.x) > std::abs(d.z))
                        nHit = glm::vec3((local.x > 0 ? 1.f : -1.f), 0, 0);
                    else if (std::abs(d.y) > std::abs(d.z))
                        nHit = glm::vec3(0, (local.y > 0 ? 1.f : -1.f), 0);
                    else
                        nHit = glm::vec3(0, 0, (local.z > 0 ? 1.f : -1.f));
                    hit = true;
                }

            } break;

            case ColliderType::Capsule: 
            {
                glm::vec3 a,b; CapsuleWorldEnds(tr, col, a, b);
                hit = RayCapsule(ray.Origin, ray.Direction, ray.MaxDistance, a, b, col.Capsule.Radius, tHit, nHit);
            } break;

            default: break;
        }

        if (!hit) return false;

        out.Hit         = true;
        out.EnTT        = entt;
        out.Distance    = tHit;
        out.Point       = ray.Origin + tHit * ray.Direction;
        out.Normal      = nHit;
        return true;
    }
}