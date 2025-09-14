#pragma once

#include <memory>

#include "PhyCore.hpp"
#include "Contact.hpp"
#include "GJK.hpp"
#include "ManifoldClip.hpp"

#include "SphereShape.hpp"
#include "BoxShape.hpp"
#include "CapsuleShape.hpp"
#include "ConvexHullShape.hpp"
#include "ConcaveMeshShape.hpp"

namespace Motion
{
    inline void BoxWorldData(const BoxShape& B, const glm::mat4& WB, glm::vec3& c, glm::vec3 axes[3], glm::vec3& eWorld)
    {
        glm::vec3 scales;
        AxesScalesFromWorld(WB, axes, scales, c);
        eWorld = (B.HalfExtents + glm::vec3(B.ConvexRadius)) * scales; 
    }

    inline glm::vec3 FaceNormalLocal(const ConvexHullShape& H, const Tri& f) 
    {
        const glm::vec3& a = H.Vertice[f.I0];
        const glm::vec3& b = H.Vertice[f.I1];
        const glm::vec3& c = H.Vertice[f.I2];
        return glm::normalize(glm::cross(b - a, c - a));
    }

    inline std::uint32_t BestFaceAlong(const ConvexHullShape& H, const glm::mat4& W, const glm::vec3& dirWS) 
    {
        std::uint32_t best = 0; float bestDot = -FLT_MAX;
        for (std::uint32_t i = 0; i < H.Faces.size(); ++i) 
        {
            glm::vec3 nL    = FaceNormalLocal(H, H.Faces[i]);
            glm::vec3 nWS   = glm::normalize(RotateVector(W, nL));
            float d         = glm::dot(nWS, dirWS);

            if (d > bestDot) { bestDot = d; best = i; }
        }

        return best;
    }

    inline void BuildFacePolygonWS(const ConvexHullShape& H, const glm::mat4& W, uint32_t faceIdx, std::vector<glm::vec3>& poly)
    {
        const Tri& f = H.Faces[faceIdx];
        poly.clear();
        poly.push_back( TransformPoint(W, H.Vertice[f.I0]) );
        poly.push_back( TransformPoint(W, H.Vertice[f.I1]) );
        poly.push_back( TransformPoint(W, H.Vertice[f.I2]) );
    }

    struct TempTriHull 
    {
        glm::vec3 v[3];
    };

    inline SupportFn MakeSupportTriVsHull(const TempTriHull& tri, const glm::mat4& Wtri, const Collider& hull, const glm::mat4& Whull)
    {
        auto SA = [&](const glm::vec3& dWS) -> glm::vec3 
        {
            float best = -FLT_MAX; glm::vec3 bestP(0);
            for (int i = 0; i < 3;++i) 
            {
                glm::vec3 p = glm::vec3(Wtri * glm::vec4(tri.v[i],1));
                float s = glm::dot(p, dWS);
                if (s > best) { best = s; bestP = p; }
            }
            return bestP;
        };

        auto SB = [&](const glm::vec3& dWS) -> glm::vec3 
        {
            return MakeSupport(hull, Whull, hull, Whull).S(dWS);
        };

        SupportFn out;
        out.S = [SA, hull, Whull](const glm::vec3& d) -> glm::vec3 
        {
            // Tri ⊖ Hull : SA(d) - SB(-d)
            auto SB_local = MakeSupport(hull, Whull, hull, Whull);
            return SA(d) - SB_local.S(-d);
        };

        return out;
    }

