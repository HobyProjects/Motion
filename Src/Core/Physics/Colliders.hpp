#pragma once

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <numeric>
#include <array>
#include <cstdint>
#include <cmath>
#include <limits>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp> 

#include "Base.hpp"
#include "Components.hpp"
#include "Mesh.hpp"

namespace Motion
{
    inline constexpr float EPS_DIST2        = 1e-8f;  
    inline constexpr float EPS_DIST         = 1e-4f;
    inline constexpr float EPS_PARALLEL     = 1e-8f;  
    inline constexpr float EPS_GJK_DOT      = 1e-6f;   
    inline constexpr int   MAX_GJK_ITERS    = 64;
    inline constexpr int   MAX_EPA_ITERS    = 96;

    inline glm::vec3 SafeNormalize(const glm::vec3& v, float eps2 = EPS_DIST2)
    {
        float l2 = glm::length2(v);
        if (l2 <= eps2) return glm::vec3(1,0,0);
        return v / std::sqrt(l2);
    }

    enum class ColliderType { AABB, OBB, Sphere, Capsule, ConvexHull, ConcaveMesh };

    template<typename T>
    concept ColliderTypeOf = requires { { T::Type } -> std::convertible_to<ColliderType>; };

    template<typename T>
    concept ColliderExpected = ColliderTypeOf<T> &&
    ( 
        T::Type == ColliderType::AABB           || 
        T::Type == ColliderType::OBB            || 
        T::Type == ColliderType::Sphere         || 
        T::Type == ColliderType::Capsule        || 
        T::Type == ColliderType::ConvexHull     || 
        T::Type == ColliderType::ConcaveMesh 
    );

    struct CollisionResult
    {
        bool HIT{false};
        glm::vec3 Normal{0.0f};
        float Depth{0.0f};
    };

    template<ColliderExpected ShapeA, ColliderExpected ShapeB>
    struct CollisionDetector;

    struct AABB
    {
        static constexpr ColliderType Type = ColliderType::AABB;

        glm::vec3 MIN{0.0f};
        glm::vec3 MAX{0.0f};

        AABB() = default;
        AABB(const glm::vec3& min, const glm::vec3& max):
            MIN(min), MAX(max) {};
        ~AABB() = default;

        [[nodiscard]] static AABB Empty() 
        {
            return 
            {
                {
                    +std::numeric_limits<float>::infinity(), 
                    +std::numeric_limits<float>::infinity(), 
                    +std::numeric_limits<float>::infinity()
                }, 
                {
                    -std::numeric_limits<float>::infinity(), 
                    -std::numeric_limits<float>::infinity(), 
                    -std::numeric_limits<float>::infinity()
                }
            };
        }

        void Expand(const glm::vec3& p)
        {
            MIN.x = std::min(MIN.x, p.x);
            MIN.y = std::min(MIN.y, p.y);
            MIN.z = std::min(MIN.z, p.z);

            MAX.x = std::max(MAX.x, p.x);
            MAX.y = std::max(MAX.y, p.y);
            MAX.z = std::max(MAX.z, p.z);
        }

        void Expand(const AABB& b)
        {
            Expand(b.MIN);
            Expand(b.MAX);
        }

        [[nodiscard]] bool Overlaps(const AABB& b) const
        {
            if (MAX.x < b.MIN.x || MIN.x > b.MAX.x) return false;
            if (MAX.y < b.MIN.y || MIN.y > b.MAX.y) return false;
            if (MAX.z < b.MIN.z || MIN.z > b.MAX.z) return false;
            return true;
        }

        [[nodiscard]] glm::vec3 Center() const { return (MIN + MAX) * 0.5f; }
        [[nodiscard]] glm::vec3 HalfExtents() const { return (MAX - MIN) * 0.5f; }
        [[nodiscard]] glm::vec3 Size() const { return MAX - MIN; }
    };

    struct OBB
    {
        static constexpr ColliderType Type = ColliderType::OBB;

        glm::vec3 Center{0.0f};
        glm::vec3 HalfExtents{0.5f};
        glm::mat3 Axes{1.0f};

        OBB() = default;
        OBB(const glm::vec3 center, glm::vec3 halfExtents, glm::mat3 axes) : 
            Center(center), HalfExtents(halfExtents), Axes(axes){};
        ~OBB() = default;
    };

    struct Box
    {
        [[nodiscard]] static float Absf(float v) { return std::fabs(v); }

        [[nodiscard]] static float ProjectRadiusOBB(const OBB& b, const glm::vec3& axisN)
        {
            return  b.HalfExtents.x * Absf(glm::dot(axisN, b.Axes[0])) +
                    b.HalfExtents.y * Absf(glm::dot(axisN, b.Axes[1])) +
                    b.HalfExtents.z * Absf(glm::dot(axisN, b.Axes[2]));
        }

        [[nodiscard]] static float ProjectRadiusAABB(const AABB& a, const glm::vec3& axisN)
        {
            const glm::vec3 e = a.HalfExtents();
            return e.x * Absf(axisN.x) + e.y * Absf(axisN.y) + e.z * Absf(axisN.z);
        }

        [[nodiscard]] static float SATOverlapOnAxis(float tAB, float rA, float rB)
        {
            const float dist    = Absf(tAB);
            const float overlap = (rA + rB) - dist;
            return overlap;
        }

        [[nodiscard]] static bool ConsiderAxisNormalized(glm::vec3 axisN, float tAB, float rA, float rB, float& minOverlap, glm::vec3& bestNormal)
        {
            axisN = SafeNormalize(axisN);
            const float overlap = SATOverlapOnAxis(tAB, rA, rB);
            if (overlap < 0.0f) { minOverlap = -1.0f; return false; }

            glm::vec3 n = (tAB < 0.0f) ? -axisN : axisN;

            if (overlap + EPS_DIST < minOverlap)
            {
                minOverlap = overlap;
                bestNormal = n;
            }
            return true;
        }
    };


    struct Sphere
    {
        static constexpr ColliderType Type = ColliderType::Sphere;

        glm::vec3   Center{0.0f};
        float       Radius{0.5f};

        [[nodiscard]] static glm::vec3 ClosestPointOnAABB(const glm::vec3& p, const AABB& b)
        {
            return glm::clamp(p, b.MIN, b.MAX);
        }

        [[nodiscard]] static glm::vec3 ClosestPointOnOBB(const glm::vec3& p, const OBB& b)
        {
            glm::vec3 d = p - b.Center;
            glm::vec3 pLocal = 
            {
                glm::dot(d, b.Axes[0]),
                glm::dot(d, b.Axes[1]),
                glm::dot(d, b.Axes[2])
            };

            glm::vec3 qLocal = glm::clamp(pLocal, -b.HalfExtents, b.HalfExtents);
            return b.Center + b.Axes[0]*qLocal.x + b.Axes[1]*qLocal.y + b.Axes[2]*qLocal.z;
        }

        Sphere() = default;
        Sphere(const glm::vec3& center, float radius): Center(center), Radius(radius) {};
        ~Sphere() = default;
    };

    struct Capsule
    {
        static constexpr ColliderType Type = ColliderType::Capsule;

        glm::vec3 P0{0.0f};
        glm::vec3 P1{0.0f};
        float     Radius{0.5f};

        [[nodiscard]] static std::pair<float, glm::vec3> ClosestPointOnSegment(const glm::vec3& A, const glm::vec3& B, const glm::vec3& P)
        {
            glm::vec3 AB    = B - A;
            float len2      = glm::length2(AB);
            float t         = (len2 > 0.0f) ? glm::dot(P - A, AB) / len2 : 0.0f;
            t               = glm::clamp(t, 0.0f, 1.0f);

            return { t, A + t * AB };
        }

        [[nodiscard]] static void ClosestPointsOnSegments(const glm::vec3& A0, const glm::vec3& A1, const glm::vec3& B0, const glm::vec3& B1, float& s, float& t, glm::vec3& PA, glm::vec3& PB)
        {
            glm::vec3   u = A1 - A0;
            glm::vec3   v = B1 - B0;
            glm::vec3   w = A0 - B0;
            float       a = glm::length2(u);
            float       b = glm::dot(u, v);
            float       c = glm::length2(v);
            float       d = glm::dot(u, w);
            float       e = glm::dot(v, w);

            float D = a * c - b * b;
            float sN, sD = D;
            float tN, tD = D;

            if (D <= EPS_DIST) 
            {
                sN = 0.0f; sD = 1.0f;
                tN = e;    tD = c;
            }
            else
            {
                sN = (b*e - c*d);
                tN = (a*e - b*d);
                if (sN < 0.0f) { sN = 0.0f; tN = e; tD = c; }
                else if (sN > sD) { sN = sD; tN = e + b; tD = c; }
            }

            if (tN < 0.0f) 
            {
                tN = 0.0f;
                if (-d < 0.0f) sN = 0.0f;
                else if (-d > a) sN = sD;
                else { sN = -d; sD = a; }
            }
            else if (tN > tD) 
            {
                tN = tD;
                if ((-d + b) < 0.0f) sN = 0.0f;
                else if ((-d + b) > a) sN = sD;
                else { sN = (-d + b); sD = a; }
            }

            s = (std::abs(sN) <= EPS_DIST ? 0.0f : sN / sD);
            t = (std::abs(tN) <= EPS_DIST ? 0.0f : tN / tD);

            PA = A0 + s * u;
            PB = B0 + t * v;
        }

        [[nodiscard]] static bool SegmentIntersectsAABB(const glm::vec3& P0, const glm::vec3& P1, const glm::vec3& bmin, const glm::vec3& bmax)
        {
            glm::vec3 d = P1 - P0;
            float tmin = 0.0f, tmax = 1.0f;

            for (int i = 0; i < 3; ++i) {
                float p = P0[i], q = d[i];
                float minv = bmin[i], maxv = bmax[i];

                if (std::abs(q) < 1e-12f) {
                    if (p < minv || p > maxv) return false;
                } else {
                    float ood = 1.0f / q;
                    float t1 = (minv - p) * ood;
                    float t2 = (maxv - p) * ood;
                    if (t1 > t2) std::swap(t1, t2);
                    tmin = std::max(tmin, t1);
                    tmax = std::min(tmax, t2);
                    if (tmin > tmax) return false;
                }
            }
            return true;
        }

