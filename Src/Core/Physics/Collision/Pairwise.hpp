#pragma once

#include <memory>

#include "PhyCore.hpp"
#include "Contact.hpp"
#include "SphereShape.hpp"
#include "BoxShape.hpp"
#include "CapsuleShape.hpp"

namespace Motion
{
    struct Segment
    {
        glm::vec3 P{0.0f}, Q{0.0f};
    };

    inline void ClosestPtsSegmentSegment(const Segment& s1, const Segment& s2, float& s, float& t, glm::vec3& c1, glm::vec3& c2)
    {
        const glm::vec3 d1 = s1.Q - s1.P;
        const glm::vec3 d2 = s2.Q - s2.P;
        const glm::vec3 r  = s1.P - s2.P;

        float a = glm::dot(d1,d1);
        float e = glm::dot(d2,d2);
        float f = glm::dot(d2,r);

        if (a <= 1e-12f && e <= 1e-12f) 
        { 
            s   = t = 0; 
            c1  = s1.P; 
            c2  = s2.P; 

            return; 
        }
        
        float c     = glm::dot(d1,r);
        float b     = glm::dot(d1,d2);

        float denom = a*e - b*b;

        if (denom > 1e-12f) s = glm::clamp((b*f - c*e)/denom, 0.0f, 1.0f);
        else                s = 0.0f;

        float tnom = b*s + f;
        if (tnom < 0)      { t = 0; s = glm::clamp(-c/a, 0.0f, 1.0f); }
        else if (tnom > e) { t = 1; s = glm::clamp((b - c)/a, 0.0f, 1.0f); }
        else               { t = tnom / e; }

        c1 = s1.P + d1*s;
        c2 = s2.P + d2*t;
    }

    inline float LengthSq(const glm::vec3& v) 
    { 
        return glm::dot(v,v); 
    }

    inline glm::vec3 ClosestPointOnOBB(const glm::vec3& p, const glm::vec3& c, const glm::vec3 axes[3], const glm::vec3& e) 
    {
        glm::vec3 d = p - c;    
        glm::vec3 q = c;

        for (int i = 0; i < 3; ++i) 
        {
            float dist  = glm::dot(d, axes[i]);
            dist        = glm::clamp(dist, -e[i], e[i]);

            q += dist * axes[i];
        }

        return q;
    }

    inline glm::vec3 BoxPushOutNormal(const glm::vec3& localP, const glm::vec3& e, const glm::vec3 axes[3]) 
    {
        const glm::vec3 slack = e - glm::abs(localP);
        int axis = 0;

        if (slack.y < slack[axis]) axis = 1;
        if (slack.z < slack[axis]) axis = 2;

        float sign = (localP[axis] >= 0.0f) ? 1.0f : -1.0f;
        return axes[axis] * sign;
    }

    inline glm::vec3 ToLocalOBB(const glm::vec3& pWS, const glm::vec3& cB, const glm::vec3 axes[3]) 
    {
        return glm::vec3(
            glm::dot(pWS - cB, axes[0]),
            glm::dot(pWS - cB, axes[1]),
            glm::dot(pWS - cB, axes[2])
        );
    }

    inline glm::vec3 FromLocalOBB(const glm::vec3& pLocal, const glm::vec3& cB, const glm::vec3 axes[3]) 
    {
        return cB + pLocal.x * axes[0] + pLocal.y * axes[1] + pLocal.z * axes[2];
    }

    inline glm::vec3 ClampVec(const glm::vec3& v, const glm::vec3& mn, const glm::vec3& mx) 
    {
        return glm::clamp(v, mn, mx);
    }

    inline float EvalCapsuleOBBAtT(const glm::vec3& p0L, const glm::vec3& p1L, float t, const glm::vec3& eWorld, const glm::vec3& cB, const glm::vec3 axes[3], glm::vec3& outS_ws, glm::vec3& outQ_ws)
    {
        const glm::vec3 sL = glm::mix(p0L, p1L, t);                         // point on segment in box-local coords
        const glm::vec3 qL = ClampVec(sL, -eWorld, eWorld);

        outS_ws = FromLocalOBB(sL, cB, axes);
        outQ_ws = FromLocalOBB(qL, cB, axes);

        return glm::length2(outQ_ws - outS_ws);
    }