    inline AABB ComputeWorldAABB(const Shape& s, const glm::mat4& M) 
    {
        switch (s.Type) 
        {
            case ShapeType::Sphere: 
            {
                const auto& S = static_cast<const SphereShape&>(s);
                return SphereShape::WorldAABB(S, M);
            }
            case ShapeType::Box: 
            {
                const auto& B = static_cast<const BoxShape&>(s);
                return BoxShape::WorldAABB(M, B.HalfExtents, B.ConvexRadius);
            }
            case ShapeType::Capsule: 
            {
                const auto& K = static_cast<const CapsuleShape&>(s);
                return CapsuleShape::WorldAABB(K, M);
            }
            case ShapeType::Convex:
            {
                const auto& H = static_cast<const ConvexHullShape&>(s);
                return ConvexHullShape::WorldAABB(H, M);
            }
        }

        return { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
    }

    inline bool CollideSphereSphere(const SphereShape& A, const glm::mat4& WA, const SphereShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        const glm::vec3 cA  = SphereShape::WorldCenter(A, WA);
        const glm::vec3 cB  = SphereShape::WorldCenter(B, WB);
        const float rA      = SphereShape::WorldRadius(A, WA);
        const float rB      = SphereShape::WorldRadius(B, WB);

        const glm::vec3 d = cB - cA;
        const float dist2 = glm::dot(d, d);
        const float rSum  = rA + rB + (A.ConvexRadius + B.ConvexRadius);
        const float rSum2 = rSum * rSum;

        if (dist2 > rSum2) return false;

        const float slop = ctx.LinearSlop;
        const float dist = glm::sqrt(glm::max(dist2, slop*slop));

        glm::vec3 n         = (dist > slop) ? (d / dist) : glm::vec3(0, 1, 0); 
        const glm::vec3 p   = cA + n * rA;

        out.Points[0]   = { p, n, rSum - dist };
        out.Count       = 1;

        return true;
    }

    inline bool CollideSphereBox(const SphereShape& A, const glm::mat4& WA, const BoxShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        const glm::vec3 cS = SphereShape::WorldCenter(A, WA);
        const float     rS = SphereShape::WorldRadius(A, WA);

        glm::vec3 axes[3], cB;
        BoxShape::AxesFromWorld(WB, axes, cB);

        const glm::vec3 e = B.HalfExtents + glm::vec3(B.ConvexRadius);
        const glm::vec3 q = ClosestPointOnOBB(cS, cB, axes, e);

        glm::vec3 v         = q - cS;
        const float dist2   = glm::dot(v, v);
        const float rTotal  = rS + A.ConvexRadius;

        const float slop = ctx.LinearSlop;
        if (dist2 <= slop * slop) 
        {
            glm::vec3 localP(
                glm::dot(cS - cB, axes[0]),
                glm::dot(cS - cB, axes[1]),
                glm::dot(cS - cB, axes[2])
            );

            const bool inside =
                (glm::abs(localP.x) <= e.x + slop) &&
                (glm::abs(localP.y) <= e.y + slop) &&
                (glm::abs(localP.z) <= e.z + slop);

            glm::vec3 n = inside ? BoxPushOutNormal(localP, e, axes) : glm::vec3(0, 1, 0);

            out.Points[0] = { cS + n * rS, n, rTotal };
            out.Count = 1;
            return true;
        }

        const float dist = glm::sqrt(dist2);
        if (dist > rTotal) return false;

        const glm::vec3 n       = v / dist;      
        const float penetration = rTotal - dist;
        const glm::vec3 p       = cS + n * rS;

        out.Points[0]   = { p, n, penetration };
        out.Count       = 1;
        return true;
    }

    inline bool CollideSphereCapsule(const SphereShape& S, const glm::mat4& WS, const CapsuleShape& C, const glm::mat4& WC, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        const glm::vec3 cS = SphereShape::WorldCenter(S, WS);
        const float     rS = SphereShape::WorldRadius(S, WS);

        glm::vec3 A, B; float rC;
        CapsuleShape::WorldSegment(C, WC, A, B, rC);

        const glm::vec3 AB  = B - A;
        const float len2    = glm::dot(AB, AB);
        float t = 0.0f;
        if (len2 > 1e-12f) t    = glm::clamp(glm::dot(cS - A, AB) / len2, 0.0f, 1.0f);
        const glm::vec3 P       = A + AB * t;

        glm::vec3 v         = P - cS;
        const float dist2   = glm::dot(v, v);
        const float rTot    = rS + rC + (S.ConvexRadius + C.ConvexRadius);
        
        if (dist2 > rTot * rTot) return false;

        const float slop    = ctx.LinearSlop;
        const float dist    = glm::sqrt(glm::max(dist2, slop * slop));
        const glm::vec3 n   = (dist > slop) ? (v / dist) : glm::normalize(B - A);

        out.Points[0]   = { cS + n * rS, n, rTot - dist };
        out.Count       = 1;
        return true;
    }

    inline bool CollideCapsuleCapsule(const CapsuleShape& A, const glm::mat4& WA, const CapsuleShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        glm::vec3 A0, A1; float rA;
        glm::vec3 B0, B1; float rB;
        CapsuleShape::WorldSegment(A, WA, A0, A1, rA);
        CapsuleShape::WorldSegment(B, WB, B0, B1, rB);

        Segment s1{ A0, A1 }, s2{ B0, B1 };
        float s, t; glm::vec3 c1, c2;
        ClosestPtsSegmentSegment(s1, s2, s, t, c1, c2);

        glm::vec3 v = c2 - c1;           
        const float dist2   = LengthSq(v);
        const float rTot    = rA + rB + (A.ConvexRadius + B.ConvexRadius);
        
        if (dist2 > rTot * rTot) return false;

        const float slop    = ctx.LinearSlop;
        const float dist    = glm::sqrt(glm::max(dist2, slop * slop));
        glm::vec3 n         = (dist > slop) ? (v / dist) : glm::normalize(A1 - A0); 
        const glm::vec3 p = c1 + n * rA;

        out.Points[0] = { p, n, rTot - dist };
        out.Count = 1;
        return true;
    }

    inline bool CollideCapsuleBox(const CapsuleShape& A, const glm::mat4& WA, const BoxShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        glm::vec3 A0{0.0f}, A1{0.0f}; float rA{0.0f};
        CapsuleShape::WorldSegment(A, WA, A0, A1, rA);

        glm::vec3 axes[3], scales, cB;
        AxesScalesFromWorld(WB, axes, scales, cB);
        const glm::vec3 eWorld = (B.HalfExtents + glm::vec3(B.ConvexRadius)) * scales;

        const glm::vec3 p0L = ToLocalOBB(A0, cB, axes);
        const glm::vec3 p1L = ToLocalOBB(A1, cB, axes);
        const glm::vec3 dL  = p1L - p0L;

        float candidates[8]; 
        
        int cn              = 0;
        candidates[cn++]    = 0.0f;
        candidates[cn++]    = 1.0f;

        for (int i = 0; i < 3; ++i) 
        {
            const float di = dL[i];
            if (glm::abs(di) > 1e-12f) 
            {
                const float p0i     = p0L[i];
                float t1            = ( +eWorld[i] - p0i) / di;
                float t2            = ( -eWorld[i] - p0i) / di;

                if (t1 >= 0.0f && t1 <= 1.0f) candidates[cn++] = t1;
                if (t2 >= 0.0f && t2 <= 1.0f) candidates[cn++] = t2;
            }
        }

        float bestD2 = std::numeric_limits<float>::infinity();
        glm::vec3 bestS_ws{0}, bestQ_ws{0};
        for (int i = 0; i < cn; ++i) 
        {
            glm::vec3 S_ws{0.0f}, Q_ws{0.0f};
            float d2 = EvalCapsuleOBBAtT(p0L, p1L, candidates[i], eWorld, cB, axes, S_ws, Q_ws);
            if (d2 < bestD2) { bestD2 = d2; bestS_ws = S_ws; bestQ_ws = Q_ws; }
        }

        const float slop  = ctx.LinearSlop;
        const float dist  = glm::sqrt(glm::max(bestD2, slop*slop));
        const float rTot  = rA + A.ConvexRadius; 
        
        if (dist > rTot) return false;

        glm::vec3 n;
        if (dist > slop) 
        {
            n = ( bestQ_ws - bestS_ws ) / dist;
        }     
        else 
        {
            glm::vec3 sL            = ToLocalOBB(bestS_ws, cB, axes);
            const glm::vec3 slack   = eWorld - glm::abs(sL);

            int axis = 0; 
            if (slack.y < slack[axis]) axis = 1; 
            if (slack.z < slack[axis]) axis = 2;

            float sign  = (sL[axis] >= 0.0f) ? 1.0f : -1.0f;
            n           = axes[axis] * sign;
        }

        const glm::vec3 p = bestS_ws + n * rA;
        out.Points[0] = { p, n, rTot - dist };
        out.Count = 1;
        return true;
    }

    inline bool CollideBoxBox(const BoxShape& A, const glm::mat4& WA, const BoxShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        glm::vec3 cA, aAxes[3], eA;
        glm::vec3 cB, bAxes[3], eB;
        BoxWorldData(A, WA, cA, aAxes, eA);
        BoxWorldData(B, WB, cB, bAxes, eB);

        float bestPen = std::numeric_limits<float>::infinity();
        glm::vec3 bestAxis{0};
        int bestType    = -1;
        int bestIndexA  = -1, bestIndexB = -1;

        auto consider_axis = [&](const glm::vec3& axis, int type, int ia, int ib) -> bool 
        {
            float pen = AxisOverlapSAT(axis, cA, aAxes, eA, cB, bAxes, eB);
            if (pen < 0.f) return false;
            
            if (pen < bestPen) { bestPen = pen; bestAxis = glm::normalize(axis); bestType = type; bestIndexA = ia; bestIndexB = ib; }
            return true;
        };

        for (int i = 0; i < 3; ++i) if (!consider_axis(aAxes[i], 0, i, -1)) return false;
        for (int i = 0; i < 3; ++i) if (!consider_axis(bAxes[i], 1, -1, i)) return false;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) 
            {
                glm::vec3 axis = glm::cross(aAxes[i], bAxes[j]);
                if (glm::length2(axis) > 1e-12f) if (!consider_axis(axis, 2, i, j)) return false;
            }

        glm::vec3 ab = cB - cA;
        if (glm::dot(bestAxis, ab) < 0.0f) bestAxis = -bestAxis;

        const BoxShape* RefBox; const glm::vec3* RefAxes; glm::vec3 RefE; glm::vec3 cRef;
        const BoxShape* IncBox; const glm::vec3* IncAxes; glm::vec3 IncE; glm::vec3 cInc;
        int refFaceAxis; float refFaceSign;

        if (bestType == 0) 
        {
            RefBox = &A; RefAxes = aAxes; RefE = eA; cRef = cA;
            IncBox = &B; IncAxes = bAxes; IncE = eB; cInc = cB;
            refFaceAxis = bestIndexA; refFaceSign = +1.0f;
        } 
        else if (bestType == 1) 
        {
            RefBox = &B; RefAxes = bAxes; RefE = eB; cRef = cB;
            IncBox = &A; IncAxes = aAxes; IncE = eA; cInc = cA;
            refFaceAxis = bestIndexB; refFaceSign = -1.0f; 
        } 
        else 
        {
            float da    = glm::max(AbsDot(aAxes[0], bestAxis), glm::max(AbsDot(aAxes[1], bestAxis), AbsDot(aAxes[2], bestAxis)));
            float db    = glm::max(AbsDot(bAxes[0], bestAxis), glm::max(AbsDot(bAxes[1], bestAxis), AbsDot(bAxes[2], bestAxis)));
            bool useA   = (da >= db);

            if (useA) 
            {
                RefBox = &A; RefAxes = aAxes; RefE = eA; cRef = cA;
                IncBox = &B; IncAxes = bAxes; IncE = eB; cInc = cB;

                int ai = 0; if (AbsDot(aAxes[1], bestAxis) > AbsDot(aAxes[ai], bestAxis)) ai = 1;
                            if (AbsDot(aAxes[2], bestAxis) > AbsDot(aAxes[ai], bestAxis)) ai = 2;
                refFaceAxis = ai; refFaceSign = +1.0f;
            } 
            else 
            {
                RefBox = &B; RefAxes = bAxes; RefE = eB; cRef = cB;
                IncBox = &A; IncAxes = aAxes; IncE = eA; cInc = cA;
                int bi = 0; if (AbsDot(bAxes[1], bestAxis) > AbsDot(bAxes[bi], bestAxis)) bi = 1;
                            if (AbsDot(bAxes[2], bestAxis) > AbsDot(bAxes[bi], bestAxis)) bi = 2;
                refFaceAxis = bi; refFaceSign = -1.0f;
            }
        }

        glm::vec3 nRef = RefAxes[refFaceAxis];
        if (glm::dot(nRef * refFaceSign, bestAxis) < 0.999f) refFaceSign *= -1.0f; // flip if needed
        nRef *= refFaceSign;

        glm::vec3 refVerts[4];
        BuildBoxFaceVerts(cRef, RefAxes, RefE, refFaceAxis, refFaceSign, refVerts);

        int incFaceAxis     = 0;
        float bestDot       = glm::dot(IncAxes[0], -nRef);
        for (int i = 1; i < 3; ++i) 
        {
            float d = glm::dot(IncAxes[i], -nRef);
            if (d > bestDot) { bestDot = d; incFaceAxis = i; }
        }

        float incSign = (glm::dot(IncAxes[incFaceAxis], -nRef) >= 0.0f) ? +1.0f : -1.0f;

        glm::vec3 incVerts[4];
        BuildBoxFaceVerts(cInc, IncAxes, IncE, incFaceAxis, incSign, incVerts);

        const int i = refFaceAxis;
        const int j = (i + 1) % 3;
        const int k = (i + 2) % 3;
        const glm::vec3 u = RefAxes[j];
        const glm::vec3 v = RefAxes[k];
        const float ej = RefE[j], ek = RefE[k];

        const glm::vec3 refFaceCenter = cRef + nRef * RefE[i];

        Plane side[4];
        side[0] = { +u, glm::dot(+u, refFaceCenter + u * ej) };
        side[1] = { -u, glm::dot(-u, refFaceCenter - u * ej) };
        side[2] = { +v, glm::dot(+v, refFaceCenter + v * ek) };
        side[3] = { -v, glm::dot(-v, refFaceCenter - v * ek) };

        glm::vec3 poly1[16]; int c1 = 4;
        for (int idx = 0; idx < 4; ++idx) poly1[idx] = incVerts[idx];

        glm::vec3 poly2[16]; int c2 = 0;
        for (int s = 0; s < 4 && c1 > 0; ++s) 
        {
            c2 = ClipPolygonAgainstPlane(poly1, c1, side[s], poly2, 16);
            if (c2 == 0) break;
            for (int m = 0; m < c2; ++m) poly1[m] = poly2[m];
            c1 = c2;
        }
        if (c1 == 0) return false;

        Plane front = { nRef, glm::dot(nRef, refFaceCenter) };
        glm::vec3 poly3[16];
        int c3 = 0;

        c3 = ClipPolygonAgainstPlane(poly1, c1, front, poly3, 16);
        if (c3 == 0) return false;

        out.Count = 0;
        const float slop = ctx.LinearSlop;
        for (int p = 0; p < c3 && out.Count < 4; ++p) 
        {
            const glm::vec3 q   = poly3[p];
            const float dist    = glm::dot(nRef, q) - front.D; 
            const float pen     = glm::max(-dist, 0.0f);

            if (pen > slop) 
            {
                const glm::vec3 cp = q - nRef * dist; 
                out.Points[out.Count++] = { cp, nRef, pen };
            }
        }

        if (out.Count == 0) 
        {
            out.Points[0]   = { refFaceCenter, nRef, glm::max(bestPen, 0.0f) };
            out.Count       = 1;
        }

        return true;
    }