        [[nodiscard]] static bool SegmentIntersectsOBB(const glm::vec3& P0, const glm::vec3& P1, const glm::vec3& halfExtents)
        {
            glm::vec3 bmin = -halfExtents;
            glm::vec3 bmax = +halfExtents;
            return SegmentIntersectsAABB(P0, P1, bmin, bmax);
        }
    };

    struct ConvexHull
    {
        static constexpr ColliderType Type = ColliderType::ConvexHull;

        std::vector<glm::vec3> Vertices{};
        glm::vec3 Center{0.0f};
        glm::mat3 Axes{1.0f};

        [[nodiscard]] glm::vec3 ToWorld(const glm::vec3& p) const
        {
            return Center + Axes[0] * p.x + Axes[1] * p.y + Axes[2] * p.z;
        }
    
        [[nodiscard]] glm::vec3 Support(const glm::vec3& dWorld) const
        {
            const glm::vec3 dLocal = { glm::dot(dWorld, Axes[0]), glm::dot(dWorld, Axes[1]), glm::dot(dWorld, Axes[2]) };
            float best = -std::numeric_limits<float>::infinity();
    
            glm::vec3 vLocal(0.0f);
            for (const glm::vec3& v : Vertices) 
            {
                float s = glm::dot(v, dLocal);
                if (s > best) { best = s; vLocal = v; }
            }
    
            return ToWorld(vLocal);
        }

        [[nodiscard]] static  glm::vec3 Support(const OBB& b, const glm::vec3& d)
        {
            const glm::vec3 dL = { glm::dot(d, b.Axes[0]), glm::dot(d, b.Axes[1]), glm::dot(d, b.Axes[2]) };
            glm::vec3 vL = { (dL.x >= 0 ? b.HalfExtents.x : -b.HalfExtents.x),
                             (dL.y >= 0 ? b.HalfExtents.y : -b.HalfExtents.y),
                             (dL.z >= 0 ? b.HalfExtents.z : -b.HalfExtents.z) };

            return b.Center + b.Axes[0]*vL.x + b.Axes[1]*vL.y + b.Axes[2]*vL.z;
        }

        [[nodiscard]] static glm::vec3 Support(const AABB& a, const glm::vec3& d)
        {
            return {
                (d.x >= 0 ? a.MAX.x : a.MIN.x),
                (d.y >= 0 ? a.MAX.y : a.MIN.y),
                (d.z >= 0 ? a.MAX.z : a.MIN.z)
            };
        }

        [[nodiscard]] static glm::vec3 Support(const Sphere& s, const glm::vec3& d)
        {
            const float len2 = glm::length2(d);
            if (len2 < 1e-20f) return s.Center; 
            return s.Center + (s.Radius / std::sqrt(len2)) * d;
        }

        [[nodiscard]] static glm::vec3 Support(const Capsule& cap, const glm::vec3& d)
        {
            float len2 = glm::length2(d);
            if (len2 < 1e-20f) return 0.5f * (cap.P0 + cap.P1);   // degenerate direction
            glm::vec3 n = d / std::sqrt(len2);

            float p0 = glm::dot(cap.P0, d);
            float p1 = glm::dot(cap.P1, d);
            const glm::vec3 tip = (p0 >= p1) ? cap.P0 : cap.P1;

            return tip + n * cap.Radius;
        }

        [[nodiscard]] static glm::vec3 Support(const ConvexHull& h, const glm::vec3& d)
        {
            return h.Support(d);
        }

        template<typename SA, typename SB>
        [[nodiscard]] static glm::vec3 Support(const SA& A, const SB& B, const glm::vec3& d)
        {
            return Support(A, d) - Support(B, -d);
        }

        struct GJKResult
        {
            bool            HIT{false};
            glm::vec3       PTS[4]{};
            std::int32_t    Count{0};
        };

        [[nodiscard]] static bool SameDirection(const glm::vec3& a, const glm::vec3& b)
        {
            return glm::dot(a, b) > 0.0f;
        }

        template<typename SA, typename SB>
        [[nodiscard]] static GJKResult GJK(const SA& A, const SB& B)
        {
            GJKResult res{};
            auto centerOf = [](const auto& s) -> glm::vec3 
            {
                if      constexpr (std::is_same_v<std::decay_t<decltype(s)>, AABB>)         return (s.MIN + s.MAX) * 0.5f;
                else if constexpr (std::is_same_v<std::decay_t<decltype(s)>, OBB>)          return s.Center;
                else if constexpr (std::is_same_v<std::decay_t<decltype(s)>, Sphere>)       return s.Center;
                else if constexpr (std::is_same_v<std::decay_t<decltype(s)>, ConvexHull>)   return s.Center;
                else if constexpr (std::is_same_v<std::decay_t<decltype(s)>, Capsule>)      return 0.5f * (s.P0 + s.P1);
                else                                                                        return glm::vec3(0.0f);
            };

            glm::vec3 d = centerOf(B) - centerOf(A);
            if (glm::length2(d) < 1e-12f) d = glm::vec3(1,0,0);

            res.PTS[res.Count++] = Support(A, B, d);
            d = -res.PTS[0];

            for (int iter = 0; iter < MAX_GJK_ITERS; ++iter)
            {
                const glm::vec3 a = Support(A, B, d);
                if (glm::dot(a, d) <= EPS_GJK_DOT) { res.HIT = false; return res; }

                res.PTS[res.Count++] = a;

                if (res.Count == 2)
                {
                    glm::vec3 A = res.PTS[1], Bp = res.PTS[0];
                    glm::vec3 AB = Bp - A, AO = -A;
                    d = glm::cross(glm::cross(AB, AO), AB);
                    d = (glm::length2(d) < EPS_DIST2) ? SafeNormalize(glm::vec3(-AB.y, AB.x, 0.0f)) : d;
                }
                else if (res.Count == 3)
                {
                    glm::vec3 A = res.PTS[2], Bp = res.PTS[1], Cp = res.PTS[0];
                    glm::vec3 AB = Bp - A, AC = Cp - A, AO = -A;
                    glm::vec3 ABC = glm::cross(AB, AC);

                    glm::vec3 abPerp = glm::cross(ABC, AB);
                    if (SameDirection(abPerp, AO))
                    {
                        res.PTS[0] = Bp; res.PTS[1] = A; res.Count = 2;
                        d = glm::cross(glm::cross(AB, AO), AB);
                        d = (glm::length2(d) < EPS_DIST2) ? SafeNormalize(glm::vec3(-AB.y, AB.x, 0.0f)) : d;
                        continue;
                    }

                    glm::vec3 acPerp = glm::cross(AC, ABC);
                    if (SameDirection(acPerp, AO))
                    {
                        res.PTS[1] = A; res.Count = 2;
                        glm::vec3 AC2 = Cp - A;
                        d = glm::cross(glm::cross(AC2, AO), AC2);
                        d = (glm::length2(d) < EPS_DIST2) ? SafeNormalize(glm::vec3(-AC2.y, AC2.x, 0.0f)) : d;
                        continue;
                    }

                    d = SameDirection(ABC, AO) ? ABC : -ABC;
                    d = SafeNormalize(d);
                }
                else 
                {
                    glm::vec3 A = res.PTS[3], Bp = res.PTS[2], Cp = res.PTS[1], Dp = res.PTS[0];
                    glm::vec3 AO = -A;
                    glm::vec3 AB = Bp - A, AC = Cp - A, AD = Dp - A;

                    glm::vec3 ABC = glm::cross(AB, AC);
                    glm::vec3 ACD = glm::cross(AC, AD);
                    glm::vec3 ADB = glm::cross(AD, AB);

                    bool aboveABC = SameDirection(ABC, AO);
                    bool aboveACD = SameDirection(ACD, AO);
                    bool aboveADB = SameDirection(ADB, AO);

                    if (!aboveABC && !aboveACD && !aboveADB) { res.HIT = true; return res; }

                    if (aboveABC) { res.PTS[0] = Cp; res.PTS[1] = Bp; res.PTS[2] = A; res.Count = 3; d = SafeNormalize(ABC); continue; }
                    if (aboveACD) { res.PTS[0] = Dp; res.PTS[1] = Cp; res.PTS[2] = A; res.Count = 3; d = SafeNormalize(ACD); continue; }
                    if (aboveADB) { res.PTS[0] = Bp; res.PTS[1] = Dp; res.PTS[2] = A; res.Count = 3; d = SafeNormalize(ADB); continue; }
                }
            }

            res.HIT = false;
            return res;
        }

        struct EPAResult 
        {
            bool        HIT{false};
            glm::vec3   Normal{0.0f};
            float       Depth{0.0f};
        };

        struct EPAFace
        {
            std::int32_t    A{0}, B{0}, C{0};
            glm::vec3       N{0.0f};
            float           D{0.0f};
        };

        [[nodiscard]] static bool BuildFace(const std::vector<glm::vec3>& poly, std::int32_t a, std::int32_t b, std::int32_t c, EPAFace& out)
        {
            const glm::vec3& A = poly[a], &B = poly[b], &C = poly[c];

            glm::vec3 n = glm::cross(B - A, C - A);
            float n2    = glm::length2(n);
            if (n2 <= EPS_DIST2) return false; 

            n       = n / std::sqrt(n2);
            float d = glm::dot(n, A);
            if (d < 0.0f) { n = -n; d = -d; std::swap(b, c); }

            out = { a, b, c, n, d };
            return true;
        }

