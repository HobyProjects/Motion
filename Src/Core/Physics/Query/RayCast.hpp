#pragma once

#include <vector>
#include <limits>

#include "PhyCore.hpp"
#include "AABB.hpp"
#include "BoxShape.hpp"
#include "SphereShape.hpp"
#include "CapsuleShape.hpp"
#include "ConvexHullShape.hpp"
#include "ConcaveMeshShape.hpp"

namespace Motion
{
    struct Ray
    {
        glm::vec3 Origin{0.0f};
        glm::vec3 Dir{0.0f, 0.0f, 1.0f};
        float TMin{0.0f};
        float TMax{1e6f};
    };

    struct RayHit
    {
        float T{std::numeric_limits<float>::infinity()};
        glm::vec3 Position{0.0f};
        glm::vec3 Normal{0.0f};
        const Collider* HitCollder{nullptr};
    };

    inline bool RayAABB(const Ray& r, const AABB& b, float& tEnter, float& tExit) 
    {
        glm::vec3 invD = 1.0f / r.Dir;
        glm::vec3 t0 = (b.MIN - r.Origin) * invD;
        glm::vec3 t1 = (b.MAX - r.Origin) * invD;
        glm::vec3 tmin = glm::min(t0, t1);
        glm::vec3 tmax = glm::max(t0, t1);
        tEnter = glm::max(glm::max(tmin.x, tmin.y), glm::max(tmin.z, r.TMin));
        tExit  = glm::min(glm::min(tmax.x, tmax.y), glm::min(tmax.z, r.TMax));
        return tExit >= tEnter && tExit >= 0.0f;
    }

    inline bool RaySphere(const Ray& r, const glm::vec3& c, float R, float& t, glm::vec3& n) 
    {
        glm::vec3 oc    = r.Origin - c;
        float b         = glm::dot(oc, r.Dir);            
        float cterm     = glm::dot(oc, oc) - R*R;
        float disc      = b * b - cterm;

        if (disc < 0.0f) return false;

        float s     = glm::sqrt(disc);
        float t0    = -b - s;
        float t1    = -b + s;

        t = (t0 >= 0.0f) ? t0 : ((t1 >= 0.0f) ? t1 : std::numeric_limits<float>::infinity());
        if (!std::isfinite(t)) return false;

        // normal at hit
        glm::vec3 p = r.Origin + t * r.Dir;
        n           = glm::normalize(p - c);
        return true;
    }

    inline bool RayCapsule(const Ray& r, const glm::vec3& A, const glm::vec3& B, float R, float& t, glm::vec3& n) 
    {
        glm::vec3 d     = B - A;           
        glm::vec3 m     = r.Origin - A;
        glm::vec3 nray  = r.Dir;
        float dd = glm::dot(d, d);
        glm::vec3 d_hat = (dd > 0.0f) ? (d / glm::sqrt(dd)) : glm::vec3(0,1,0);

        glm::vec3 tmp = (std::abs(d_hat.x) < 0.577f) ? glm::vec3(1,0,0) : glm::vec3(0,1,0);
        glm::vec3 u = glm::normalize(glm::cross(d_hat, tmp));
        glm::vec3 v = glm::cross(d_hat, u);

        auto proj = [&](const glm::vec3& w){ return glm::vec2(glm::dot(w,u), glm::dot(w,v)); };

        glm::vec2 m2 = proj(m);
        glm::vec2 d2 = proj(d);
        glm::vec2 n2 = proj(nray);

        float Aqq = glm::dot(n2, n2);
        float Bqq = 2.0f * glm::dot(m2, n2);
        float Cqq = glm::dot(m2, m2) - R*R;

        float tCyl = std::numeric_limits<float>::infinity();
        if (Aqq > 1e-8f) 
        {
            float disc = Bqq*Bqq - 4*Aqq*Cqq;
            if (disc >= 0.0f) 
            {
                float sdisc = glm::sqrt(disc);
                float t0 = (-Bqq - sdisc) / (2*Aqq);
                float t1 = (-Bqq + sdisc) / (2*Aqq);

                if (t0 >= 0.0f) tCyl = t0; else if (t1 >= 0.0f) tCyl = t1;
            }
        }

        auto on_axis_t = [&](float thit) -> bool
        {
            if (!std::isfinite(thit)) return false;
            glm::vec3 p = r.Origin + thit * nray;
            float s = glm::dot(p - A, d_hat);
            return (s >= 0.0f && s <= glm::length(d));
        };

        float tHit = std::numeric_limits<float>::infinity();
        glm::vec3 nHit{0};

        if (on_axis_t(tCyl)) 
        {
            tHit = tCyl;
            glm::vec3 p = r.Origin + tHit * nray;
            glm::vec3 aClosest = A + d_hat * glm::dot(p - A, d_hat);
            nHit = glm::normalize(p - aClosest);
        }

        float tA, tB; glm::vec3 nA, nB;
        bool hitA = RaySphere(r, A, R, tA, nA);
        bool hitB = RaySphere(r, B, R, tB, nB);
        if (hitA && tA < tHit) { tHit = tA; nHit = nA; }
        if (hitB && tB < tHit) { tHit = tB; nHit = nB; }

        if (!std::isfinite(tHit)) return false;
        t = tHit; n = nHit;
        return true;
    }