    inline bool CollideConvexFallback(const Collider& A, const glm::mat4& WA, const Collider& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        SupportFn sup   = MakeSupport(A, WA, B, WB);
        auto gjk        = GJK(sup, 32);
        if (!gjk.Intersect) return false;

        auto epa = EPA(sup, gjk.Splex, 64);
        if (!epa.Success) return false;

        const glm::vec3 n = epa.Normal;
        auto SupportWSOriginal = [&](const Collider& C, const glm::mat4& W, const glm::vec3& d) -> glm::vec3 
        {
            switch (C.ColliderShape->Type) 
            {
                case ShapeType::Sphere: 
                {
                    const auto& S   = *static_cast<const SphereShape*>(C.ColliderShape);
                    glm::vec3 p     = S.Center + glm::normalize(TransformVector(glm::inverse(W), d)) * (S.Radius + S.ConvexRadius);
                    
                    return TransformPoint(W, p);
                }
                case ShapeType::Capsule: 
                {
                    const auto& K = *static_cast<const CapsuleShape*>(C.ColliderShape);
                    glm::vec3 A0(0, +K.HalfHeight, 0), B0(0, -K.HalfHeight, 0);

                    glm::vec3 dLS   = glm::normalize(TransformVector(glm::inverse(W), d));
                    float sA        = glm::dot(A0, dLS), sB = glm::dot(B0, dLS);
                    glm::vec3 q     = (sA > sB) ? A0 : B0;

                    return TransformPoint(W, q + dLS * (K.Radius + K.ConvexRadius));
                }
                case ShapeType::Box: 
                {
                    const auto& Bx  = *static_cast<const BoxShape*>(C.ColliderShape);
                    glm::vec3 dLS   = TransformVector(glm::inverse(W), d);
                    glm::vec3 pLS   = glm::vec3( (dLS.x>=0? +Bx.HalfExtents.x : -Bx.HalfExtents.x),
                                                 (dLS.y>=0? +Bx.HalfExtents.y : -Bx.HalfExtents.y),
                                                 (dLS.z>=0? +Bx.HalfExtents.z : -Bx.HalfExtents.z) );
                    
                    pLS += glm::sign(dLS) * Bx.ConvexRadius;
                    return TransformPoint(W, pLS);
                }
                case ShapeType::Convex: 
                {
                    const auto& H = *static_cast<const ConvexHullShape*>(C.ColliderShape);
                    glm::vec3 dLS = TransformVector(glm::inverse(W), d);

                    if (glm::length2(dLS) < 1e-24f) dLS = glm::vec3(1,0,0);
                    dLS = glm::normalize(dLS);

                    glm::vec3 pLS = H.SupportPoint(dLS) + dLS * H.ConvexRadius;
                    return TransformPoint(W, pLS);
                }
            }

            return glm::vec3(0);
        };

        if (A.ColliderShape->Type & ShapeType::Convex && B.ColliderShape->Type & ShapeType::Convex)
        {
            const auto& HA = *static_cast<const ConvexHullShape*>(A.ColliderShape);
            const auto& HB = *static_cast<const ConvexHullShape*>(B.ColliderShape);

            uint32_t iRef = BestFaceAlong(HA, WA,  n);
            uint32_t iInc = BestFaceAlong(HB, WB, -n);

            std::vector<glm::vec3> refTri, incPoly;
            BuildFacePolygonWS(HA, WA, iRef, refTri);
            BuildFacePolygonWS(HB, WB, iInc, incPoly);

            glm::vec3 rN = glm::normalize(glm::cross(refTri[1]-refTri[0], refTri[2]-refTri[0]));
            if (glm::dot(rN, n) < 0) std::swap(refTri[1], refTri[2]);
            rN = glm::normalize(glm::cross(refTri[1]-refTri[0], refTri[2]-refTri[0]));

            Plane refPlane = PlaneFromPointNormal(refTri[0], rN);
            std::array<Plane,3> sidePlanes;
            ReferenceFaceSidePlanes(refTri, rN, sidePlanes, ctx.LinearSlop);

            std::vector<glm::vec3> clipped;
            ClipIncidentAgainstReference(incPoly, sidePlanes, refPlane, ctx.LinearSlop, clipped);

            out.SharedFriction     = CombineFriction(A.MaterialProp, B.MaterialProp);
            out.SharedRestitution  = CombineRestitution(A.MaterialProp, B.MaterialProp);
            EmitManifoldFromClipped(clipped, n, refPlane, ctx.LinearSlop, out);

            return out.Count > 0;
        }

        {
            glm::vec3 pA = SupportWSOriginal(A, WA, -n);
            glm::vec3 pB = SupportWSOriginal(B, WB,  n);
            glm::vec3 contact = 0.5f * (pA + pB);

            out.Count               = 1;
            out.SharedNormalWS      = n;
            out.SharedFriction      = CombineFriction(A.MaterialProp, B.MaterialProp);
            out.SharedRestitution   = CombineRestitution(A.MaterialProp, B.MaterialProp);

            out.Points[0].PositionWS  = contact;
            out.Points[0].NormalWS    = n;         
            out.Points[0].Penetration = epa.Depth;
            return true;
        }
    }