        template<typename SA, typename SB>
        [[nodiscard]] static EPAResult EPA(const SA& A, const SB& B, const GJKResult& gjk)
        {
            EPAResult R{};
            std::vector<glm::vec3> poly(gjk.PTS, gjk.PTS + gjk.Count);
            std::vector<EPAFace> faces{};

            if (poly.size() == 3) 
            {
                EPAFace f; BuildFace(poly, 0, 1, 2, f); faces.push_back(f);
            } 
            else if (poly.size() == 4) 
            {
                EPAFace f{};
                BuildFace(poly, 0, 1, 2, f); faces.push_back(f);
                BuildFace(poly, 0, 2, 3, f); faces.push_back(f);
                BuildFace(poly, 0, 3, 1, f); faces.push_back(f);
                BuildFace(poly, 1, 3, 2, f); faces.push_back(f);
            } 
            else 
            {
                glm::vec3 d = glm::vec3(1, 0, 0);
                poly.push_back(Support(A, B, d));

                EPAFace f{};
                BuildFace(poly, 0, 1, 2, f); faces.push_back(f);
                BuildFace(poly, 0, 2, 3, f); faces.push_back(f);
                BuildFace(poly, 0, 3, 1, f); faces.push_back(f);
                BuildFace(poly, 1, 3, 2, f); faces.push_back(f);
            }

            for (std::int32_t iter = 0; iter < 96; ++iter)
            {
                std::int32_t fi = -1; float best = std::numeric_limits<float>::infinity();
                for (std::int32_t i = 0; i < (std::int32_t)faces.size(); ++i) 
                {
                    float d = faces[i].D; 
                    if (d < best) { best = d; fi = i; }
                }

                EPAFace bestFace    = faces[fi];
                glm::vec3 p         = Support(A, B, bestFace.N);
                float pd            = glm::dot(bestFace.N, p);

                if ((pd - best) <= EPS_DIST) 
                {
                    R.HIT    = true;
                    R.Normal = bestFace.N;
                    R.Depth  = best;
                    return R;
                }

                std::int32_t startCount = (std::int32_t)poly.size();
                poly.push_back(p);
                std::int32_t newIndex   = (std::int32_t)poly.size() - 1;

                std::vector<std::int8_t> visible(faces.size(), 0);
                for (std::int32_t i = 0; i < (std::int32_t)faces.size(); ++i) 
                {
                    visible[i] = (glm::dot(faces[i].N, p - poly[faces[i].A]) > 1e-8f);
                }

                struct Edge { std::int32_t A{0}, B{0}; };
                std::vector<Edge> horizon;

                auto addEdge = [&](std::int32_t a, std::int32_t b)
                {
                    for (auto it = horizon.begin(); it != horizon.end(); ++it)
                    {
                        if ((it->A == b && it->B == a) || (it->A == a && it->B == b))
                        { horizon.erase(it); return; }
                    }
                    horizon.push_back({ a, b });
                };


                for (std::int32_t i = 0; i < (std::int32_t)faces.size(); ++i) 
                    if (visible[i]) 
                    {
                        const EPAFace& f = faces[i];
                        addEdge(f.A, f.B);
                        addEdge(f.B, f.C);
                        addEdge(f.C, f.A);
                    }

                
                for (std::int32_t i = (std::int32_t)faces.size() - 1; i >= 0; --i) 
                    if (visible[i]) faces.erase(faces.begin() + i);

                for (const auto& e : horizon) 
                {
                    EPAFace nf;
                    BuildFace(poly, e.A, e.B, newIndex, nf);
                    faces.push_back(nf);
                }
            }

            R.HIT       = true;
            R.Normal    = glm::vec3(1, 0, 0);
            R.Depth     = 0.0f;

            return R;
        }
    
        ConvexHull() = default;
        ConvexHull(const std::vector<glm::vec3>& verts,
                const glm::vec3& center = glm::vec3(0.0f),
                const glm::mat3& axes   = glm::mat3(1.0f))
            : Vertices(verts), Center(center), Axes(axes) {}

        ConvexHull(std::vector<glm::vec3>&& verts,
                const glm::vec3& center = glm::vec3(0.0f),
                const glm::mat3& axes   = glm::mat3(1.0f))
            : Vertices(std::move(verts)), Center(center), Axes(axes) {}

        ~ConvexHull() = default;
    };

    struct ConcaveMesh
    {
        static constexpr ColliderType Type = ColliderType::ConcaveMesh;

        std::vector<glm::vec3>      Vertices{};
        std::vector<std::uint32_t>  Indices{};

        glm::vec3 Center{0.0f};
        glm::mat3 Axes{1.0f};

        struct Triangle { std::uint32_t I0{0}, I1{0}, I2{0}; };
        struct Node
        {
            AABB Bounds{};
            std::uint32_t Left{std::numeric_limits<std::uint32_t>::max()};
            std::uint32_t Right{std::numeric_limits<std::uint32_t>::max()};
            std::uint32_t Begin{0}, End{0};

            [[nodiscard]] bool IsLeaf() const { return (Left == std::numeric_limits<std::uint32_t>::max() && Right == std::numeric_limits<std::uint32_t>::max()); }
        };

        std::vector<Triangle> Tris{};
        std::vector<Node>     Nodes{};

        [[nodiscard]] glm::vec3 ToWorld(const glm::vec3& pL) const
        {
            return Center + Axes[0] * pL.x + Axes[1] * pL.y + Axes[2] * pL.z;
        }

        [[nodiscard]] glm::vec3 ToLocal(const glm::vec3& pW) const 
        {
            glm::vec3 d = pW - Center; 
            return { glm::dot(d, Axes[0]), glm::dot(d, Axes[1]), glm::dot(d, Axes[2]) };
        }

        void Build()
        {
            Tris.clear();
            Tris.reserve(Indices.size() / 3);
            for(std::size_t t = 0; t < Indices.size(); t += 3)
                Tris.push_back({Indices[t + 0], Indices[t + 1], Indices[t + 2]});

            std::vector<AABB>  triAABB(Tris.size());
            std::vector<glm::vec3>          centroids(Tris.size());
            for(std::size_t i = 0; i < Tris.size(); i++)
            {
                const auto& tri         = Tris[i];
                AABB bb                 = AABB::Empty();

                bb.Expand(Vertices[tri.I0]);
                bb.Expand(Vertices[tri.I1]);
                bb.Expand(Vertices[tri.I2]);

                triAABB[i]      = bb;
                centroids[i]    = (Vertices[tri.I0] + Vertices[tri.I1] + Vertices[tri.I2]) * (1.0f / 3.0f);
            }

            std::vector<std::uint32_t> idx(Tris.size());
            std::iota(idx.begin(), idx.end(), 0);

            Nodes.clear();
            Nodes.reserve(Tris.size() * 2);
            std::function<std::uint32_t(std::uint32_t, std::uint32_t)> BuildRec =[&](std::uint32_t begin, std::uint32_t end) -> std::uint32_t
            {
                Node n{};
                n.Begin     = begin;
                n.End       = end;
                n.Bounds    = AABB::Empty();

                for(std::uint32_t k = begin; k < end; ++k)
                    n.Bounds.Expand(triAABB[idx[k]]);

                std::uint32_t nodeIndex = (std::uint32_t)Nodes.size();
                Nodes.push_back(n);

                const std::uint32_t count = end - begin;
                if(count <= 8)
                {
                    Nodes[nodeIndex].Left = Nodes[nodeIndex].Right = std::numeric_limits<std::uint32_t>::max();
                    return nodeIndex;
                }

                glm::vec3 ext = Nodes[nodeIndex].Bounds.Size();
                std::int32_t axis = (ext.x > ext.y && ext.x > ext.z) ? 0 : (ext.y > ext.z ? 1 : 2);

                std::uint32_t mid = begin + count / 2;
                std::nth_element(idx.begin() + begin, idx.begin() + mid, idx.begin() + end,
                    [&](std::uint32_t a, std::uint32_t b) { return centroids[a][axis] < centroids[b][axis]; });

                Nodes[nodeIndex].Left   = BuildRec(begin, mid);
                Nodes[nodeIndex].Right  = BuildRec(mid, end);
                return nodeIndex;
            };

            if(!Tris.empty()) BuildRec(0, (std::uint32_t)Tris.size());
        }

        [[nodiscard]] static bool Overlap_LocalAABB_Vs_WorldAABB(const AABB& localBox, const AABB& worldBox, const glm::vec3& center, const glm::mat3& axes)
        {
            AABB wbLocal                = AABB::Empty();
            const glm::vec3 wMIN        = worldBox.MIN;
            const glm::vec3 wMAX        = worldBox.MAX;

            for(std::int32_t XI = 0; XI < 2; ++XI) for(std::int32_t YI = 0; YI < 2; ++YI) for(std::int32_t ZI = 0; ZI < 2; ++ZI)
            {
                glm::vec3 pW = { XI ? wMAX.x : wMIN.x, YI ? wMAX.y : wMIN.y, ZI ? wMAX.z : wMIN.z };
                glm::vec3 pL;

                {
                    glm::vec3 d = pW - center;
                    pL = { glm::dot(d, axes[0]), glm::dot(d, axes[1]), glm::dot(d, axes[2]) };
                }

                wbLocal.Expand(pL);
            }
                                 
            return localBox.Overlaps(wbLocal);       
        }

        [[nodiscard]] static AABB WorldAABB(const Sphere& s)
        {
            AABB b{};
            b.MIN = s.Center - glm::vec3(s.Radius);
            b.MAX = s.Center + glm::vec3(s.Radius);
            return b;
        }

        [[nodiscard]] static AABB WorldAABB(const OBB& o)
        {
            glm::mat3 R     = o.Axes;
            glm::vec3 e     = o.HalfExtents;
            glm::vec3 ax    = glm::abs(glm::vec3(R[0])); 
            glm::vec3 ay    = glm::abs(glm::vec3(R[1]));
            glm::vec3 az    = glm::abs(glm::vec3(R[2]));
            glm::vec3 r     = ax * e.x + ay * e.y + az * e.z;

            AABB b{}; 
            b.MIN = o.Center - r; 
            b.MAX = o.Center + r; 
            return b;
        }

        [[nodiscard]] static AABB WorldAABB(const AABB& a) 
        { 
            return a; 
        }

        [[nodiscard]] static AABB WorldAABB(const ConvexHull& h)
        {
            AABB b = AABB::Empty(); 
            for (auto& vL : h.Vertices) 
                b.Expand(h.ToWorld(vL)); 

            return b;
        }

        [[nodiscard]] static AABB WorldAABB(const Capsule& cap)
        {
            glm::vec3 pmin = glm::min(cap.P0, cap.P1);
            glm::vec3 pmax = glm::max(cap.P0, cap.P1);

            AABB b;
            b.MIN = pmin - glm::vec3(cap.Radius);
            b.MAX = pmax + glm::vec3(cap.Radius);
            return b;
        }