    inline bool RayOBB(const Ray& r, const glm::vec3& c, const glm::vec3 axes[3], const glm::vec3& e, float& t, glm::vec3& n) 
    {
        glm::vec3 roL = { glm::dot(r.Origin - c, axes[0]),
                          glm::dot(r.Origin - c, axes[1]),
                          glm::dot(r.Origin - c, axes[2]) };

        glm::vec3 rdL = { glm::dot(r.Dir, axes[0]),
                          glm::dot(r.Dir, axes[1]),
                          glm::dot(r.Dir, axes[2]) };

        float tmin  = r.TMin, tmax = r.TMax;
        int hitAxis = -1; float hitSign = 0.0f;

        auto slab = [&](int i, float minv, float maxv) 
        {
            if (std::abs(rdL[i]) < 1e-8f) 
            {
                if (roL[i] < minv || roL[i] > maxv) { tmin = 1; tmax = 0; return; } 
                return;
            }

            float invD  = 1.0f / rdL[i];
            float t0    = (minv - roL[i]) * invD;
            float t1    = (maxv - roL[i]) * invD;
            float enter = glm::min(t0, t1);
            float exit  = glm::max(t0, t1);

            int axisEnter = (t0 > t1) ? -i-1 : i+1; 

            if (enter > tmin) { tmin = enter; hitAxis = std::abs(axisEnter)-1; hitSign = (axisEnter>0)? +1.0f : -1.0f; }
            if (exit  < tmax) { tmax = exit; }
        };

        slab(0, -e.x, +e.x);
        slab(1, -e.y, +e.y);
        slab(2, -e.z, +e.z);
        if (tmax < tmin || tmax < 0.0f) return false;

        t = (tmin >= 0.0f) ? tmin : tmax; 
        n = axes[hitAxis] * hitSign;
        return true;
    }

    inline AABB TransformAABB(const AABB& b, const glm::mat4& W)
    {
        const glm::vec3 mn = b.MIN, mx = b.MAX;
        const glm::vec3 corners[8] = 
        {
            {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},
            {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z}
        };

        glm::vec3 wmn(+FLT_MAX), wmx(-FLT_MAX);
        for (auto& c : corners) 
        {
            glm::vec3 p = glm::vec3(W * glm::vec4(c,1));
            wmn = glm::min(wmn, p); wmx = glm::max(wmx, p);
        }

        return { wmn, wmx };
    }

    inline bool RayTriangle(const Ray& r, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& t, glm::vec3& n)
    {
        const float EPS     = 1e-8f;
        glm::vec3 e1        = b - a;
        glm::vec3 e2        = c - a;
        glm::vec3 p         = glm::cross(r.Dir, e2);
        float det           = glm::dot(e1, p);

        if (glm::abs(det) < EPS) return false; 

        float invDet    = 1.0f / det;
        glm::vec3 s     = r.Origin - a;
        float u         = glm::dot(s, p) * invDet;

        if (u < 0.0f || u > 1.0f) return false;

        glm::vec3 q = glm::cross(s, e1);
        float v     = glm::dot(r.Dir, q) * invDet;

        if (v < 0.0f || u + v > 1.0f) return false;

        float tt = glm::dot(e2, q) * invDet;
        if (tt < r.TMin || tt > r.TMax) return false;

        t = tt;
        n = glm::normalize(glm::cross(e1, e2)); 
        if (glm::dot(n, r.Dir) > 0.0f) n = -n;  
        return true;
    }

    inline bool RaycastConvex(const Ray& r, const ConvexHullShape& H, const glm::mat4& W, const Collider* hullCollider,  RayHit& out)
    {
        if (H.Vertice.empty() || H.Faces.empty()) return false;

        glm::vec3 centroid(0.0f);
        for (const auto& v : H.Vertice) centroid += glm::vec3(W * glm::vec4(v,1));
        centroid *= (1.0f / float(H.Vertice.size()));

        float tmin = r.TMin;
        float tmax = glm::min(r.TMax, out.T);
        glm::vec3 enterNormal(0.0f);
        bool hadEnter = false;

        const float cr = H.ConvexRadius;
        for (const auto& f : H.Faces)
        {
            const glm::vec3 a = glm::vec3(W * glm::vec4(H.Vertice[f.I0],1));
            const glm::vec3 b = glm::vec3(W * glm::vec4(H.Vertice[f.I1],1));
            const glm::vec3 c = glm::vec3(W * glm::vec4(H.Vertice[f.I2],1));

            glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));  
            glm::vec3 faceCenter = (a + b + c) * (1.0f/3.0f);
            if (glm::dot(n, faceCenter - centroid) < 0.0f) n = -n;

            const float d     = glm::dot(n, a) + cr;                  
            const float numer = d - glm::dot(n, r.Origin);
            const float denom = glm::dot(n, r.Dir);