    inline bool CollideAgainstConcave(const Collider& dynCol, const glm::mat4& Wdyn, const Collider& meshCol, const glm::mat4& Wmesh, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        const auto* M = static_cast<const ConcaveMeshShape*>(meshCol.ColliderShape);
        if (!M || M->Nodes.empty()) return false;

        auto TransformAABB = [](const AABB& b, const glm::mat4& W) -> AABB 
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
        };

        auto OverlapAABB = [](const AABB& a, const AABB& b) -> bool 
        {
            return (a.MIN.x <= b.MAX.x && a.MAX.x >= b.MIN.x) &&
                (a.MIN.y <= b.MAX.y && a.MAX.y >= b.MIN.y) &&
                (a.MIN.z <= b.MAX.z && a.MAX.z >= b.MIN.z);
        };

        auto ClosestPointOnTri = [](const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) -> glm::vec3
        {
            glm::vec3 ab=b-a, ac=c-a, ap=p-a;
            float d1=glm::dot(ab,ap), d2=glm::dot(ac,ap);
            if (d1<=0 && d2<=0) return a;

            glm::vec3 bp=p-b; float d3=glm::dot(ab,bp), d4=glm::dot(ac,bp);
            if (d3>=0 && d4<=d3) return b;

            float vc=d1*d4 - d3*d2;
            if (vc<=0 && d1>=0 && d3<=0) { float v=d1/(d1-d3); return a + v*ab; }

            glm::vec3 cp=p-c; float d5=glm::dot(ab,cp), d6=glm::dot(ac,cp);
            if (d6>=0 && d5<=d6) return c;

            float vb=d5*d2 - d1*d6;
            if (vb<=0 && d2>=0 && d6<=0) { float w=d2/(d2-d6); return a + w*ac; }

            float va=d3*d6 - d5*d4;
            if (va<=0 && (d4-d3)>=0 && (d5-d6)>=0) {
                float w=(d4-d3)/((d4-d3)+(d5-d6)); return b + w*(c-b);
            }

            glm::vec3 n = glm::normalize(glm::cross(ab,ac));
            float dist = glm::dot(p - a, n);
            return p - dist*n;
        };