        [[nodiscard]] static glm::vec3 ClosestPointOnTri(const glm::vec3& p, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
        {
            const glm::vec3 ab = b - a, ac = c - a, ap = p - a;

            float d1 = glm::dot(ab, ap), d2 = glm::dot(ac, ap);
            if (d1 <= 0.f && d2 <= 0.f) return a;

            const glm::vec3 bp = p - b; float d3 = glm::dot(ab, bp), d4 = glm::dot(ac, bp);
            if (d3 >= 0.f && d4 <= d3) return b;

            float vc = d1 * d4 - d3 * d2;
            if (vc<=0.f && d1>=0.f && d3<=0.f) 
            { 
                float v = d1 / (d1-d3); 
                return a + v * ab; 
            }
            const glm::vec3 cp = p - c; float d5 = glm::dot(ab, cp), d6 = glm::dot(ac, cp);
            if (d6 >= 0.f && d5 <= d6) return c;

            float vb = d5 * d2 - d1 * d6;
            if (vb<=0.f && d2>=0.f && d6<=0.f) 
            {  
                float w = d2 / (d2-d6); 
                return a + w * ac; 
            }

            float va = d3 * d6 - d5 * d4;
            if (va <= 0.f && (d4-d3) >= 0.f && (d5 - d6) >= 0.f) 
            { 
                float w = (d4 - d3)/((d4 - d3) + (d5 - d6)); 
                return b + w  * (c - b); 
            }

            float denom = 1.0f / (va + vb + vc);
            float v = vb * denom, w = vc * denom;
            return a + ab * v + ac * w;
        }

        [[nodiscard]] static CollisionResult SphereTriManifold(const Sphere& s, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
        {
            CollisionResult m{};
            glm::vec3 q = ClosestPointOnTri(s.Center, a, b, c);
            glm::vec3 v = q - s.Center;

            float d2    = glm::length2(v);
            float r     = s.Radius;
            if (d2 > r * r) return m;

            if (d2 > 1e-12f) 
            { 
                float d     = std::sqrt(d2); 
                m.HIT       = true; 
                m.Normal    = v / d; 
                m.Depth     = r - d; 

                return m; 
            }

            glm::vec3 n = glm::cross(b - a, c - a);
            if (glm::length2(n) <= EPS_DIST2)
            {
                glm::vec3 centroid = (a + b + c) * (1.0f / 3.0f);
                n = SafeNormalize(centroid - s.Center);
            }
            else
            {
                n = SafeNormalize(n);
            }

            m.HIT    = true;
            m.Normal = n;
            m.Depth  = r;
            return m;
        }

        [[nodiscard]] static CollisionResult CapsuleTriManifold(const Capsule& cap, const glm::vec3& a, const glm::vec3& b, const glm::vec3& c)
        {
            CollisionResult best{}; 
            float bestDepth = -std::numeric_limits<float>::infinity();

            glm::vec3 mids = 0.5f*(cap.P0+cap.P1);
            glm::vec3 triQ0 = ClosestPointOnTri(cap.P0, a,b,c);
            glm::vec3 triQ1 = ClosestPointOnTri(cap.P1, a,b,c);
            glm::vec3 triQm = ClosestPointOnTri(mids,  a,b,c);

            auto sphereify = [&](const glm::vec3& center)-> CollisionResult
            {
                Sphere s{}; 
                s.Center    = center; 
                s.Radius    = cap.Radius;
                return SphereTriManifold(s, a, b, c);
            };

            for (auto& cand : {triQ0, triQm, triQ1}) 
            {
                auto m = sphereify(cand);
                if (m.HIT && m.Depth>bestDepth) 
                { 
                    best = m; 
                    bestDepth = m.Depth; 
                }
            }
            return best;
        }

        [[nodiscard]] static CollisionResult TriVsConvexHullManifold(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const ConvexHull& other)
        {
            ConvexHull triHull;
            triHull.Vertices = {a,b,c};
            triHull.Center   = glm::vec3(0);
            triHull.Axes     = glm::mat3(1.0f);

            CollisionResult out{};
            auto gjk = ConvexHull::GJK(triHull, other);
            if (!gjk.HIT) return out;

            auto epa = ConvexHull::EPA(triHull, other, gjk);
            if (!epa.HIT) return out;

            out.HIT    = true;
            out.Normal = epa.Normal;   // A→B (triangle hull → other hull)
            out.Depth  = epa.Depth;
            return out;
        }

        [[nodiscard]] static CollisionResult TriVsOBBManifold(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const OBB& obb)
        {
            ConvexHull obbHull;
            obbHull.Vertices = {
                {-obb.HalfExtents.x, -obb.HalfExtents.y, -obb.HalfExtents.z},
                {-obb.HalfExtents.x, -obb.HalfExtents.y, +obb.HalfExtents.z},
                {-obb.HalfExtents.x, +obb.HalfExtents.y, -obb.HalfExtents.z},
                {-obb.HalfExtents.x, +obb.HalfExtents.y, +obb.HalfExtents.z},
                {+obb.HalfExtents.x, -obb.HalfExtents.y, -obb.HalfExtents.z},
                {+obb.HalfExtents.x, -obb.HalfExtents.y, +obb.HalfExtents.z},
                {+obb.HalfExtents.x, +obb.HalfExtents.y, -obb.HalfExtents.z},
                {+obb.HalfExtents.x, +obb.HalfExtents.y, +obb.HalfExtents.z},
            };
            obbHull.Center = obb.Center;
            obbHull.Axes   = obb.Axes;

            ConvexHull triHull;
            triHull.Vertices = {a,b,c};
            triHull.Center   = glm::vec3(0);
            triHull.Axes     = glm::mat3(1);

            CollisionResult out{};
            auto gjk = ConvexHull::GJK(triHull, obbHull);
            if (!gjk.HIT) return out;

            auto epa = ConvexHull::EPA(triHull, obbHull, gjk);
            if (!epa.HIT) return out;

            out.HIT    = true;
            out.Normal = epa.Normal;   // A→B (triangle hull → obb hull)
            out.Depth  = epa.Depth;
            return out;
        }


        template<class Shape>
        [[nodiscard]] bool Intersect(const Shape& shape) const
        {
            AABB wAABB = WorldAABB(shape);

            if (Nodes.empty()) return false;
            std::vector<std::uint32_t> stack; stack.push_back(0);
            while (!stack.empty())
            {
                std::uint32_t nidx  = stack.back(); stack.pop_back();
                const Node& N       = Nodes[nidx];
                if (!Overlap_LocalAABB_Vs_WorldAABB(N.Bounds, wAABB, Center, Axes)) continue;

                if (N.IsLeaf()) 
                {
                    for (std::uint32_t k = N.Begin; k < N.End; ++k) 
                    {
                        std::uint32_t triIdx    = k; 
                        const Triangle& t       = Tris[ triIdx ];

                        glm::vec3 a = ToWorld(Vertices[t.I0]);
                        glm::vec3 b = ToWorld(Vertices[t.I1]);
                        glm::vec3 c = ToWorld(Vertices[t.I2]);

                        if constexpr (std::is_same_v<Shape, Sphere>) 
                        {
                            if (SphereTriManifold(shape, a, b, c).HIT) return true;
                        } 
                        else if constexpr (std::is_same_v<Shape, Capsule>) 
                        {
                            if (CapsuleTriManifold(shape, a, b, c).HIT) return true;
                        } 
                        else if constexpr (std::is_same_v<Shape, OBB>) 
                        {
                            if (TriVsOBBManifold(a, b, c, shape).HIT) return true;
                        } 
                        else if constexpr (std::is_same_v<Shape, AABB>) 
                        {
                            OBB obb{}; 
                            obb.Center          = 0.5f * (shape.MIN + shape.MAX);
                            obb.Axes            = glm::mat3(1); 
                            obb.HalfExtents     = shape.HalfExtents();

                            if (TriVsOBBManifold(a, b, c, obb).HIT) return true;
                        } 
                        else if constexpr (std::is_same_v<Shape, ConvexHull>) 
                        {
                            if (TriVsConvexHullManifold(a, b, c, shape).HIT) return true;
                        }
                    }
                } 
                else 
                {
                    stack.push_back(N.Left); 
                    stack.push_back(N.Right);
                }
            }

            return false;
        }

        template<class Shape>
        [[nodiscard]] CollisionResult IntersectCollision(const Shape& shape) const
        {
            CollisionResult best{}; float bestDepth = -std::numeric_limits<float>::infinity();
            AABB wAABB = WorldAABB(shape);

            if (Nodes.empty()) return best;
            std::vector<std::uint32_t> stack; stack.push_back(0);
            while (!stack.empty())
            {
                std::uint32_t nidx = stack.back(); stack.pop_back();
                const Node& N = Nodes[nidx];
                if (!Overlap_LocalAABB_Vs_WorldAABB(N.Bounds, wAABB, Center, Axes)) continue;

                if (N.IsLeaf()) 
                {
                    for (uint32_t k=N.Begin; k < N.End; ++k) 
                    {
                        const Triangle& t = Tris[k];
                        glm::vec3 a = ToWorld(Vertices[t.I0]);
                        glm::vec3 b = ToWorld(Vertices[t.I1]);
                        glm::vec3 c = ToWorld(Vertices[t.I2]);

                        CollisionResult m{};
                        if constexpr (std::is_same_v<Shape, Sphere>) 
                        {
                            m = SphereTriManifold(shape,a,b,c);
                        } 
                        else if constexpr (std::is_same_v<Shape, Capsule>) 
                        {
                            m = CapsuleTriManifold(shape,a,b,c);
                        } 
                        else if constexpr (std::is_same_v<Shape, OBB>) 
                        {
                            m = TriVsOBBManifold(a,b,c, shape);
                        }
                        else if constexpr (std::is_same_v<Shape, AABB>) 
                        {
                            OBB obb; 
                            obb.Center      = 0.5f * (shape.MIN + shape.MAX);
                            obb.Axes        = glm::mat3(1); 
                            obb.HalfExtents = shape.HalfExtents();

                            m = TriVsOBBManifold(a, b, c, obb);

                        } 
                        else if constexpr (std::is_same_v<Shape, ConvexHull>) 
                        {
                            m = TriVsConvexHullManifold(a,b,c, shape);
                        }

                        if (m.HIT && m.Depth > bestDepth) { best = m; bestDepth = m.Depth; }
                    }
                } 
                else 
                {
                    stack.push_back(N.Left); 
                    stack.push_back(N.Right);
                }
            }

            return best;
        }
    
    
        ConcaveMesh() = default;
        ConcaveMesh(const std::vector<glm::vec3>& vertices, const std::vector<std::uint32_t>& indices, const glm::vec3& center = glm::vec3(0.0f), const glm::mat3& axes   = glm::mat3(1.0f), bool build_now = true)
            : Vertices(vertices), Indices(indices), Center(center), Axes(axes)
        {
            MOTION_ASSERT(indices.size() % 3 == 0, "ConcaveMesh: Indices must be a multiple of 3");
#ifndef MOTION_BUILD_DEBUG
            for (auto i : Indices) { MOTION_ASSERT(i < Vertices.size(),"ConcaveMesh: index out of range"); }
#endif
            if (build_now) Build();
        }

        ConcaveMesh(std::vector<glm::vec3>&& vertices, std::vector<std::uint32_t>&& indices, const glm::vec3& center = glm::vec3(0.0f), const glm::mat3& axes   = glm::mat3(1.0f), bool build_now = true)
            : Vertices(std::move(vertices)), Indices(std::move(indices)), Center(center), Axes(axes)
        {
            MOTION_ASSERT(Indices.size() % 3 == 0, "ConcaveMesh: Indices must be a multiple of 3");
#ifndef MOTION_BUILD_DEBUG
            for (auto i : Indices) { MOTION_ASSERT(i < Vertices.size(), "ConcaveMesh: index out of range"); }
 #endif
            if (build_now) Build();
        }

        ~ConcaveMesh() = default;
    };

    #pragma region Box Collision
    
    template<>
    struct CollisionDetector<AABB, AABB>
    {
        [[nodiscard]] static bool Intersect(const AABB& a, const AABB& b)
        {
            return a.Overlaps(b);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const AABB& a, const AABB& b)
        {
            CollisionResult bc;
            if(!a.Overlaps(b)) return bc;

            const glm::vec3 aMin = a.MIN, aMax = a.MAX;
            const glm::vec3 bMin = b.MIN, bMax = b.MAX;

            const float dx1 = bMax.x - aMin.x; 
            const float dx2 = aMax.x - bMin.x;
            const float dy1 = bMax.y - aMin.y;
            const float dy2 = aMax.y - bMin.y;
            const float dz1 = bMax.z - aMin.z;
            const float dz2 = aMax.z - bMin.z;

            float pushX = (dx1 < dx2) ? +dx1 : -dx2;
            float pushY = (dy1 < dy2) ? +dy1 : -dy2;
            float pushZ = (dz1 < dz2) ? +dz1 : -dz2;

            glm::vec3 mtv = { pushX, pushY, pushZ };
            float ax = std::abs(pushX), ay = std::abs(pushY), az = std::abs(pushZ);

            glm::vec3 cA = a.Center(), cB = b.Center();
            glm::vec3 dir = glm::sign(cB - cA);

            if (ax <= ay && ax <= az)      mtv = { dir.x * ax, 0.0f, 0.0f };
            else if (ay <= ax && ay <= az) mtv = { 0.0f, dir.y * ay, 0.0f };
            else                           mtv = { 0.0f, 0.0f, dir.z * az };

            bc.HIT      = true;
            bc.Depth    = glm::length(mtv);
            bc.Normal   = (bc.Depth > 0.0f) ? (mtv / bc.Depth) : glm::vec3(1, 0, 0);
            return bc;
        }
    };

    template<>
    struct CollisionDetector<AABB, OBB>
    {
        [[nodiscard]] static CollisionResult IntersectCollision(const AABB& a, const OBB& b)
        {
            CollisionResult out;
            const glm::vec3 cA = a.Center();
            const glm::vec3 eA = a.HalfExtents();
            const glm::vec3 cB = b.Center;

            const glm::vec3 Ax[3] = { {1, 0, 0}, {0, 1, 0}, {0, 0, 1} };
            const glm::vec3 Bx[3] = { b.Axes[0], b.Axes[1], b.Axes[2] };

            float minOverlap = std::numeric_limits<float>::infinity();
            glm::vec3 bestNormal(0.0f);

            const glm::vec3 t = cB - cA;

            for (int i = 0; i < 3; ++i)
            {
                const glm::vec3& L  = Ax[i];
                const float tProj   = glm::dot(t, L);
                const float rA      = (i==0 ? eA.x : (i==1 ? eA.y : eA.z));
                const float rB      = Box::ProjectRadiusOBB(b, L);

                if(!Box::ConsiderAxisNormalized(L, tProj, rA, rB, minOverlap, bestNormal)) return out;
            }

            for (int i = 0; i < 3; ++i)
            {
                const glm::vec3& L  = glm::normalize(Bx[i]);
                const float tProj   = glm::dot(t, L);
                const float rA      = Box::ProjectRadiusAABB(a, L);
                const float rB      = Box::ProjectRadiusOBB(b, L);

                if(!Box::ConsiderAxisNormalized(L, tProj, rA, rB, minOverlap, bestNormal)) return out;
            }

            for (int i = 0; i < 3; ++i) 
            {
                for (int j = 0; j < 3; ++j)
                {
                    const glm::vec3 L   = glm::cross(Ax[i], Bx[j]);
                    float L2            = glm::length2(L);
                    if(L2 < 1e-8f) continue;

                    glm::vec3 n         = L / std::sqrt(L2);
                    const float tProj   = glm::dot(t, n);
                    const float rA      = Box::ProjectRadiusAABB(a, n);
                    const float rB      = Box::ProjectRadiusOBB(b, n);

                    if(!Box::ConsiderAxisNormalized(n, tProj, rA, rB, minOverlap, bestNormal)) return out;
                }
            }

            out.HIT     = true;
            out.Normal  = bestNormal;
            out.Depth   = (minOverlap == std::numeric_limits<float>::infinity()) ? 0.0f : minOverlap;
            return out;
        }

        [[nodiscard]] static bool Intersect(const AABB& a, const OBB& b)
        {
            CollisionResult bc = IntersectCollision(a, b);
            return bc.HIT;
        }
    };

    template<>
    struct CollisionDetector<OBB, OBB>
    {
        [[nodiscard]] static CollisionResult IntersectCollision(const OBB& a, const OBB& b)
        {
            CollisionResult out;

            const glm::vec3 cA = a.Center;
            const glm::vec3 eA = a.HalfExtents;
            const glm::vec3 cB = b.Center;
            const glm::vec3 eB = b.HalfExtents;
    
            const glm::vec3 A[3] = { a.Axes[0], a.Axes[1], a.Axes[2] };
            const glm::vec3 B[3] = { b.Axes[0], b.Axes[1], b.Axes[2] };
    
            const glm::vec3 t = cB - cA;
    
            float minOverlap = std::numeric_limits<float>::infinity();
            glm::vec3 bestNormal(0.0f);
    
            for (int i = 0; i < 3; ++i)
            {
                const glm::vec3& L  = glm::normalize(A[i]);
                const float tProj   = glm::dot(t, L);
                const float rA      = (i == 0 ? eA.x : (i == 1 ? eA.y : eA.z));
                const float rB      = Box::ProjectRadiusOBB(b, L);

                if(!Box::ConsiderAxisNormalized(L, tProj, rA, rB, minOverlap, bestNormal)) return out;
            }
    
            for (int i = 0; i < 3; ++i)
            {
                const glm::vec3& L  = glm::normalize(B[i]);
                const float tProj   = glm::dot(t, L);
                const float rA      = Box::ProjectRadiusOBB(a, L);
                const float rB      = (i == 0 ? eB.x : (i == 1 ? eB.y : eB.z));
    
                if(!Box::ConsiderAxisNormalized(L, tProj, rA, rB, minOverlap, bestNormal)) return out;
            }
    
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    const glm::vec3 L   = glm::cross(A[i], B[j]);
                    float L2            = glm::length2(L);
                    if(L2 < 1e-8f) continue;

                    glm::vec3 n         = L / std::sqrt(L2);
                    const float tProj   = glm::dot(t, n);
                    const float rA      = Box::ProjectRadiusOBB(a, n);
                    const float rB      = Box::ProjectRadiusOBB(b, n);
    
                    if(!Box::ConsiderAxisNormalized(n, tProj, rA, rB, minOverlap, bestNormal)) return out;
                }
            }
    
            out.HIT     = true;
            out.Normal  = bestNormal;  // from A to B
            out.Depth   = (minOverlap == std::numeric_limits<float>::infinity()) ? 0.0f : minOverlap;
            return out;
        }