    struct Plane 
    {
        glm::vec3 N{0.0f};
        float D{0.0f};
    };

    inline float AbsDot(const glm::vec3& a, const glm::vec3& b) 
    {
        return glm::abs(glm::dot(a, b));
    }

    inline void BoxWorldData(const BoxShape& B, const glm::mat4& WB, glm::vec3& c, glm::vec3 axes[3], glm::vec3& eWorld)
    {
        glm::vec3 scales;
        AxesScalesFromWorld(WB, axes, scales, c);
        eWorld = (B.HalfExtents + glm::vec3(B.ConvexRadius)) * scales; // non-uniform scale handled
    }

    inline void ProjectBoxOnAxis(const glm::vec3& c, const glm::vec3 axes[3], const glm::vec3& e, const glm::vec3& axis, float& outMin, float& outMax)
    {
        const float p = glm::dot(c, axis);
        const float r = e.x * AbsDot(axes[0], axis)
                    + e.y * AbsDot(axes[1], axis)
                    + e.z * AbsDot(axes[2], axis);

        outMin = p - r;
        outMax = p + r;
    }

    inline float AxisOverlapSAT(const glm::vec3& axis, const glm::vec3& cA, const glm::vec3 aAxes[3], const glm::vec3& eA, const glm::vec3& cB, const glm::vec3 bAxes[3], const glm::vec3& eB)
    {
        if (glm::length2(axis) < 1e-12f) return std::numeric_limits<float>::infinity(); 
        const glm::vec3 n = glm::normalize(axis);

        float minA, maxA, minB, maxB;
        ProjectBoxOnAxis(cA, aAxes, eA, n, minA, maxA);
        ProjectBoxOnAxis(cB, bAxes, eB, n, minB, maxB);

        const float d0 = maxB - minA;
        const float d1 = maxA - minB;
        if (d0 <= 0.0f || d1 <= 0.0f) return -1.0f; 

        return glm::min(d0, d1);
    }

    inline void BuildBoxFaceVerts(const glm::vec3& c, const glm::vec3 axes[3], const glm::vec3& e, int faceAxis, float faceSign, glm::vec3 out[4])
    {
        const int i = faceAxis;
        const int j = (i + 1) % 3;
        const int k = (i + 2) % 3;

        const glm::vec3 n  = axes[i] * faceSign;
        const glm::vec3 u  = axes[j];
        const glm::vec3 v  = axes[k];

        const float ei = e[i], ej = e[j], ek = e[k];

        const glm::vec3 fc = c + n * ei; 
        out[0] = fc + u * ej + v * ek;
        out[1] = fc - u * ej + v * ek;
        out[2] = fc - u * ej - v * ek;
        out[3] = fc + u * ej - v * ek;
    }

    inline int ClipPolygonAgainstPlane(const glm::vec3* inPts, int inCount, const Plane& pl, glm::vec3* outPts, int outCapacity = 16)
    {
        int outCount = 0;
        if (inCount <= 0) return 0;

        auto inside = [&](const glm::vec3& p) { return glm::dot(pl.N, p) <= pl.D + 1e-6f; };

        glm::vec3 S = inPts[inCount - 1];
        bool S_in = inside(S);

        for (int i = 0; i < inCount; ++i) {
            const glm::vec3 E = inPts[i];
            const bool E_in = inside(E);

            if (S_in && E_in) 
            {
                if (outCount < outCapacity) outPts[outCount++] = E;
            } 
            else if (S_in && !E_in) 
            {
                const glm::vec3 dir     = E - S;
                const float denom       = glm::dot(pl.N, dir);
                float t                 = (pl.D - glm::dot(pl.N, S)) / denom;
                t                       = glm::clamp(t, 0.0f, 1.0f);

                if (outCount < outCapacity) outPts[outCount++] = S + t * dir;
            } 
            else if (!S_in && E_in) 
            {
                const glm::vec3 dir     = E - S;
                const float denom       = glm::dot(pl.N, dir);
                float t                 = (pl.D - glm::dot(pl.N, S)) / denom;
                t                       = glm::clamp(t, 0.0f, 1.0f);

                if (outCount < outCapacity) outPts[outCount++] = S + t * dir;
                if (outCount < outCapacity) outPts[outCount++] = E;
            }

            S = E; S_in = E_in;
        }

        return outCount;
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

        return false;
    }
}