        auto SupportWSOriginal = [&](const Collider& C, const glm::mat4& W, const glm::vec3& dWS)->glm::vec3
        {
            switch (C.ColliderShape->Type) 
            {
                case ShapeType::Sphere: 
                {
                    const auto& S   = *static_cast<const SphereShape*>(C.ColliderShape);
                    glm::vec3 c     = SphereShape::WorldCenter(S, W);
                    float     R     = SphereShape::WorldRadius(S, W) + S.ConvexRadius;
                    glm::vec3 d     = glm::normalize(dWS);

                    return c + d * R;
                }
                case ShapeType::Capsule: 
                {
                    const auto& K = *static_cast<const CapsuleShape*>(C.ColliderShape);
                    glm::vec3 A, B; float r;
                    CapsuleShape::WorldSegment(K, W, A, B, r);
                    r += K.ConvexRadius;
                    glm::vec3 d = glm::normalize(dWS);
                    const float sA = glm::dot(A, d), sB = glm::dot(B, d);
                    const glm::vec3 q = (sA > sB) ? A : B;
                    return q + d * r;
                }
                case ShapeType::Box: 
                {
                    const auto& Bx = *static_cast<const BoxShape*>(C.ColliderShape);
                    glm::vec3 axes[3], scales, c;
                    AxesScalesFromWorld(W, axes, scales, c);
                    const glm::vec3 e = (Bx.HalfExtents + glm::vec3(Bx.ConvexRadius)) * scales;
                    float s0 = (glm::dot(axes[0], dWS) >= 0.0f) ? +1.0f : -1.0f;
                    float s1 = (glm::dot(axes[1], dWS) >= 0.0f) ? +1.0f : -1.0f;
                    float s2 = (glm::dot(axes[2], dWS) >= 0.0f) ? +1.0f : -1.0f;
                    return c + axes[0]*(s0*e.x) + axes[1]*(s1*e.y) + axes[2]*(s2*e.z);
                }
                case ShapeType::Convex: 
                {
                    const auto& H = *static_cast<const ConvexHullShape*>(C.ColliderShape);
                    glm::mat3 invT = glm::transpose(glm::inverse(glm::mat3(W)));
                    glm::vec3 dLS  = invT * dWS;
                    if (glm::length2(dLS) < 1e-24f) dLS = glm::vec3(1,0,0);
                    dLS = glm::normalize(dLS);
                    glm::vec3 pLS = H.SupportPoint(dLS) + dLS * H.ConvexRadius;
                    return glm::vec3(W * glm::vec4(pLS,1));
                }

                default: break;
            }

            return glm::vec3(0);
        };