        [[nodiscard]] static bool Intersect(const OBB& a, const OBB& b)
        {
            CollisionResult bc = IntersectCollision(a, b);
            return bc.HIT;
        }
    };

    template<>
    struct CollisionDetector<OBB, AABB>
    {
        [[nodiscard]] static bool Intersect(const OBB& a, const AABB& b)
        {
            return CollisionDetector<AABB, OBB>::Intersect(b, a);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const OBB& a, const AABB& b)
        {
            CollisionResult m = CollisionDetector<AABB, OBB>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal; // ensure normal points A->B
            return m;
        }
    };

    #pragma endregion 
    #pragma region Sphere Collision

    template<>
    struct CollisionDetector<Sphere, Sphere>
    {
        [[nodiscard]] static bool Intersect(const Sphere& a, const Sphere& b)
        {
            glm::vec3 d = b.Center - a.Center;
            float r     = a.Radius + b.Radius;

            return glm::length2(d) <= r * r;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Sphere& a, const Sphere& b)
        {
            CollisionResult m;
            glm::vec3 d = b.Center - a.Center;
            float dist2 = glm::length2(d);
            float r     = a.Radius + b.Radius;

            if (dist2 > r*r) return m;

            float dist = std::sqrt(std::max(dist2, 1e-12f));
            if (dist > 1e-6f)
            {
                m.HIT       = true;
                m.Normal    = d / dist;     
                m.Depth     = r - dist;
                return m;
            }

            m.HIT       = true;
            m.Normal    = glm::vec3(1,0,0);
            m.Depth     = r;

            return m;
        }
    };

