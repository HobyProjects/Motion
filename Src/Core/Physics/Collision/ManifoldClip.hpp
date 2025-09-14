#pragma once

#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtx/compatibility.hpp> 
#include <glm/gtx/norm.hpp>

#include "PhyCore.hpp"
#include "PhyMath.hpp"
#include "Contact.hpp"

namespace Motion
{
    struct Segment
    {
        glm::vec3 P{0.0f}, Q{0.0f};
    };

    struct Plane 
    {
        glm::vec3 N{0.0f};
        float D{0.0f};
    };


    inline void AxesScalesFromWorld(const glm::mat4& world, glm::vec3 axes[3], glm::vec3& scales, glm::vec3& center)
    {
        const glm::vec3 cx = glm::vec3(world[0]);
        const glm::vec3 cy = glm::vec3(world[1]);
        const glm::vec3 cz = glm::vec3(world[2]);

        const float lx = glm::length(cx);
        const float ly = glm::length(cy);
        const float lz = glm::length(cz);

        axes[0] = (lx > 0) ? (cx / lx) : glm::vec3(1,0,0);
        axes[1] = (ly > 0) ? (cy / ly) : glm::vec3(0,1,0);
        axes[2] = (lz > 0) ? (cz / lz) : glm::vec3(0,0,1);

        scales = glm::vec3(lx, ly, lz);              
        center = TransformPoint(world, glm::vec3(0)); 
    }

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

    inline float EvalCapsuleOBBAtT(const glm::vec3& p0L, const glm::vec3& p1L, float t, const glm::vec3& eWorld, const glm::vec3& cB, const glm::vec3 axes[3], glm::vec3& outS_ws, glm::vec3& outQ_ws)
    {
        const glm::vec3 sL = glm::mix(p0L, p1L, t);                       
        const glm::vec3 qL = ClampVec(sL, -eWorld, eWorld);

        outS_ws = FromLocalOBB(sL, cB, axes);
        outQ_ws = FromLocalOBB(qL, cB, axes);

        return glm::length2(outQ_ws - outS_ws);
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

    inline Plane PlaneFromPointNormal(const glm::vec3& p, const glm::vec3& n) 
    {
        Plane pl; 
        pl.N = glm::normalize(n); 
        pl.D = glm::dot(pl.N, p); 
        return pl;
    }

    inline void ClipPolygonAgainstPlane(const std::vector<glm::vec3>& inPoly, const Plane& pl, std::vector<glm::vec3>& outPoly, float slop)
    {
        outPoly.clear();
        if (inPoly.empty()) return;
        auto side = [&](const glm::vec3& P) -> float { return glm::dot(pl.N, P) - pl.D; };

        for (size_t i=0; i<inPoly.size(); ++i) 
        {
            const glm::vec3& A = inPoly[i];
            const glm::vec3& B = inPoly[(i+1) % inPoly.size()];
            float da = side(A), db = side(B);
            bool ina = (da <= slop), inb = (db <= slop);

            if (ina && inb) 
            {
                outPoly.push_back(B);
            } 
            else if (ina && !inb) 
            {
                float t = da / (da - db);
                outPoly.push_back( glm::mix(A, B, t) );
            } 
            else if (!ina && inb) 
            {
                float t = da / (da - db);
                outPoly.push_back( glm::mix(A, B, t) );
                outPoly.push_back( B );
            } 
            else 
            {
            }
        }
    }

    inline void ReferenceFaceSidePlanes(const std::vector<glm::vec3>& refTriWS, const glm::vec3& refN, std::array<Plane,3>& out, float slop)
    {
        for (int i = 0; i < 3; ++i) 
        {
            const glm::vec3& a  = refTriWS[i];
            const glm::vec3& b  = refTriWS[(i + 1) % 3];

            glm::vec3 edge      = b - a;
            glm::vec3 outward   = glm::normalize(glm::cross(refN, edge));
            Plane pl            = PlaneFromPointNormal(a, outward);


            pl.D -= slop;
            out[i] = pl;
        }
    }

    inline void ClipIncidentAgainstReference(const std::vector<glm::vec3>& incidentPoly, const std::array<Plane,3>& sidePlanes, const Plane& refPlane, float slop, std::vector<glm::vec3>& outClipped)
    {
        std::vector<glm::vec3> tmp0, tmp1;
        tmp0 = incidentPoly;
        for (int i = 0; i < 3; ++i) 
        {
            ClipPolygonAgainstPlane(tmp0, sidePlanes[i], tmp1, slop);
            tmp0.swap(tmp1);
            tmp1.clear();
            if (tmp0.empty()) break;
        }

        outClipped.clear();
        for (const auto& P : tmp0) 
        {
            float signedDist = glm::dot(refPlane.N, P) - refPlane.D; 
            if (signedDist <= slop) outClipped.push_back(P);
        }
    }

    inline void EmitManifoldFromClipped(const std::vector<glm::vec3>& pts, const glm::vec3& n, const Plane& refPlane, float slop, ContactManifold& out)
    {
        struct CP { glm::vec3 p; float pen; };
        std::vector<CP> cps; cps.reserve(pts.size());

        for (auto& P : pts) 
        {
            float pen = (refPlane.D - glm::dot(refPlane.N, P));
            if (pen > slop) cps.push_back({P, pen});
        }

        if (cps.empty()) { out.Count = 0; return; }

        std::sort(cps.begin(), cps.end(), [](const CP& a, const CP& b){ return a.pen > b.pen; });
        int keep = std::min<int>(4, (int)cps.size());

        out.Count = keep;
        for (int i = 0; i < keep; ++i) 
        {
            out.Points[i].PositionWS  = cps[i].p;
            out.Points[i].NormalWS    = n;
            out.Points[i].Penetration = cps[i].pen;
        }
    }




    


}