        auto MakeSupportTriMinusDyn = [&](const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) -> SupportFn
        {
            SupportFn fn;
            fn.S = [&, a,b,c](const glm::vec3& d) -> glm::vec3 
            {
                float sa = glm::dot(a,d), sb = glm::dot(b,d), sc = glm::dot(c,d);
                glm::vec3 pTri = (sa>sb && sa>sc) ? a : (sb>sc ? b : c);
                glm::vec3 pDyn = SupportWSOriginal(dynCol, Wdyn, -d);
                return pTri - pDyn;
            };

            return fn;
        };

        const AABB dynAABB = ComputeWorldAABB(*dynCol.ColliderShape, Wdyn);

        std::vector<uint32_t> triIdx;
        triIdx.reserve(64);
        std::vector<uint32_t> stack; stack.reserve(64);
        stack.push_back(0u);

        while (!stack.empty()) 
        {
            uint32_t ni = stack.back(); stack.pop_back();
            const auto& N = M->Nodes[ni];
            AABB nW = TransformAABB(N.Box, Wmesh);
            if (!OverlapAABB(dynAABB, nW)) continue;

            if (N.IsLeaf) 
            {
                const uint32_t first = N.Left;
                const uint32_t count = N.Right;
                for (uint32_t i = 0; i < count; ++i) triIdx.push_back(first + i);
            } 
            else 
            {
                stack.push_back(N.Left);
                stack.push_back(N.Right);
            }
        }