    template<>
    struct CollisionDetector<Sphere, AABB>
    {
        [[nodiscard]] static bool Intersect(const Sphere& a, const AABB& b)
        {
            glm::vec3 q = Sphere::ClosestPointOnAABB(a.Center, b);
            glm::vec3 v = q - a.Center;
            return glm::length2(v) <= a.Radius * a.Radius;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Sphere& a, const AABB& b)
        {
            CollisionResult m;

            glm::vec3 q     = Sphere::ClosestPointOnAABB(a.Center, b);
            glm::vec3 v     = q - a.Center;             
            float dist2     = glm::length2(v);
            float r         = a.Radius;

            if (dist2 > r*r) return m;

            if (dist2 > 1e-12f)
            {
                float dist  = std::sqrt(dist2);
                m.HIT       = true;
                m.Normal    = v / dist;                 
                m.Depth     = r - dist;

                return m;
            }

            glm::vec3 c = a.Center;
            float dxMin = c.x - b.MIN.x;
            float dxMax = b.MAX.x - c.x;
            float dyMin = c.y - b.MIN.y;
            float dyMax = b.MAX.y - c.y;
            float dzMin = c.z - b.MIN.z;
            float dzMax = b.MAX.z - c.z;

            float minDist = dxMin; glm::vec3 n = {-1,0,0};
            if (dxMax < minDist) { minDist = dxMax; n = {+1,0,0}; }
            if (dyMin < minDist) { minDist = dyMin; n = {0,-1,0}; }
            if (dyMax < minDist) { minDist = dyMax; n = {0,+1,0}; }
            if (dzMin < minDist) { minDist = dzMin; n = {0,0,-1}; }
            if (dzMax < minDist) { minDist = dzMax; n = {0,0,+1}; }

            m.HIT       = true;
            m.Normal    = n;               
            m.Depth     = r + minDist;
                
            return m;
        }
    };

    template<>
    struct CollisionDetector<Sphere, OBB>
    {
        [[nodiscard]] static bool Intersect(const Sphere& a, const OBB& b)
        {
            glm::vec3 q = Sphere::ClosestPointOnOBB(a.Center, b);
            glm::vec3 v = q - a.Center;

            return glm::length2(v) <= a.Radius * a.Radius;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Sphere& a, const OBB& b)
        {
            CollisionResult m;
            glm::vec3 q     = Sphere::ClosestPointOnOBB(a.Center, b);
            glm::vec3 v     = q - a.Center;        
            float dist2     = glm::length2(v);
            float r         = a.Radius;

            if (dist2 > r*r) return m;

            if (dist2 > 1e-12f)
            {
                float dist = std::sqrt(dist2);
                m.HIT = true;
                m.Normal = v / dist;           
                m.Depth  = r - dist;
                return m;
            }

            glm::vec3 d = a.Center - b.Center;
            glm::vec3 pLocal = { 
                glm::dot(d, b.Axes[0]),
                glm::dot(d, b.Axes[1]),
                glm::dot(d, b.Axes[2]) 
            };

            glm::vec3 e = b.HalfExtents;
            float dxMin = pLocal.x + e.x; 
            float dxMax = e.x - pLocal.x; 
            float dyMin = pLocal.y + e.y;
            float dyMax = e.y - pLocal.y;
            float dzMin = pLocal.z + e.z;
            float dzMax = e.z - pLocal.z;

            float minDist = dxMin; glm::vec3 nLocal = {-1,0,0};
            if (dxMax < minDist) { minDist = dxMax; nLocal = {+1,0,0}; }
            if (dyMin < minDist) { minDist = dyMin; nLocal = {0,-1,0}; }
            if (dyMax < minDist) { minDist = dyMax; nLocal = {0,+1,0}; }
            if (dzMin < minDist) { minDist = dzMin; nLocal = {0,0,-1}; }
            if (dzMax < minDist) { minDist = dzMax; nLocal = {0,0,+1}; }

            glm::vec3 nWorld = b.Axes[0] * nLocal.x + b.Axes[1] * nLocal.y + b.Axes[2] * nLocal.z;

            m.HIT       = true;
            m.Normal    = glm::normalize(nWorld); 
            m.Depth     = r + minDist;

            return m;
        }
    };

