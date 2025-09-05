#pragma once

#include "Components.hpp"

namespace Motion
{
    inline AABB MakeWorldAABB(const TransformComponent& tr, const ColliderComponent& c)
    {
        if(c.Type == ColliderType::Sphere)
        {
            float r             = c.Sphere.Radius;
            glm::vec3 center    = tr.Translation;
            return { center - glm::vec3(r), center + glm::vec3(r) };
        }

        if(c.Type == ColliderType::Box)
        {
            glm::vec3 he = c.Box.HalfExtents;
            glm::vec3 center = tr.Translation;
            return { center - he, center + he };
        }

        return {};
    }

    inline glm::vec3 ClosestPointOnAABB(const glm::vec3& p, const AABB& aabb)
    {
        return glm::clamp(p, aabb.MIN, aabb.MAX);
    }

    inline glm::mat3 ToMat3(const glm::quat& q)
    {
        // If you already have utilities, use those. This is the standard conversion.
        const float x2 = q.x+q.x, y2 = q.y+q.y, z2 = q.z+q.z;
        const float xx = q.x*x2,  yy = q.y*y2,  zz = q.z*z2;
        const float xy = q.x*y2,  xz = q.x*z2,  yz = q.y*z2;
        const float wx = q.w*x2,  wy = q.w*y2,  wz = q.w*z2;

        glm::mat3 m;
        m[0][0] = 1.0f - (yy + zz);
        m[0][1] = xy + wz;
        m[0][2] = xz - wy;
        m[1][0] = xy - wz;
        m[1][1] = 1.0f - (xx + zz);
        m[1][2] = yz + wx;
        m[2][0] = xz + wy;
        m[2][1] = yz - wx;
        m[2][2] = 1.0f - (xx + yy);
        return m;
    }

    inline glm::mat3 InvInertiaWorld(const Motion::RigidBodyComponent& rb, const glm::quat& qWorld)
    {
        // I^-1_world = R * I^-1_body * R^T  (diagonal in body space)
        const glm::mat3 R           = ToMat3(qWorld);
        const glm::mat3 I_body_inv  = glm::mat3(
            rb.InvInertiaDiag.x, 0, 0,
            0, rb.InvInertiaDiag.y, 0,
            0, 0, rb.InvInertiaDiag.z
        );
        
        return R * I_body_inv * glm::transpose(R);
    }

    inline float Combine(float a, float b, CombineMode mode)
    {
        using CM = Motion::CombineMode;
        switch (mode) 
        {
            case CM::Average:  return 0.5f * (a + b);
            case CM::Minimum:  return std::min(a, b);
            case CM::Maximum:  return std::max(a, b);
            case CM::Multiply: return a * b;
        }

        return 0.5f * (a + b);
    }

    inline float CombineSym(float a, float b, CombineMode modeA, CombineMode modeB)
    {
        const float va = Combine(a, b, modeA);
        const float vb = Combine(a, b, modeB);
        return 0.5f * (va + vb); // symmetric & predictable
    }

    struct PairMaterial 
    {
        float Restitution;
        float MU_S;
        float MU_D;
    };

    inline PairMaterial MakePairMaterial(const ColliderComponent& A, const ColliderComponent& B)
    {
        PairMaterial pm{};
        pm.Restitution = CombineSym(A.MaterialBase.Restitution,     B.MaterialBase.Restitution,     A.MaterialBase.RestitutionCombine, B.MaterialBase.RestitutionCombine);
        pm.MU_S        = CombineSym(A.MaterialBase.FrictionStatic,  B.MaterialBase.FrictionStatic,  A.MaterialBase.FrictionCombine,    B.MaterialBase.FrictionCombine);
        pm.MU_D        = CombineSym(A.MaterialBase.FrictionDynamic, B.MaterialBase.FrictionDynamic, A.MaterialBase.FrictionCombine,    B.MaterialBase.FrictionCombine);
        return pm;
    }

    inline void CapsuleWorldEnds(const Motion::TransformComponent& tr, const ColliderComponent& c, glm::vec3& aOut, glm::vec3& bOut)
    {
        const glm::vec3 up = ToMat3(tr.Rotation) * glm::vec3(0,1,0);
        aOut = tr.Translation + up * c.Capsule.HalfHeight;
        bOut = tr.Translation - up * c.Capsule.HalfHeight;
    }