        if (triIdx.empty()) return false;
        std::vector<ContactPoint> pool; pool.reserve(16);

        for (uint32_t idx : triIdx) 
        {
            const auto t        = M->Tris[idx];
            const glm::vec3 a   = glm::vec3(Wmesh * glm::vec4(M->Vertice[t.I0],1));
            const glm::vec3 b   = glm::vec3(Wmesh * glm::vec4(M->Vertice[t.I1],1));
            const glm::vec3 c   = glm::vec3(Wmesh * glm::vec4(M->Vertice[t.I2],1));

            switch (dynCol.ColliderShape->Type)
            {
                case ShapeType::Sphere:
                {
                    const auto& S               = *static_cast<const SphereShape*>(dynCol.ColliderShape);
                    const glm::vec3 centerWS    = SphereShape::WorldCenter(S, Wdyn);
                    const float     radiusWS    = SphereShape::WorldRadius(S, Wdyn) + S.ConvexRadius;

                    glm::vec3 q     = ClosestPointOnTri(centerWS, a, b, c);
                    glm::vec3 diff  = centerWS - q;
                    float d2        = glm::dot(diff, diff);

                    if (d2 > 1e-12f) 
                    {
                        float d = std::sqrt(d2);
                        float pen = radiusWS - d;
                        if (pen > ctx.LinearSlop) 
                        {
                            ContactPoint cp;
                            cp.PositionWS  = q;
                            cp.NormalWS    = diff / d;   // tri -> sphere
                            cp.Penetration = pen;
                            pool.push_back(cp);
                        }
                    }
                    break;
                }

                case ShapeType::Capsule:
                case ShapeType::Box:
                case ShapeType::Convex:
                {
                    SupportFn sup = MakeSupportTriMinusDyn(a,b,c);
                    auto gjk = GJK(sup, 32);
                    if (!gjk.Intersect) break;

                    auto epa = EPA(sup, gjk.Splex, 64);
                    if (!epa.Success || epa.Depth <= ctx.LinearSlop) break;

                    const glm::vec3 n   = epa.Normal; 
                    glm::vec3 pTri      = a;
                    float sa            = glm::dot(a,n), sb = glm::dot(b,n), sc = glm::dot(c,n);

                    if (sb > sa && sb > sc) pTri = b; else if (sc > sa && sc > sb) pTri = c;
                    glm::vec3 pDyn = SupportWSOriginal(dynCol, Wdyn,  n);
                    glm::vec3 mid  = 0.5f*(pTri + pDyn);

                    ContactPoint cp;
                    cp.PositionWS  = mid;
                    cp.NormalWS    = n;
                    cp.Penetration = epa.Depth;
                    pool.push_back(cp);
                    break;
                }

                default: break;
            }
        }

        if (pool.empty()) return false;

        std::sort(pool.begin(), pool.end(), [](const ContactPoint& a, const ContactPoint& b){ return a.Penetration > b.Penetration; });

        const int keep  = std::min<int>(4, (int)pool.size());
        out.Count       = keep;

        glm::vec3 nAvg(0);
        for (int i = 0; i < keep; ++i) { out.Points[i] = pool[i]; nAvg += pool[i].NormalWS; }

        if (glm::length2(nAvg) > 1e-12f) 
        {
            nAvg                = glm::normalize(nAvg);
            out.SharedNormalWS  = nAvg;                    
            for (int i = 0; i < keep; ++i) out.Points[i].NormalWS = nAvg; 

        } 
        else 
        {
            out.SharedNormalWS = out.Points[0].NormalWS;
        }

