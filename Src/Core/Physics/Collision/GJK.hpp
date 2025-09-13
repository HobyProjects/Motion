#pragma once

#include <array>
#include <vector>
#include <limits>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include "PhyCore.hpp"
#include "SphereShape.hpp"
#include "CapsuleShape.hpp"
#include "BoxShape.hpp"
#include "ConvexHullShape.hpp"
#include "PhyMath.hpp"

namespace Motion
{
    struct SupportFn
    {
        std::function<glm::vec3(const glm::vec3&)> S;
    };

    inline glm::vec3 BoxSupportLocal(const BoxShape& Bx, const glm::vec3& d) 
    {
        return glm::vec3( (d.x>=0? +Bx.HalfExtents.x : -Bx.HalfExtents.x),
                          (d.y>=0? +Bx.HalfExtents.y : -Bx.HalfExtents.y),
                          (d.z>=0? +Bx.HalfExtents.z : -Bx.HalfExtents.z) );
    }


    inline SupportFn MakeSupport(const Collider& A, const glm::mat4& WA, const Collider& B, const glm::mat4& WB)
    {
        glm::mat4 WAi = glm::inverse(WA);
        glm::mat4 WBi = glm::inverse(WB);

        auto SA = [&](const glm::vec3& dWS) -> glm::vec3 
        {
            switch (A.ColliderShape->Type) 
            {
                case ShapeType::Sphere: 
                {
                    const auto& S   = *static_cast<const SphereShape*>(A.ColliderShape);
                    glm::vec3 c     = S.Center;
                    float R         = S.Radius + S.ConvexRadius;

                    glm::vec3 dLS = glm::normalize(TransformVector(WAi, dWS));
                    glm::vec3 pLS = c + dLS * R;

                    return TransformPoint(WA, pLS);
                }
                case ShapeType::Capsule: 
                {
                    const auto& K   = *static_cast<const CapsuleShape*>(A.ColliderShape);
                    glm::vec3 A0    = glm::vec3(0, +K.HalfHeight, 0);
                    glm::vec3 B0    = glm::vec3(0, -K.HalfHeight, 0);
                    float R         = K.Radius + K.ConvexRadius;

                    glm::vec3 dLS   = glm::normalize(TransformVector(WAi, dWS));

                    float sA = glm::dot(A0, dLS);
                    float sB = glm::dot(B0, dLS);
                    glm::vec3 q = (sA > sB) ? A0 : B0;

                    return TransformPoint(WA, q + dLS * R);
                }
                case ShapeType::Box: 
                {
                    const auto& Bx = *static_cast<const BoxShape*>(A.ColliderShape);

                    glm::vec3 dLS = TransformVector(WAi, dWS);
                    glm::vec3 pLS = BoxSupportLocal(Bx, dLS);


                    glm::vec3 sgn = glm::sign(dLS);
                    pLS += sgn * Bx.ConvexRadius;
                    return TransformPoint(WA, pLS);
                }
                case ShapeType::Convex: 
                {
                    const auto& H = *static_cast<const ConvexHullShape*>(A.ColliderShape);
                    glm::vec3 dLS = TransformVector(WAi, dWS);

                    if (glm::length2(dLS) < 1e-24f) dLS = glm::vec3(1,0,0);
                    dLS = glm::normalize(dLS);

                    glm::vec3 pLS = H.SupportPoint(dLS) + dLS * H.ConvexRadius;
                    return TransformPoint(WA, pLS);
                }
            }

            return glm::vec3(0);
        };

        auto SB = [&](const glm::vec3& dWS)->glm::vec3 
        {
            switch (B.ColliderShape->Type) 
            {
                case ShapeType::Sphere: 
                {
                    const auto& S   = *static_cast<const SphereShape*>(B.ColliderShape);
                    glm::vec3 c     = S.Center;
                    float R         = S.Radius + S.ConvexRadius;

                    glm::vec3 dLS = glm::normalize(TransformVector(WBi, dWS));
                    glm::vec3 pLS = c + dLS * R;

                    return TransformPoint(WB, pLS);
                }
                case ShapeType::Capsule: 
                {
                    const auto& K   = *static_cast<const CapsuleShape*>(B.ColliderShape);
                    glm::vec3 A0    = glm::vec3(0, +K.HalfHeight, 0);
                    glm::vec3 B0    = glm::vec3(0, -K.HalfHeight, 0);
                    float R         = K.Radius + K.ConvexRadius;

                    glm::vec3 dLS = glm::normalize(TransformVector(WBi, dWS));

                    float sA = glm::dot(A0, dLS);
                    float sB = glm::dot(B0, dLS);
                    glm::vec3 q = (sA > sB) ? A0 : B0;

                    return TransformPoint(WB, q + dLS * R);
                }
                case ShapeType::Box: 
                {
                    const auto& Bx  = *static_cast<const BoxShape*>(B.ColliderShape);
                    glm::vec3 dLS   = TransformVector(WBi, dWS);
                    glm::vec3 pLS   = BoxSupportLocal(Bx, dLS);
                    glm::vec3 sgn   = glm::sign(dLS);

                    pLS += sgn * Bx.ConvexRadius;
                    return TransformPoint(WB, pLS);
                }
                case ShapeType::Convex: 
                {
                    const auto& H = *static_cast<const ConvexHullShape*>(B.ColliderShape);
                    glm::vec3 dLS = TransformVector(WBi, dWS);
                    if (glm::length2(dLS) < 1e-24f) dLS = glm::vec3(1,0,0);

                    dLS = glm::normalize(dLS);
                    glm::vec3 pLS = H.SupportPoint(dLS) + dLS * H.ConvexRadius;

                    return TransformPoint(WB, pLS);
                }
            }

            return glm::vec3(0);
        };

        SupportFn out{};
        out.S = [SA, SB](const glm::vec3& d) -> glm::vec3 
        {
            return SA(d) - SB(-d);
        };

        return out;
    }