    template<>
    struct CollisionDetector<OBB, Sphere>
    {
        [[nodiscard]] static bool Intersect(const OBB& a, const Sphere& b)
        { 
            return CollisionDetector<Sphere, OBB>::Intersect(b, a); 
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const OBB& a, const Sphere& b)
        {
            CollisionResult m = CollisionDetector<Sphere, OBB>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    template<>
    struct CollisionDetector<AABB, Sphere>
    {
        [[nodiscard]] static bool Intersect(const AABB& a, const Sphere& b)
        { 
            return CollisionDetector<Sphere, AABB>::Intersect(b, a); 
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const AABB& a, const Sphere& b)
        {
            CollisionResult m = CollisionDetector<Sphere, AABB>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    #pragma endregion
    #pragma region Capsule Collision

    template<>
    struct CollisionDetector<Capsule, Capsule>
    {
        [[nodiscard]] static bool Intersect(const Capsule& A, const Capsule& B)
        {
            float s, t;
            glm::vec3 pA{0.0f}, pB{0.0f};
            Capsule::ClosestPointsOnSegments(A.P0, A.P1, B.P0, B.P1, s, t, pA, pB);
            glm::vec3 d = pB - pA;
            float R = A.Radius + B.Radius;
            return glm::length2(d) <= R*R;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Capsule& A, const Capsule& B)
        {
            CollisionResult m;
            float s{0.0f}, t{0.0f};
            glm::vec3 pA{}, pB{0.0f};
            Capsule::ClosestPointsOnSegments(A.P0, A.P1, B.P0, B.P1, s, t, pA, pB);

            glm::vec3 d     = pB - pA;
            float dist2     = glm::length2(d);
            float R         = A.Radius + B.Radius;
            if (dist2 > R*R) return m;

            float dist = std::sqrt(std::max(dist2, 1e-12f));
            if (dist > 1e-6f) 
            {
                m.HIT = true; m.Normal = (dist > 0.0f ? d / dist : glm::vec3(1,0,0)); m.Depth = R - dist; return m;
            }

            glm::vec3 axis = A.P1 - A.P0;
            if (glm::length2(axis) < 1e-12f) axis = glm::vec3(1,0,0);
            else axis = glm::normalize(axis);
            m.HIT = true; m.Normal = axis; m.Depth = R; return m;
        }
    };

    template<>
    struct CollisionDetector<Capsule, Sphere>
    {
        [[nodiscard]] static bool Intersect(const Capsule& cap, const Sphere& sph)
        {
            auto [t, c] = Capsule::ClosestPointOnSegment(cap.P0, cap.P1, sph.Center);
            glm::vec3 d = sph.Center - c;
            float R = cap.Radius + sph.Radius;
            return glm::length2(d) <= R*R;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Capsule& cap, const Sphere& sph)
        {
            CollisionResult m;
            auto [t, c] = Capsule::ClosestPointOnSegment(cap.P0, cap.P1, sph.Center);
            glm::vec3 d = sph.Center - c;
            float dist2 = glm::length2(d);
            float R = cap.Radius + sph.Radius;
            if (dist2 > R*R) return m;

            float dist = std::sqrt(std::max(dist2, 1e-12f));
            if (dist > 1e-6f) 
            {
                m.HIT = true; m.Normal = d / dist; m.Depth = R - dist; return m;
            }

            glm::vec3 axis = cap.P1 - cap.P0;
            if (glm::length2(axis) > 1e-12f) axis = glm::normalize(axis); else axis = glm::vec3(1,0,0);
            m.HIT = true; m.Normal = axis; m.Depth = R; return m;
        }
    };

    template<>
    struct CollisionDetector<Capsule, AABB>
    {
        [[nodiscard]] static bool Intersect(const Capsule& cap, const AABB& box)
        {
            const glm::vec3 bmin = box.MIN - glm::vec3(cap.Radius);
            const glm::vec3 bmax = box.MAX + glm::vec3(cap.Radius);
            return Capsule::SegmentIntersectsAABB(cap.P0, cap.P1, bmin, bmax);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Capsule& cap, const AABB& box)
        {
            CollisionResult m;

            const glm::vec3 boxCenter = 0.5f * (box.MIN + box.MAX);
            auto [t, pStar] = Capsule::ClosestPointOnSegment(cap.P0, cap.P1, boxCenter);

            const glm::vec3 q = glm::clamp(pStar, box.MIN, box.MAX);          
            const glm::vec3 v = q - pStar;                                        
            const float dist2 = glm::length2(v);
            const float r = cap.Radius;

            if (dist2 > r * r) return m;

            if (dist2 > 1e-12f)
            {
                const float dist = std::sqrt(dist2);
                m.HIT    = true;
                m.Normal = v / dist;                     
                m.Depth  = r - dist;                   
                return m;
            }

            const glm::vec3 c = pStar;
            const float dxMin = c.x - box.MIN.x, dxMax = box.MAX.x - c.x;
            const float dyMin = c.y - box.MIN.y, dyMax = box.MAX.y - c.y;
            const float dzMin = c.z - box.MIN.z, dzMax = box.MAX.z - c.z;

            float minDist = dxMin; glm::vec3 n = {-1,0,0};
            if (dxMax < minDist) { minDist = dxMax; n = {+1,0,0}; }
            if (dyMin < minDist) { minDist = dyMin; n = {0,-1,0}; }
            if (dyMax < minDist) { minDist = dyMax; n = {0,+1,0}; }
            if (dzMin < minDist) { minDist = dzMin; n = {0,0,-1}; }
            if (dzMax < minDist) { minDist = dzMax; n = {0,0,+1}; }

            m.HIT    = true;
            m.Normal = n;                 
            m.Depth  = r + minDist;       
            return m;
        }
    };

    template<>
    struct CollisionDetector<Capsule, OBB>
    {
        [[nodiscard]] static bool Intersect(const Capsule& cap, const OBB& box)
        {
            const glm::vec3 c = box.Center;
            const glm::mat3 R = box.Axes; 
            auto toLocal = [&](const glm::vec3& p) -> glm::vec3 
            {
                glm::vec3 d = p - c;
                return { glm::dot(d, R[0]), glm::dot(d, R[1]), glm::dot(d, R[2]) };
            };

            glm::vec3 p0L = toLocal(cap.P0);
            glm::vec3 p1L = toLocal(cap.P1);
            glm::vec3 half = box.HalfExtents + glm::vec3(cap.Radius);
            return Capsule::SegmentIntersectsOBB(p0L, p1L, half);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Capsule& cap, const OBB& box)
        {
            CollisionResult m;

            const glm::vec3 cB = box.Center;
            const glm::mat3 R  = box.Axes;

            auto toLocal = [&](const glm::vec3& p)->glm::vec3 {
                const glm::vec3 d = p - cB;
                return { glm::dot(d, R[0]), glm::dot(d, R[1]), glm::dot(d, R[2]) };
            };

            glm::vec3 p0L = toLocal(cap.P0);
            glm::vec3 p1L = toLocal(cap.P1);

            auto [t0, pStarL] = Capsule::ClosestPointOnSegment(p0L, p1L, glm::vec3(0.0f));
            glm::vec3 qL = glm::clamp(pStarL, -box.HalfExtents, box.HalfExtents);

            glm::vec3 vL   = qL - pStarL;
            float dist2    = glm::length2(vL);
            float r        = cap.Radius;

            if (dist2 > r*r) return m;

            if (dist2 > EPS_DIST2)
            {
                float dist = std::sqrt(dist2);
                glm::vec3 nLocal = vL / dist;
                glm::vec3 nWorld = box.Axes[0]*nLocal.x + box.Axes[1]*nLocal.y + box.Axes[2]*nLocal.z;

                m.HIT    = true;
                m.Normal = SafeNormalize(nWorld);        // A -> B
                m.Depth  = r - dist;
                return m;
            }

            glm::vec3 cL = pStarL;
            float dxMin = cL.x + box.HalfExtents.x, dxMax = box.HalfExtents.x - cL.x;
            float dyMin = cL.y + box.HalfExtents.y, dyMax = box.HalfExtents.y - cL.y;
            float dzMin = cL.z + box.HalfExtents.z, dzMax = box.HalfExtents.z - cL.z;

            float minDist = dxMin; glm::vec3 nLocal = {-1,0,0};
            if (dxMax < minDist) { minDist = dxMax; nLocal = {+1,0,0}; }
            if (dyMin < minDist) { minDist = dyMin; nLocal = {0,-1,0}; }
            if (dyMax < minDist) { minDist = dyMax; nLocal = {0,+1,0}; }
            if (dzMin < minDist) { minDist = dzMin; nLocal = {0,0,-1}; }
            if (dzMax < minDist) { minDist = dzMax; nLocal = {0,0,+1}; }

            glm::vec3 nWorld = box.Axes[0]*nLocal.x + box.Axes[1]*nLocal.y + box.Axes[2]*nLocal.z;
            m.HIT    = true;
            m.Normal = SafeNormalize(nWorld);            
            m.Depth  = r + minDist;
            return m;
        }
    };

    template<>
    struct CollisionDetector<Sphere, Capsule>
    {
        [[nodiscard]] static bool Intersect(const Sphere& a, const Capsule& b)
        {
            return CollisionDetector<Capsule, Sphere>::Intersect(b, a);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Sphere& a, const Capsule& b)
        {
            CollisionResult m = CollisionDetector<Capsule, Sphere>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    template<>
    struct CollisionDetector<AABB, Capsule>
    {
        [[nodiscard]] static bool Intersect(const AABB& a, const Capsule& b)
        {
            return CollisionDetector<Capsule, AABB>::Intersect(b, a);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const AABB& a, const Capsule& b)
        {
            CollisionResult m = CollisionDetector<Capsule, AABB>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    template<>
    struct CollisionDetector<OBB, Capsule>
    {
        [[nodiscard]] static bool Intersect(const OBB& a, const Capsule& b)
        {
            return CollisionDetector<Capsule, OBB>::Intersect(b, a);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const OBB& a, const Capsule& b)
        {
            CollisionResult m = CollisionDetector<Capsule, OBB>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    #pragma endregion
    #pragma region CovexHull Collision

    template<>
    struct CollisionDetector<ConvexHull, ConvexHull>
    {
        [[nodiscard]] static bool Intersect(const ConvexHull& a, const ConvexHull& b)
        {
            return ConvexHull::GJK(a, b).HIT;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConvexHull& a, const ConvexHull& b)
        {
            CollisionResult c{};
            auto gjk = ConvexHull::GJK(a, b);
            if (!gjk.HIT) return c;

            auto epa = ConvexHull::EPA(a, b, gjk);
            if (!epa.HIT) return c;

            c.HIT       = true;
            c.Normal    = epa.Normal;   // A → B
            c.Depth     = epa.Depth;

            return c;
        }
    };

    template<>
    struct CollisionDetector<ConvexHull, AABB>
    {
        [[nodiscard]] static bool Intersect(const ConvexHull& a, const AABB& b)
        {
            return ConvexHull::GJK(a, b).HIT;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConvexHull& a, const AABB& b)
        {
            CollisionResult c{};
            auto gjk = ConvexHull::GJK(a, b);
            if (!gjk.HIT) return c;

            auto epa = ConvexHull::EPA(a, b, gjk);
            if (!epa.HIT) return c;

            c.HIT       = true; 
            c.Normal    = epa.Normal; 
            c.Depth     = epa.Depth; 

            return c;
        }
    };

    template<>
    struct CollisionDetector<ConvexHull, OBB>
    {
        [[nodiscard]] static bool Intersect(const ConvexHull& a, const OBB& b)
        {
            return ConvexHull::GJK(a, b).HIT;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConvexHull& a, const OBB& b)
        {
            CollisionResult c{};
            auto gjk = ConvexHull::GJK(a, b);
            if (!gjk.HIT) return c;

            auto epa = ConvexHull::EPA(a, b, gjk);
            if (!epa.HIT) return c;

            c.HIT       = true; 
            c.Normal    = epa.Normal; 
            c.Depth     = epa.Depth; 

            return c;
        }
    };

    template<>
    struct CollisionDetector<ConvexHull, Sphere>
    {
        [[nodiscard]] static bool Intersect(const ConvexHull& a, const Sphere& b)
        {
            return ConvexHull::GJK(a, b).HIT;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConvexHull& a, const Sphere& b)
        {
            CollisionResult c{};
            auto gjk = ConvexHull::GJK(a, b);
            if (!gjk.HIT) return c;

            auto epa = ConvexHull::EPA(a, b, gjk);
            if (!epa.HIT) return c;

            c.HIT       = true; 
            c.Normal    = epa.Normal; 
            c.Depth     = epa.Depth; 

            return c;
        }
    };

    template<>
    struct CollisionDetector<ConvexHull, Capsule>
    {
        [[nodiscard]] static bool Intersect(const ConvexHull& a, const Capsule& b)
        {
            return ConvexHull::GJK(a, b).HIT;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConvexHull& a, const Capsule& b)
        {
            CollisionResult m{};

            auto gjk = ConvexHull::GJK(a, b);
            if (!gjk.HIT) return m;

            auto epa = ConvexHull::EPA(a, b, gjk);
            if (!epa.HIT) return m;

            m.HIT    = true;
            m.Normal = epa.Normal;   // by Minkowski convention this points A→B (Hull → Capsule)
            m.Depth  = epa.Depth;
            return m;
        }
    };

    template<>
    struct CollisionDetector<AABB, ConvexHull>
    {
        [[nodiscard]] static bool Intersect(const AABB& a, const ConvexHull& b)
        {
            return CollisionDetector<ConvexHull, AABB>::Intersect(b, a);
        }
        [[nodiscard]] static CollisionResult IntersectCollision(const AABB& a, const ConvexHull& b)
        {
            CollisionResult m = CollisionDetector<ConvexHull, AABB>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    template<>
    struct CollisionDetector<OBB, ConvexHull>
    {
        [[nodiscard]] static bool Intersect(const OBB& a, const ConvexHull& b)
        {
            return CollisionDetector<ConvexHull, OBB>::Intersect(b, a);
        }
        [[nodiscard]] static CollisionResult IntersectCollision(const OBB& a, const ConvexHull& b)
        {
            CollisionResult m = CollisionDetector<ConvexHull, OBB>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    template<>
    struct CollisionDetector<Sphere, ConvexHull>
    {
        [[nodiscard]] static bool Intersect(const Sphere& a, const ConvexHull& b)
        {
            return CollisionDetector<ConvexHull, Sphere>::Intersect(b, a);
        }
        [[nodiscard]] static CollisionResult IntersectCollision(const Sphere& a, const ConvexHull& b)
        {
            CollisionResult m = CollisionDetector<ConvexHull, Sphere>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    template<>
    struct CollisionDetector<Capsule, ConvexHull>
    {
        [[nodiscard]] static bool Intersect(const Capsule& a, const ConvexHull& b)
        {
            return CollisionDetector<ConvexHull, Capsule>::Intersect(b, a);
        }
        [[nodiscard]] static CollisionResult IntersectCollision(const Capsule& a, const ConvexHull& b)
        {
            CollisionResult m = CollisionDetector<ConvexHull, Capsule>::IntersectCollision(b, a);
            if (m.HIT) m.Normal = -m.Normal;
            return m;
        }
    };

    #pragma endregion
    #pragma region ConcaveMsh Collision

    template<>
    struct CollisionDetector<ConcaveMesh, AABB>
    {
        [[nodiscard]] static bool Intersect(const ConcaveMesh& a, const AABB& b)
        {
            return a.Intersect<AABB>(b);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConcaveMesh& a, const AABB& b)
        {
            return a.IntersectCollision<AABB>(b);
        }
    };

    template<>
    struct CollisionDetector<ConcaveMesh, OBB>
    {
        [[nodiscard]] static bool Intersect(const ConcaveMesh& a, const OBB& b)
        {
            return a.Intersect<OBB>(b);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConcaveMesh& a, const OBB& b)
        {
            return a.IntersectCollision<OBB>(b);
        }
    };

    template<>
    struct CollisionDetector<ConcaveMesh, Sphere>
    {
        [[nodiscard]] static bool Intersect(const ConcaveMesh& a, const Sphere& b)
        {
            return a.Intersect<Sphere>(b);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConcaveMesh& a, const Sphere& b)
        {
            return a.IntersectCollision<Sphere>(b);
        }
    };

    template<>
    struct CollisionDetector<ConcaveMesh, Capsule>
    {
        [[nodiscard]] static bool Intersect(const ConcaveMesh& a, const Capsule& b)
        {
            return a.Intersect<Capsule>(b);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConcaveMesh& a, const Capsule& b)
        {
            return a.IntersectCollision<Capsule>(b);
        }
    };

    template<>
    struct CollisionDetector<ConcaveMesh, ConvexHull>
    {
        [[nodiscard]] static bool Intersect(const ConcaveMesh& a, const ConvexHull& b)
        {
            return a.Intersect<ConvexHull>(b);
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConcaveMesh& a, const ConvexHull& b)
        {
            return a.IntersectCollision<ConvexHull>(b);
        }
    };

    template<>
    struct CollisionDetector<AABB, ConcaveMesh>
    {
        [[nodiscard]] static bool Intersect(const AABB& a, const ConcaveMesh& b)
        { 
            return CollisionDetector<ConcaveMesh, AABB>::Intersect(b, a); 
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const AABB& a, const ConcaveMesh& b)
        { 
            auto m = CollisionDetector<ConcaveMesh, AABB>::IntersectCollision(b, a); 
            if (m.HIT) m.Normal = -m.Normal; 
            return m; 
        }
    };

    template<>
    struct CollisionDetector<OBB, ConcaveMesh>
    {
        [[nodiscard]] static bool Intersect(const OBB& a, const ConcaveMesh& b)
        { 
            return CollisionDetector<ConcaveMesh, OBB>::Intersect(b, a); 
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const OBB& a, const ConcaveMesh& b)
        { 
            auto m = CollisionDetector<ConcaveMesh, OBB>::IntersectCollision(b, a); 
            if (m.HIT) m.Normal = -m.Normal; 
            return m; 
        }
    };

    template<>
    struct CollisionDetector<Sphere, ConcaveMesh>
    {
        [[nodiscard]] static bool Intersect(const Sphere& a, const ConcaveMesh& b)
        { 
            return CollisionDetector<ConcaveMesh, Sphere>::Intersect(b, a); 
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Sphere& a, const ConcaveMesh& b)
        { 
            auto m = CollisionDetector<ConcaveMesh, Sphere>::IntersectCollision(b, a); 
            if (m.HIT) m.Normal = -m.Normal; return m; 
        }
    };

    template<>
    struct CollisionDetector<Capsule, ConcaveMesh>
    {
        [[nodiscard]] static bool Intersect(const Capsule& a, const ConcaveMesh& b)
        { 
            return CollisionDetector<ConcaveMesh, Capsule>::Intersect(b, a); 
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Capsule& a, const ConcaveMesh& b)
        { 
            auto m = CollisionDetector<ConcaveMesh, Capsule>::IntersectCollision(b, a); 
            if (m.HIT) m.Normal = -m.Normal; return m; 
        }
    };

    template<>
    struct CollisionDetector<ConvexHull, ConcaveMesh>
    {
        [[nodiscard]] static bool Intersect(const ConvexHull& a, const ConcaveMesh& b)
        { 
            return CollisionDetector<ConcaveMesh, ConvexHull>::Intersect(b, a); 
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const ConvexHull& a, const ConcaveMesh& b)
        { 
            auto m = CollisionDetector<ConcaveMesh, ConvexHull>::IntersectCollision(b, a); 
            if (m.HIT) m.Normal = -m.Normal; return m; 
        }
    };

    #pragma endregion

    struct Collider
    {
        ColliderType Type{ColliderType::ConvexHull};

        AABB           AABBCollider{};
        OBB            OBBCollider{};
        Sphere         SphereCollider{};
        Capsule        CapsuleCollider{};
        ConvexHull     CovxHullCollider{};
        ConcaveMesh    ConcvMeshCollider{};

        template<ColliderExpected T>
        static constexpr T Create()
        {
            return T{};
        }

        template<ColliderExpected T, class... Args>
        requires std::constructible_from<T, Args...>
        static constexpr T Create(Args&&... args)
        {
            return T{std::forward<Args>(args)...};
        }
        

        [[nodiscard]] static bool Intersect(const Collider& a, const Collider& b) 
        {
            switch (a.Type) 
            {
                case ColliderType::AABB:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<AABB, AABB>::Intersect(a.AABBCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<AABB, OBB>::Intersect(a.AABBCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<AABB, Sphere>::Intersect(a.AABBCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<AABB, Capsule>::Intersect(a.AABBCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<AABB, ConvexHull>::Intersect(a.AABBCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<AABB, ConcaveMesh>::Intersect(a.AABBCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
                case ColliderType::OBB:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<OBB, AABB>::Intersect(a.OBBCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<OBB, OBB>::Intersect(a.OBBCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<OBB, Sphere>::Intersect(a.OBBCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<OBB, Capsule>::Intersect(a.OBBCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<OBB, ConvexHull>::Intersect(a.OBBCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<OBB, ConcaveMesh>::Intersect(a.OBBCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
                case ColliderType::Sphere:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<Sphere, AABB>::Intersect(a.SphereCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<Sphere, OBB>::Intersect(a.SphereCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<Sphere, Sphere>::Intersect(a.SphereCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<Sphere, Capsule>::Intersect(a.SphereCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<Sphere, ConvexHull>::Intersect(a.SphereCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<Sphere, ConcaveMesh>::Intersect(a.SphereCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
                case ColliderType::Capsule:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<Capsule, AABB>::Intersect(a.CapsuleCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<Capsule, OBB>::Intersect(a.CapsuleCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<Capsule, Sphere>::Intersect(a.CapsuleCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<Capsule, Capsule>::Intersect(a.CapsuleCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<Capsule, ConvexHull>::Intersect(a.CapsuleCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<Capsule, ConcaveMesh>::Intersect(a.CapsuleCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
                case ColliderType::ConvexHull:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<ConvexHull, AABB>::Intersect(a.CovxHullCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<ConvexHull, OBB>::Intersect(a.CovxHullCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<ConvexHull, Sphere>::Intersect(a.CovxHullCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<ConvexHull, Capsule>::Intersect(a.CovxHullCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<ConvexHull, ConvexHull>::Intersect(a.CovxHullCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<ConvexHull, ConcaveMesh>::Intersect(a.CovxHullCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }

                case ColliderType::ConcaveMesh:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<ConcaveMesh, AABB>::Intersect(a.ConcvMeshCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<ConcaveMesh, OBB>::Intersect(a.ConcvMeshCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<ConcaveMesh, Sphere>::Intersect(a.ConcvMeshCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<ConcaveMesh, Capsule>::Intersect(a.ConcvMeshCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<ConcaveMesh, ConvexHull>::Intersect(a.ConcvMeshCollider, b.CovxHullCollider);
                        //case ColliderType::ConcaveMesh:     return CollisionDetector<ConcaveMesh, ConcaveMesh>::Intersect(a.ConcvMeshCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
            }

            return false;
        }

        [[nodiscard]] static CollisionResult IntersectCollision(const Collider& a, const Collider& b) 
        {
            switch (a.Type) 
            {
                case ColliderType::AABB:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<AABB, AABB>::IntersectCollision(a.AABBCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<AABB, OBB>::IntersectCollision(a.AABBCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<AABB, Sphere>::IntersectCollision(a.AABBCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<AABB, Capsule>::IntersectCollision(a.AABBCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<AABB, ConvexHull>::IntersectCollision(a.AABBCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<AABB, ConcaveMesh>::IntersectCollision(a.AABBCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
                case ColliderType::OBB:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<OBB, AABB>::IntersectCollision(a.OBBCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<OBB, OBB>::IntersectCollision(a.OBBCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<OBB, Sphere>::IntersectCollision(a.OBBCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<OBB, Capsule>::IntersectCollision(a.OBBCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<OBB, ConvexHull>::IntersectCollision(a.OBBCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<OBB, ConcaveMesh>::IntersectCollision(a.OBBCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
                case ColliderType::Sphere:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<Sphere, AABB>::IntersectCollision(a.SphereCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<Sphere, OBB>::IntersectCollision(a.SphereCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<Sphere, Sphere>::IntersectCollision(a.SphereCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<Sphere, Capsule>::IntersectCollision(a.SphereCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<Sphere, ConvexHull>::IntersectCollision(a.SphereCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<Sphere, ConcaveMesh>::IntersectCollision(a.SphereCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
                case ColliderType::Capsule:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<Capsule, AABB>::IntersectCollision(a.CapsuleCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<Capsule, OBB>::IntersectCollision(a.CapsuleCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<Capsule, Sphere>::IntersectCollision(a.CapsuleCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<Capsule, Capsule>::IntersectCollision(a.CapsuleCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<Capsule, ConvexHull>::IntersectCollision(a.CapsuleCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<Capsule, ConcaveMesh>::IntersectCollision(a.CapsuleCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
                case ColliderType::ConvexHull:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<ConvexHull, AABB>::IntersectCollision(a.CovxHullCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<ConvexHull, OBB>::IntersectCollision(a.CovxHullCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<ConvexHull, Sphere>::IntersectCollision(a.CovxHullCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<ConvexHull, Capsule>::IntersectCollision(a.CovxHullCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<ConvexHull, ConvexHull>::IntersectCollision(a.CovxHullCollider, b.CovxHullCollider);
                        case ColliderType::ConcaveMesh:     return CollisionDetector<ConvexHull, ConcaveMesh>::IntersectCollision(a.CovxHullCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }

                case ColliderType::ConcaveMesh:
                {
                    switch (b.Type) 
                    {
                        case ColliderType::AABB:            return CollisionDetector<ConcaveMesh, AABB>::IntersectCollision(a.ConcvMeshCollider, b.AABBCollider);
                        case ColliderType::OBB:             return CollisionDetector<ConcaveMesh, OBB>::IntersectCollision(a.ConcvMeshCollider, b.OBBCollider);
                        case ColliderType::Sphere:          return CollisionDetector<ConcaveMesh, Sphere>::IntersectCollision(a.ConcvMeshCollider, b.SphereCollider);
                        case ColliderType::Capsule:         return CollisionDetector<ConcaveMesh, Capsule>::IntersectCollision(a.ConcvMeshCollider, b.CapsuleCollider);
                        case ColliderType::ConvexHull:      return CollisionDetector<ConcaveMesh, ConvexHull>::IntersectCollision(a.ConcvMeshCollider, b.CovxHullCollider);
                        //case ColliderType::ConcaveMesh:     return CollisionDetector<ConcaveMesh, ConcaveMesh>::IntersectCollision(a.ConcvMeshCollider, b.ConcvMeshCollider); 
                    }

                    break;
                }
            }

            return {};
        }
    };
}