            if (glm::abs(denom) < 1e-8f)
            {
                if (numer < 0.0f) return false; 
                continue;
            }

            const float t = numer / denom;

            if (denom < 0.0f)
            {
                if (t > tmin)
                {
                    tmin = t;
                    enterNormal = n;
                    hadEnter = true;
                }
            }
            else
            {
                if (t < tmax) tmax = t;
            }

            if (tmin > tmax) return false; 
        }

        const float tHit = glm::clamp(tmin, r.TMin, tmax);
        if (tHit >= out.T) return false; 

        out.T          = tHit;
        out.Position   = r.Origin + tHit * r.Dir;
        out.Normal     = hadEnter ? enterNormal : glm::vec3(0,1,0); 
        out.HitCollder = hullCollider;
        return true;
    }

    inline bool RaycastConcave(const Ray& r, const ConcaveMeshShape& M, const glm::mat4& Wmesh, const Collider* meshCollider, RayHit& out)
    {
        if (M.Nodes.empty()) return false;

        bool hitAny = false;
        float tEnter, tExit;

        const AABB rootWB = TransformAABB(M.Nodes[0].Box, Wmesh);
        if (!RayAABB(r, rootWB, tEnter, tExit)) return false;

        std::vector<std::uint32_t> stack;
        stack.reserve(64);
        stack.push_back(0u);

        while (!stack.empty()) 
        {
            std::uint32_t ni = stack.back(); stack.pop_back();
            const auto& N = M.Nodes[ni];

            AABB nodeWB = TransformAABB(N.Box, Wmesh);
            if (!RayAABB(r, nodeWB, tEnter, tExit)) continue;

            if (N.IsLeaf) 
            {
                const std::uint32_t first = N.Left;
                const std::uint32_t count = N.Right;

                for (std::uint32_t i=0; i<count; ++i) 
                {
                    const auto tri      = M.Tris[first + i];
                    const glm::vec3 a   = glm::vec3(Wmesh * glm::vec4(M.Vertice[tri.I0],1));
                    const glm::vec3 b   = glm::vec3(Wmesh * glm::vec4(M.Vertice[tri.I1],1));
                    const glm::vec3 c   = glm::vec3(Wmesh * glm::vec4(M.Vertice[tri.I2],1));

                    float t; glm::vec3 n;
                    if (RayTriangle(r, a, b, c, t, n) && t < out.T) 
                    {
                        out.T           = t;
                        out.Position    = r.Origin + t * r.Dir;
                        out.Normal      = n;
                        out.HitCollder  = meshCollider;
                        hitAny = true;
                    }
                }
            } 
            else 
            {
                stack.push_back(N.Left);
                stack.push_back(N.Right);
            }
        }
        
        return hitAny;
    }



    inline bool RaycastCollider(const Ray& r, const Collider& col, const glm::mat4& world, RayHit& out) 
    {
        switch (col.ColliderShape->Type) 
        {
            case ShapeType::Sphere: 
            {
                const auto& S   = *static_cast<const SphereShape*>(col.ColliderShape);
                glm::vec3 c     = SphereShape::WorldCenter(S, world);
                float R         = SphereShape::WorldRadius(S, world) + S.ConvexRadius;
                
                float t; glm::vec3 n;
                if (RaySphere(r, c, R, t, n) && t >= r.TMin && t <= r.TMax && t < out.T) 
                {
                    out.T = t; out.Position = r.Origin + t*r.Dir; out.Normal = n; return true;
                }
                return false;
            }
            case ShapeType::Capsule: 
            {
                const auto& K = *static_cast<const CapsuleShape*>(col.ColliderShape);
                glm::vec3 A, B; float R; CapsuleShape::WorldSegment(K, world, A, B, R);
                R += K.ConvexRadius;

                float t; glm::vec3 n;
                if (RayCapsule(r, A, B, R, t, n) && t >= r.TMin && t <= r.TMax && t < out.T) 
                {
                    out.T = t; out.Position = r.Origin + t*r.Dir; out.Normal = n; return true;
                }

                return false;
            }
            case ShapeType::Box: 
            {
                const auto& Bx = *static_cast<const BoxShape*>(col.ColliderShape);
                glm::vec3 axes[3], scales, cB;
                AxesScalesFromWorld(world, axes, scales, cB);

                glm::vec3 e = (Bx.HalfExtents + glm::vec3(Bx.ConvexRadius)) * scales;
                float t; glm::vec3 n;
                if (RayOBB(r, cB, axes, e, t, n) && t >= r.TMin && t <= r.TMax && t < out.T) 
                {
                    out.T = t; out.Position = r.Origin + t * r.Dir; out.Normal = n; return true;
                }

                return false;
            }
            case ShapeType::Convex:
            {
                const auto& H = *static_cast<const ConvexHullShape*>(col.ColliderShape);
                return RaycastConvex(r, H, world, &col, out);
            }
            case ShapeType::Concave:
            {
                const auto& M = *static_cast<const ConcaveMeshShape*>(col.ColliderShape);
                return RaycastConcave(r, M, world, &col, out);
            }
        }
        
        return false;
    }














}