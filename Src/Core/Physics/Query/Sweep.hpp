#pragma once

#include <vector>
#include <limits>

#include "RayCast.hpp"
#include "SphereShape.hpp"
#include "CapsuleShape.hpp"
#include "BoxShape.hpp"

namespace Motion
{
    struct SweepSphere
    {
        glm::vec3 Origin{0.0f};
        glm::vec3 Dir{0.0f, 0.0f, 1.0f};
        float R{0.5f};
        float TMin{0.0f};
        float TMax{1e6f};
    };

    struct SweepCapsule 
    {
        glm::vec3 A0;     
        glm::vec3 B0;     
        float     R;    

        glm::vec3 Dir;    
        float     TMax;   
    };

    struct SweepHit
    {
        float T{std::numeric_limits<float>::infinity()};
        glm::vec3 Position{0.0f};
        glm::vec3 Normal{0.0f};
        const Collider* HitCollider{nullptr};
    };



    inline bool SweepSphereVsSphere(const SweepSphere& q, const glm::vec3& cB, float rB, SweepHit& out) 
    {
        Ray ray{ q.Origin, q.Dir, q.TMin, q.TMax };
        float t; glm::vec3 n;

        if (!RaySphere(ray, cB, rB + q.R, t, n)) return false;
        if (t < out.T) 
        {
            out.T = t; out.Position = ray.Origin + t * ray.Dir; out.Normal = n;
            return true;
        }

        return false;
    }

    inline bool SweepSphereVsCapsule(const SweepSphere& q, const glm::vec3& A, const glm::vec3& B, float rCap, SweepHit& out) 
    {
        Ray ray{ q.Origin, q.Dir, q.TMin, q.TMax };
        float t; glm::vec3 n;

        if (!RayCapsule(ray, A, B, rCap + q.R, t, n)) return false;
        if (t < out.T) 
        {
            out.T = t; out.Position = ray.Origin + t * ray.Dir; out.Normal = n;
            return true;
        }
        return false;
    }

    inline bool SweepSphereVsOBB(const SweepSphere& q, const glm::vec3& c, const glm::vec3 axes[3], const glm::vec3& e, SweepHit& out) 
    {
        glm::vec3 eInf = e + glm::vec3(q.R);
        Ray ray{ q.Origin, q.Dir, q.TMin, q.TMax };

        float t; glm::vec3 n;
        if (!RayOBB(ray, c, axes, eInf, t, n)) return false;
        if (t < out.T) 
        {
            out.T = t; out.Position = ray.Origin + t * ray.Dir; out.Normal = n;
            return true;
        }

        return false;
    }

    inline bool SweepSphereAgainstCollider(const SweepSphere& q, const Collider& col, const glm::mat4& world, SweepHit& out) 
    {
        switch (col.ColliderShape->Type) 
        {
            case ShapeType::Sphere: 
            {
                const auto& S       = *static_cast<const SphereShape*>(col.ColliderShape);
                const glm::vec3 cB  = SphereShape::WorldCenter(S, world);
                const float rB      = SphereShape::WorldRadius(S, world) + S.ConvexRadius;
                
                return SweepSphereVsSphere(q, cB, rB, out);
            }
            case ShapeType::Capsule: 
            {
                const auto& K = *static_cast<const CapsuleShape*>(col.ColliderShape);
                glm::vec3 A,B; float rCap; CapsuleShape::WorldSegment(K, world, A, B, rCap);
                rCap += K.ConvexRadius;
                return SweepSphereVsCapsule(q, A, B, rCap, out);
            }
            case ShapeType::Box: 
            {
                const auto& Bx = *static_cast<const BoxShape*>(col.ColliderShape);
                glm::vec3 axes[3], scales, cB;
                AxesScalesFromWorld(world, axes, scales, cB);
                glm::vec3 e = (Bx.HalfExtents + glm::vec3(Bx.ConvexRadius)) * scales;
                return SweepSphereVsOBB(q, cB, axes, e, out);
            }
        }

        return false;
    }

    inline bool SweepCapsuleApprox3SpheresAgainstCollider(const SweepCapsule& q, const Collider& col, const glm::mat4& world, SweepHit& out)
    {
        SweepSphere sA{ q.A0, q.Dir, q.R, 0.0f, q.TMax };
        SweepSphere sB{ q.B0, q.Dir, q.R, 0.0f, q.TMax };
        SweepSphere sM{ (q.A0 + q.B0)*0.5f, q.Dir, q.R, 0.0f, q.TMax };

        bool hit = false;
        SweepHit h; h.T = out.T;

        switch (col.ColliderShape->Type) 
        {
            case ShapeType::Sphere: 
            {
                const auto& S = *static_cast<const SphereShape*>(col.ColliderShape);
                glm::vec3 c  = SphereShape::WorldCenter(S, world);
                float     rB = SphereShape::WorldRadius(S, world) + S.ConvexRadius;

                hit |= SweepSphereVsSphere(sA, c, rB, h);
                hit |= SweepSphereVsSphere(sB, c, rB, h);
                hit |= SweepSphereVsSphere(sM, c, rB, h);
                break;
            }
            case ShapeType::Capsule: 
            {
                const auto& K = *static_cast<const CapsuleShape*>(col.ColliderShape);
                glm::vec3 A,B; float rCap; CapsuleShape::WorldSegment(K, world, A, B, rCap);
                rCap += K.ConvexRadius;

                hit |= SweepSphereVsCapsule(sA, A, B, rCap, h);
                hit |= SweepSphereVsCapsule(sB, A, B, rCap, h);
                hit |= SweepSphereVsCapsule(sM, A, B, rCap, h);
                break;
            }
            case ShapeType::Box: 
            {
                const auto& Bx = *static_cast<const BoxShape*>(col.ColliderShape);
                glm::vec3 axes[3], scales, cB;
                AxesScalesFromWorld(world, axes, scales, cB);
                glm::vec3 e = (Bx.HalfExtents + glm::vec3(Bx.ConvexRadius)) * scales;

                hit |= SweepSphereVsOBB(sA, cB, axes, e, h);
                hit |= SweepSphereVsOBB(sB, cB, axes, e, h);
                hit |= SweepSphereVsOBB(sM, cB, axes, e, h);
                break;
            }
        }

        if (hit && h.T < out.T) { out = h; return true; }
        return false;
    }



}