    struct Simplex 
    {
        std::array<glm::vec3,4> P{};
        std::int32_t N{0}; 
    };

    struct GjkResult 
    {
        bool Intersect{false};
        Simplex Splex{};
        glm::vec3 Dir{}; 
    };

    inline GjkResult GJK(const SupportFn& sup, int maxIters = 32) 
    {
        GjkResult res{};
        Simplex s{};
        glm::vec3 d(1,0,0); 

        s.P[0]  = sup.S(d); s.N = 1;
        d       = -s.P[0];
        if (glm::length2(d) < 1e-20f) { res.Intersect = true; res.Splex = s; return res; }

        for (int it = 0; it < maxIters; ++it) 
        {
            glm::vec3 a = sup.S(d);
            if (glm::dot(a, d) < 0.0f) 
            { 
                res.Intersect = false; res.Dir = d; res.Splex = s; return res;
            }

            s.P[s.N++] = a;

            if (s.N == 2) 
            {
                glm::vec3 A     = s.P[1], B = s.P[0];
                glm::vec3 AB    = B - A, AO = -A;

                if (glm::dot(AB, AO) > 0) 
                {
                    d = glm::cross(glm::cross(AB, AO), AB);
                } 
                else 
                {
                    s.P[0] = A; s.N = 1; d = AO;
                }

            } 
            else if (s.N == 3) 
            {
                glm::vec3 A     = s.P[2], B = s.P[1], C = s.P[0];
                glm::vec3 AB    = B - A, AC = C - A, AO = -A;
                glm::vec3 N     = glm::cross(AB, AC);
                glm::vec3 Nac   = glm::cross(N, AC);

                if (glm::dot(Nac, AO) > 0) 
                {
                    s.P[1] = C; s.P[0] = A; s.N = 2;
                    d = glm::cross(glm::cross(AC, AO), AC);
                    continue;
                }

                glm::vec3 Nab = glm::cross(AB, N);
                if (glm::dot(Nab, AO) > 0) 
                {
                    s.P[0] = B; s.P[1] = A; s.N = 2;
                    d = glm::cross(glm::cross(AB, AO), AB);
                    continue;
                }

                if (glm::dot(N, AO) > 0) 
                {
                    d = N;
                } 
                else 
                {
                    std::swap(s.P[0], s.P[1]);
                    d = -N;
                }

            } else 
            {
                glm::vec3 A     = s.P[3], B = s.P[2], C = s.P[1], D = s.P[0];
                glm::vec3 AO    = -A;

                auto faceHasOrigin = [&](const glm::vec3& U, const glm::vec3& V, const glm::vec3& W) -> bool
                {
                    glm::vec3 N = glm::cross(V - U, W - U);
                    return glm::dot(N, -U) > 0.0f;
                };

                bool aoABC = faceHasOrigin(A,B,C);
                bool aoACD = faceHasOrigin(A,C,D);
                bool aoADB = faceHasOrigin(A,D,B);

                if (!aoABC && !aoACD && !aoADB) {
                    res.Intersect = true; res.Splex = s; return res;
                }

                auto reduce = [&](glm::vec3 U, glm::vec3 V, glm::vec3 W)
                {
                    s.P[0]=W; s.P[1]=V; s.P[2]=U; s.N=3;
                    glm::vec3 N = glm::cross(V-U, W-U);
                    if (glm::dot(N, -U) < 0) N = -N;
                    d = N;
                };

                if (aoABC)      reduce(A,B,C);
                else if (aoACD) reduce(A,C,D);
                else            reduce(A,D,B);
            }
        }

        res.Intersect = false; res.Dir = d; res.Splex = s; return res;
    }