    inline AABB CapsuleWorldAABB(const Motion::TransformComponent& tr, const ColliderComponent& c)
    {
        glm::vec3 a, b; CapsuleWorldEnds(tr, c, a, b);
        const glm::vec3 r(c.Capsule.Radius);
        Motion::AABB out;
        out.MIN = glm::min(a, b) - r;
        out.MAX = glm::max(a, b) + r;
        return out;
    }

    inline glm::vec3 ClosestPointOnSegment(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b)
    {
        const glm::vec3 ab = b - a;
        const float ab2 = glm::dot(ab, ab);
        if (ab2 <= 1e-12f) return a; // degenerate
        const float t = glm::clamp(glm::dot(p - a, ab) / ab2, 0.0f, 1.0f);
        return a + t * ab;
    }

    inline bool RayAABB(const glm::vec3& ro, const glm::vec3& rd, float tmax, const Motion::AABB& aabb, float& tminOut)
    {
        // Slab method
        glm::vec3 inv = 1.0f / rd;
        glm::vec3 t0  = (aabb.MIN - ro) * inv;
        glm::vec3 t1  = (aabb.MAX - ro) * inv;
        glm::vec3 tsm = glm::min(t0, t1);
        glm::vec3 tbg = glm::max(t0, t1);

        float tmin = std::max({tsm.x, tsm.y, tsm.z, 0.0f});
        float tmaxHit = std::min({tbg.x, tbg.y, tbg.z, tmax});
        if (tmaxHit >= tmin) { tminOut = tmin; return true; }
        return false;
    }

    inline bool RaySphere(const glm::vec3& ro, const glm::vec3& rd, float tmax, const glm::vec3& center, float radius, float& tHit, glm::vec3& nhit)
    {
        glm::vec3 oc    = ro - center;
        float b         = glm::dot(oc, rd);
        float c         = glm::dot(oc, oc) - radius*radius;
        float disc      = b*b - c;

        if (disc < 0.0f) return false;

        float s = std::sqrt(disc);
        float t = -b - s;

        if (t < 0.0f) t = -b + s;
        if (t < 0.0f || t > tmax) return false;

        glm::vec3 p = ro + t * rd;
        nhit        = glm::normalize(p - center);
        tHit        = t;

        return true;
    }

    inline bool RayCapsule(const glm::vec3& ro, const glm::vec3& rd, float tmax, const glm::vec3& a, const glm::vec3& b, float radius, float& tHit, glm::vec3& nHit)
    {
        // Compute closest approach between infinite ray and segment.
        const glm::vec3 u = rd;         // ray direction (normalized)
        const glm::vec3 v = b - a;      // segment direction
        const glm::vec3 w0 = ro - a;

        float a_ = glm::dot(u,u);       // 1
        float b_ = glm::dot(u,v);
        float c_ = glm::dot(v,v);
        float d_ = glm::dot(u,w0);
        float e_ = glm::dot(v,w0);

        float denom = a_*c_ - b_*b_;
        float s, t; // s along ray, t along segment
        if (denom > 1e-12f) 
        {
            s = (b_*e_ - c_*d_) / denom;
            t = (a_*e_ - b_*d_) / denom;
            t = glm::clamp(t, 0.0f, 1.0f);
            // if clamped, recompute s with t fixed
            s = glm::dot(u, (a + v*t) - ro);
        } 
        else 
        {
            // ray parallel to segment: project a onto ray
            t = 0.0f;
            s = -d_;
        }

        if (s < 0.0f || s > tmax) return false;

        glm::vec3 cptRay    = ro + s * u;
        glm::vec3 cptSeg    = a + t * v;
        glm::vec3 diff      = cptRay - cptSeg;
        float d2            = glm::dot(diff, diff);

        if (d2 > radius*radius) return false;

        float d = std::sqrt(std::max(d2, 1e-12f));
        nHit    = (d > 1e-6f) ? (diff / d) : glm::vec3(0,1,0);
        tHit    = s;
        
        return true;
    }




}