        out.SharedFriction    = CombineFriction(dynCol.MaterialProp, meshCol.MaterialProp);
        out.SharedRestitution = CombineRestitution(dynCol.MaterialProp, meshCol.MaterialProp);
        return true;
    }


    inline bool Collide(const Collider& a, const glm::mat4& worldA, const Collider& b, const glm::mat4& worldB, const NarrowPhaseContext& ctx, ContactManifold& out)
    {
        if(!a.ColliderShape || !b.ColliderShape) return false;

        const ShapeType ta = a.ColliderShape->Type;
        const ShapeType tb = b.ColliderShape->Type;

        if(ta & ShapeType::Sphere && tb & ShapeType::Sphere)
        {
            const auto* SA = static_cast<const SphereShape*>(a.ColliderShape);
            const auto* SB = static_cast<const SphereShape*>(b.ColliderShape);
            return CollideSphereSphere(*SA, worldA * a.LocalPose, *SB, worldB * b.LocalPose, ctx, out);
        }

        if (ta & ShapeType::Sphere && tb & ShapeType::Box) 
        {
            const auto* SA = static_cast<const SphereShape*>(a.ColliderShape);
            const auto* BB = static_cast<const BoxShape*>(b.ColliderShape);
            return CollideSphereBox(*SA, worldA * a.LocalPose, *BB, worldB * b.LocalPose, ctx, out);
        }

        if (ta & ShapeType::Box && tb & ShapeType::Sphere) 
        {
            const auto* BA = static_cast<const BoxShape*>(a.ColliderShape);
            const auto* SB = static_cast<const SphereShape*>(b.ColliderShape);

            ContactManifold tmp{};
            const bool hit = CollideSphereBox(*SB, worldB * b.LocalPose, *BA, worldA * a.LocalPose, ctx, tmp);
            if (!hit) return false;

            tmp.Points[0].NormalWS = -tmp.Points[0].NormalWS;
            out = tmp;
            return true;
        }




        if (ta & ShapeType::Capsule && tb & ShapeType::Capsule) 
        {
            const auto* CA = static_cast<const CapsuleShape*>(a.ColliderShape);
            const auto* CB = static_cast<const CapsuleShape*>(b.ColliderShape);
            return CollideCapsuleCapsule(*CA, worldA * a.LocalPose, *CB, worldB * b.LocalPose, ctx, out);
        }

        if (ta & ShapeType::Sphere && tb & ShapeType::Capsule) 
        {
            const auto* SA = static_cast<const SphereShape*>(a.ColliderShape);
            const auto* CB = static_cast<const CapsuleShape*>(b.ColliderShape);
            return CollideSphereCapsule(*SA, worldA * a.LocalPose, *CB, worldB * b.LocalPose, ctx, out);
        }

        if (ta & ShapeType::Capsule && tb & ShapeType::Sphere) 
        {
            const auto* CA = static_cast<const CapsuleShape*>(a.ColliderShape);
            const auto* SB = static_cast<const SphereShape*>(b.ColliderShape);

            ContactManifold tmp{};
            const bool hit = CollideSphereCapsule(*SB, worldB * b.LocalPose, *CA, worldA * a.LocalPose, ctx, tmp);
            if (!hit) return false;

            tmp.Points[0].NormalWS = -tmp.Points[0].NormalWS;
            out = tmp;
            return true;
        }
        
        if (ta & ShapeType::Capsule && tb & ShapeType::Box) 
        {
            const auto* CA = static_cast<const CapsuleShape*>(a.ColliderShape);
            const auto* BB = static_cast<const BoxShape*>(b.ColliderShape);
            return CollideCapsuleBox(*CA, worldA * a.LocalPose, *BB, worldB * b.LocalPose, ctx, out);
        }

        if (ta & ShapeType::Box && tb & ShapeType::Capsule) 
        {
            const auto* BA = static_cast<const BoxShape*>(a.ColliderShape);
            const auto* CB = static_cast<const CapsuleShape*>(b.ColliderShape);

            ContactManifold tmp{};
            const bool hit = CollideCapsuleBox(*CB, worldB * b.LocalPose, *BA, worldA * a.LocalPose, ctx, tmp);
            if (!hit) return false;

            tmp.Points[0].NormalWS = -tmp.Points[0].NormalWS;
            out = tmp;
            return true;
        }

        if (ta & ShapeType::Box && tb & ShapeType::Box) 
        {
            const auto* BA = static_cast<const BoxShape*>(a.ColliderShape);
            const auto* BB = static_cast<const BoxShape*>(b.ColliderShape);
            return CollideBoxBox(*BA, worldA * a.LocalPose, *BB, worldB * b.LocalPose, ctx, out);
        }

        if(ta & ShapeType::Convex && tb & ShapeType::Convex)
        {
            return CollideConvexFallback(a, worldA, b, worldB, ctx, out);
        }

        if (ta & ShapeType::Concave && tb & ShapeType::Concave) 
        {
            // not supported: two dynamic concaves — skip or treat both as static (usually disallowed)
            return false;
        }

        if (ta & ShapeType::Concave) 
        {
            return CollideAgainstConcave(a, worldB, b, worldA, ctx, out);
        }
        
        if (tb & ShapeType::Concave) 
        {
            return CollideAgainstConcave(a, worldA, b, worldB, ctx, out);
        }

        return false;
    }
}