    struct EpaResult 
    {
        bool Success{false};
        glm::vec3 Normal{0.0f, 1.0f, 0.0f};
        float Depth{0.0f};
    };

    struct EpaFace 
    { 
        glm::vec3 A{0.0f}, B{0.0f}, C{0.0f}; 
        glm::vec3 N{0.0f}; 
        float D{0.0f}; 
    }; 

    inline EpaResult EPA(const SupportFn& sup, const Simplex& tetra, int maxIters = 48) 
    {
        std::vector<EpaFace> faces{};
        auto mkFace = [&](const glm::vec3& A, const glm::vec3& B, const glm::vec3& C)
        {
            EpaFace f{A,B,C};
            f.N = glm::normalize(glm::cross(B-A, C-A));
            if (glm::dot(f.N, A) < 0) { std::swap(f.B, f.C); f.N = -f.N; }
            f.D = glm::dot(f.N, A);
            return f;
        };

        faces.reserve(64);
        faces.push_back(mkFace(tetra.P[3], tetra.P[2], tetra.P[1]));
        faces.push_back(mkFace(tetra.P[3], tetra.P[1], tetra.P[0]));
        faces.push_back(mkFace(tetra.P[3], tetra.P[0], tetra.P[2]));
        faces.push_back(mkFace(tetra.P[2], tetra.P[0], tetra.P[1]));

        for (int it = 0; it < maxIters; ++it) 
        {
            int best = -1; float minDist = std::numeric_limits<float>::infinity();
            for (int i = 0; i < (int) faces.size(); ++i) 
            {
                float dist = faces[i].D; 
                if (dist < minDist) { minDist = dist; best = i; }
            }

            if (best < 0) break;
            const auto& f = faces[best];

            glm::vec3 p     = sup.S(f.N);
            float pdot      = glm::dot(p, f.N);

            float sep = pdot - f.D;
            if (sep <= 1e-5f) 
            {
                return { true, f.N, f.D };
            }

            struct Edge { glm::vec3 a{0.0f}, b{0.0f}; };
            std::vector<Edge> border{};
            std::vector<EpaFace> keep{};

            keep.reserve(faces.size());
            auto addEdge = [&](const glm::vec3& a, const glm::vec3& b)
            {
                for (auto it = border.begin(); it != border.end(); ++it) 
                {
                    if (glm::all(glm::epsilonEqual(it->a, b, 1e-6f)) &&
                        glm::all(glm::epsilonEqual(it->b, a, 1e-6f))) {
                        border.erase(it); return;
                    }
                }

                border.push_back({a,b});
            };

            for (const auto& F : faces) 
            {
                if (glm::dot(F.N, p - F.A) > 1e-6f) 
                {
                    addEdge(F.A, F.B);
                    addEdge(F.B, F.C);
                    addEdge(F.C, F.A);
                } 
                else 
                {
                    keep.push_back(F);
                }
            }

            faces.swap(keep);
            for (auto& e : border) faces.push_back(mkFace(p, e.a, e.b));
        }
        
        return { false, glm::vec3(0,1,0), 0.0f };
    }



}