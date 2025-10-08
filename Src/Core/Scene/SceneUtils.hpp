#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>
#include <algorithm>
#include <limits>
#include <cmath>
#include <format>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <reactphysics3d/reactphysics3d.h>


#include "Asserts.hpp"
#include "Components.hpp"

namespace Motion
{
    inline rp3d::Vector3 ToVec3(const glm::vec3& v) 
    { 
        return rp3d::Vector3(v.x, v.y, v.z); 
    }

    inline glm::vec3 ToVec3(const rp3d::Vector3& v) 
    { 
        return glm::vec3(v.x, v.y, v.z);
    }

    inline rp3d::Quaternion ToQuat(const glm::quat& q) 
    { 
        return rp3d::Quaternion(q.x, q.y, q.z, q.w); 
    }

    inline glm::quat ToQuat(const rp3d::Quaternion& q) 
    { 
        return glm::quat(q.w, q.x, q.y, q.z);
    }

    inline rp3d::Transform Transform(const TransformComponent& t)
    {
        rp3d::Vector3 position{t.Translation.x, t.Translation.y, t.Translation.z};
        rp3d::Quaternion orientation{t.Rotation.x, t.Rotation.y, t.Rotation.z, t.Rotation.w};
        return rp3d::Transform(position, orientation);
    }

    inline rp3d::Transform Transform(const glm::vec3& position, const glm::quat& orientation)
    {
        rp3d::Vector3 p{position.x, position.y, position.z};
        rp3d::Quaternion o{orientation.x, orientation.y, orientation.z, orientation.w};
        return rp3d::Transform(p, o);
    }

    inline void Transform(const rp3d::Transform& tr, TransformComponent& out) 
    {
        const rp3d::Quaternion q    = tr.getOrientation();
        const rp3d::Vector3 pos     = tr.getPosition();
        out.Translation             = ToVec3(pos);
        out.Rotation                = ToQuat(q);
    }


    inline rp3d::BodyType GetBodyType(BodyType t)
    {
        switch (t) 
        { 
            case BodyType::Static: return rp3d::BodyType::STATIC; 
            case BodyType::Dynamic: return rp3d::BodyType::DYNAMIC; 
        }

        return rp3d::BodyType::STATIC;
    }

    inline float SquaredDistance(const glm::vec3& a, const glm::vec3& b) 
    {
        glm::vec3 d = a - b;
        return glm::dot(d, d);
    }

    inline bool IsFinite(const glm::vec3& p) 
    {
        return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
    }

    static inline float Len2(const glm::vec3& v) { return glm::dot(v, v); }

    inline glm::vec3 Centroid(const std::vector<glm::vec3>& v) 
    {
        glm::dvec3 acc(0.0);
        for (auto& p : v) acc += glm::dvec3(p);
        return v.empty() ? glm::vec3(0) : glm::vec3(acc / (double)v.size());
    }

    struct AABB 
    {
        glm::vec3 minv, maxv;
    };

    struct CellKey 
    {
        int64_t x, y, z;
        bool operator==(const CellKey& o) const { return x==o.x && y==o.y && z==o.z; }
    };

    struct CellKeyHash 
    {
        size_t operator()(const CellKey& k) const noexcept 
        {
            auto h = static_cast<uint64_t>(1469598103934665603ull);
            auto mix = [&](uint64_t v){ h ^= v; h *= 1099511628211ull; };
            mix(static_cast<uint64_t>(k.x) + 0x9e3779b97f4a7c15ull);
            mix(static_cast<uint64_t>(k.y) + 0x85ebca6b);
            mix(static_cast<uint64_t>(k.z) + 0xc2b2ae35);
            return static_cast<size_t>(h);
        }
    };

    enum class PointSetRank { Degenerate, Line, Plane, Full3D };

    struct HullBuildResult 
    {
        rp3d::ConvexMesh* mesh = nullptr; 
        rp3d::ConvexMeshShape* shape = nullptr; 
        glm::vec3 localOffset = glm::vec3(0.0f);
        std::vector<std::string> log;              
        bool ok() const { return mesh && shape; }
    };

    struct CompoundHullResult
    {
        std::vector<rp3d::ConvexMesh*> meshes;
        std::vector<rp3d::ConvexMeshShape*> shapes;
        std::vector<glm::vec3> localOffsets;
        std::vector<std::string> log;
        bool ok() const { return !shapes.empty(); }
    };

    static inline CellKey ToCell(const glm::vec3& p, float eps) 
    {
        return CellKey
        {
            static_cast<int64_t>(std::floor(p.x / eps)),
            static_cast<int64_t>(std::floor(p.y / eps)),
            static_cast<int64_t>(std::floor(p.z / eps))
        };
    }

    inline AABB ComputeAABB(const std::vector<glm::vec3>& v) 
    {
        AABB b;
        if (v.empty()) { b.minv = b.maxv = glm::vec3(0); return b; }
        b.minv = b.maxv = v[0];
        for (const auto& p : v) 
        {
            b.minv.x = (b.minv.x < p.x) ? b.minv.x : p.x;
            b.minv.y = (b.minv.y < p.y) ? b.minv.y : p.y;
            b.minv.z = (b.minv.z < p.z) ? b.minv.z : p.z;

            b.maxv.x = (b.maxv.x > p.x) ? b.maxv.x : p.x;
            b.maxv.y = (b.maxv.y > p.y) ? b.maxv.y : p.y;
            b.maxv.z = (b.maxv.z > p.z) ? b.maxv.z : p.z;
        }
        
        return b;
    }

    inline PointSetRank EstimateRank(const std::vector<glm::vec3>& v, float eps = 1e-6f) 
    {
        if (v.size() < 4) return PointSetRank::Degenerate;

        AABB b          = ComputeAABB(v);
        glm::vec3 ext   = b.maxv - b.minv;
        float e[3]      = { std::abs(ext.x), std::abs(ext.y), std::abs(ext.z) };

        std::sort(e, e+3);

        if (e[2] < eps) return PointSetRank::Degenerate; 
        if (e[1] < eps) return PointSetRank::Line;       
        if (e[0] < eps) return PointSetRank::Plane; 

        return PointSetRank::Full3D;
    }

    inline void Simplify(std::vector<glm::vec3>& points, std::uint32_t maxPoints) 
    {
        if (points.size() <= maxPoints || maxPoints == 0) return;
        if (points.empty()) return;

        const std::size_t N = points.size();
        std::size_t minX = 0, maxX = 0, minY = 0, maxY = 0, minZ = 0, maxZ = 0;
        for (std::size_t i = 1; i < N; ++i) 
        {
            if (points[i].x < points[minX].x) minX = i;
            if (points[i].x > points[maxX].x) maxX = i;
            if (points[i].y < points[minY].y) minY = i;
            if (points[i].y > points[maxY].y) maxY = i;
            if (points[i].z < points[minZ].z) minZ = i;
            if (points[i].z > points[maxZ].z) maxZ = i;
        }

        std::vector<std::size_t> chosenIdx;
        chosenIdx.reserve(std::min<std::size_t>(maxPoints, 6));
        auto pushUnique = [&](std::size_t idx) 
        {
            if (std::find(chosenIdx.begin(), chosenIdx.end(), idx) == chosenIdx.end())
                chosenIdx.push_back(idx);
        };

        pushUnique(minX); pushUnique(maxX);
        pushUnique(minY); pushUnique(maxY);
        pushUnique(minZ); pushUnique(maxZ);

        if (chosenIdx.size() >= maxPoints) 
        {
            chosenIdx.resize(maxPoints);
        } 
        else 
        {
            std::vector<float> minDist2(N, std::numeric_limits<float>::infinity());
            auto updateMinDists = [&](std::size_t newIdx) 
            {
                const glm::vec3& pNew = points[newIdx];
                for (std::size_t i = 0; i < N; ++i) 
                {
                    float d2 = SquaredDistance(points[i], pNew);
                    if (d2 < minDist2[i]) minDist2[i] = d2;
                }
            };

            for (std::size_t idx : chosenIdx) updateMinDists(idx);

            std::vector<char> taken(N, 0);
            for (auto idx : chosenIdx) taken[idx] = 1;
            while (chosenIdx.size() < maxPoints) 
            {
                float bestD2 = -1.0f;
                std::size_t bestIdx = N; 
                for (std::size_t i = 0; i < N; ++i) 
                {
                    if (taken[i]) continue;
                    if (minDist2[i] > bestD2) 
                    {
                        bestD2 = minDist2[i];
                        bestIdx = i;
                    }
                }

                if (bestIdx == N) break; 

                taken[bestIdx] = 1;
                chosenIdx.push_back(bestIdx);
                updateMinDists(bestIdx);
            }
        }

        std::vector<glm::vec3> simplified;
        simplified.reserve(chosenIdx.size());
        for (std::size_t idx : chosenIdx) simplified.push_back(points[idx]);
        points.swap(simplified);
    }

    inline void RemoveDuplicates(std::vector<glm::vec3>& verts, float eps, std::vector<std::uint32_t>& oldToNew)
    {
        const std::size_t N = verts.size();
        oldToNew.assign(N, 0);

        if (N == 0) 
        {
            verts.clear();
            return;
        }

        const float minEps = 1e-12f;
        if (!(eps > minEps) || !std::isfinite(eps))
        {
            eps = 1e-6f;
        }
        const float eps2 = eps * eps;

        std::unordered_map<CellKey, std::vector<std::uint32_t>, CellKeyHash> grid;
        grid.reserve(N * 2);

        std::vector<glm::vec3> uniqueVerts;
        uniqueVerts.reserve(N);


        for (std::size_t i = 0; i < N; ++i) 
        {
            const glm::vec3 p = verts[i];
            if (!IsFinite(p)) 
            {
                std::uint32_t newIdx = static_cast<std::uint32_t>(uniqueVerts.size());
                uniqueVerts.push_back(p);
                oldToNew[i] = newIdx;
                continue;
            }

            const CellKey c = ToCell(p, eps);
            std::int32_t matchIdx = (std::numeric_limits<std::int32_t>::max)();
            for (int dz = -1; dz <= 1 && matchIdx == (std::numeric_limits<std::int32_t>::max)(); ++dz) 
            {
                for (int dy = -1; dy <= 1 && matchIdx == (std::numeric_limits<std::int32_t>::max)(); ++dy) 
                {
                    for (int dx = -1; dx <= 1 && matchIdx == (std::numeric_limits<std::int32_t>::max)(); ++dx) 
                    {
                        CellKey ncell{ c.x + dx, c.y + dy, c.z + dz };
                        auto it = grid.find(ncell);
                        if (it == grid.end()) continue;

                        const auto& candidates = it->second;
                        for (std::int32_t u : candidates) 
                        {
                            if (SquaredDistance(p, uniqueVerts[u]) <= eps2) 
                            {
                                matchIdx = u; 
                                break;
                            }
                        }
                    }
                }
            }

            if (matchIdx == (std::numeric_limits<std::int32_t>::max)()) 
            {
                std::uint32_t newIdx = static_cast<std::uint32_t>(uniqueVerts.size());
                uniqueVerts.push_back(p);
                oldToNew[i] = newIdx;
                grid[c].push_back(newIdx);

            } 
            else 
            {
                oldToNew[i] = matchIdx;
            }
        }

        verts.swap(uniqueVerts);
    }

    inline glm::vec3 ComputePlaneNormal(const std::vector<glm::vec3>& pts, float eps = 1e-6f)
    {
        if (pts.size() < 3) return glm::vec3(0,1,0);

        const glm::vec3 c = Centroid(pts);
        glm::vec3 base = glm::vec3(0);
        bool haveBase = false;

        for (size_t i = 0; i + 1 < pts.size(); ++i)
        {
            glm::vec3 d = pts[i] - c;
            if (glm::length2(d) > eps*eps)
            {
                base = d;
                haveBase = true;
                break;
            }
        }
        if (!haveBase) return glm::vec3(0,1,0);

        glm::vec3 n(0);
        for (size_t i = 0; i < pts.size(); ++i)
        {
            glm::vec3 a = pts[i] - c;
            if (glm::length2(a) <= eps*eps) continue;
            glm::vec3 cr = glm::cross(base, a);
            if (glm::length2(cr) > eps*eps)
            {
                n = cr;
                break;
            }
        }

        if (glm::length2(n) <= eps*eps)
        {
            for (size_t i = 0; i < pts.size(); ++i)
            {
                const glm::vec3 a = pts[i] - c;
                const glm::vec3 b = pts[(i + 1) % pts.size()] - c;
                n += glm::cross(a, b);
            }
        }

        if (glm::length2(n) <= eps*eps) n = glm::vec3(0, 1, 0);
        else n = glm::normalize(n);

        return n;
    }

    inline void PlaneBasis(const glm::vec3& n, glm::vec3& u, glm::vec3& v)
    {
        // Choose a helper vector not parallel to n
        glm::vec3 helper = (std::abs(n.x) < 0.9f) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
        u = glm::normalize(glm::cross(n, helper));
        v = glm::normalize(glm::cross(n, u));
    }

    inline float Cross2D(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    inline std::vector<glm::vec2> ConvexHull2D(std::vector<glm::vec2> pts, float eps = 1e-7f)
    {
        std::sort(pts.begin(), pts.end(), [](const glm::vec2& a, const glm::vec2& b) 
        {
            if (a.x == b.x) return a.y < b.y;
            return a.x < b.x;
        });

        std::vector<glm::vec2> unique;
        unique.reserve(pts.size());
        for (const auto& p : pts)
        {
            if (unique.empty() || glm::length2(p - unique.back()) > eps*eps)
                unique.push_back(p);
        }
        if (unique.size() <= 2) return unique;

        std::vector<glm::vec2> lower, upper;
        for (const auto& p : unique)
        {
            while (lower.size() >= 2 && Cross2D(lower[lower.size()-2], lower.back(), p) <= 0.0f)
                lower.pop_back();
            lower.push_back(p);
        }
        for (int i = (int)unique.size() - 1; i >= 0; --i)
        {
            const auto& p = unique[(size_t)i];
            while (upper.size() >= 2 && Cross2D(upper[upper.size()-2], upper.back(), p) <= 0.0f)
                upper.pop_back();
            upper.push_back(p);
        }
        lower.pop_back();
        upper.pop_back();

        std::vector<glm::vec2> hull;
        hull.reserve(lower.size() + upper.size());
        hull.insert(hull.end(), lower.begin(), lower.end());
        hull.insert(hull.end(), upper.begin(), upper.end());
        return hull;
    }

    inline void BuildThinPrismFromPlanarPoints(const std::vector<glm::vec3>& ptsCentered, std::vector<glm::vec3>& outVerts, float degeneracyEps)
    {
        outVerts.clear();
        if (ptsCentered.empty()) return;

        const glm::vec3 n = ComputePlaneNormal(ptsCentered, degeneracyEps);
        glm::vec3 u, v;
        PlaneBasis(n, u, v);

        std::vector<glm::vec2> proj;
        proj.reserve(ptsCentered.size());
        for (const auto& p : ptsCentered)
        {
            proj.emplace_back(glm::dot(p, u), glm::dot(p, v));
        }

        auto hull2 = ConvexHull2D(std::move(proj));

        AABB aabb = ComputeAABB(ptsCentered);
        const glm::vec3 ext = aabb.maxv - aabb.minv;
        const float scaleRef = std::max<float>(std::max<float>(std::abs(ext.x), std::abs(ext.y)), std::max<float>(std::abs(ext.z), 1.0f));
        float thickness = std::max<float>(1e-4f * scaleRef, degeneracyEps * 10.0f);

        const float tMin = std::max<float>(1e-5f * scaleRef, degeneracyEps * 10.0f);
        const float tMax = 1e-2f * scaleRef;
        thickness = std::clamp(thickness, tMin, tMax);

        const float halfT = 0.5f * thickness;
        for (const auto& q : hull2)
        {
            glm::vec3 base3 = q.x * u + q.y * v;
            outVerts.push_back(base3 + halfT * n); // top
        }
        for (const auto& q : hull2)
        {
            glm::vec3 base3 = q.x * u + q.y * v;
            outVerts.push_back(base3 - halfT * n); // bottom
        }

        if (!outVerts.empty())
        {
            std::vector<std::uint32_t> dummy;
            const float epsPlanar = std::max<float>(degeneracyEps, 1e-7f * scaleRef);
            RemoveDuplicates(outVerts, epsPlanar, dummy);
        }
    }






    inline void ComputeConvexHull(rp3d::PhysicsCommon* physicsCommon, const std::vector<glm::vec3>& inputPoints, HullBuildResult& res, const rp3d::Vector3 scaling = rp3d::Vector3{1,1,1}, float degeneracyEps = 1e-6f)
    {
        if (inputPoints.size() < 4) 
        {
            res.log.emplace_back("Not enough unique points to build a 3D hull (need >= 4).");
            return;
        }

        std::vector<glm::vec3> pts;
        pts.reserve(inputPoints.size());
        for (const auto& p : inputPoints)
            if (IsFinite(p)) pts.push_back(p);

        if (pts.size() < 4)
        {
            res.log.emplace_back("Insufficient finite points after filtering.");
            return;
        }

        res.localOffset = Centroid(pts);
        for (auto& p : pts) p -= res.localOffset;


        {
            AABB aabb = ComputeAABB(pts);
            glm::vec3 ext = aabb.maxv - aabb.minv;
            const float scaleRef = std::max<float>(std::max<float>(std::abs(ext.x), std::abs(ext.y)), std::max<float>(std::abs(ext.z), 1.0f));
            const float epsAdapt = std::max<float>(degeneracyEps, 1e-7f * scaleRef);
            std::vector<std::uint32_t> tmpMap;
            RemoveDuplicates(pts, epsAdapt, tmpMap);
        }

        if (pts.size() < 4)
        {
            res.log.emplace_back("Not enough points after deduplication.");
            return;
        }

        bool usedPlanarPath = false;
        std::vector<glm::vec3> planarExtruded;

        AABB aabbNow = ComputeAABB(pts);
        glm::vec3 extNow = aabbNow.maxv - aabbNow.minv;
        const float scaleRefNow = std::max<float>(std::max<float>(std::abs(extNow.x), std::abs(extNow.y)), std::max<float>(std::abs(extNow.z), 1.0f));
        const float epsClass = std::max<float>(degeneracyEps, 1e-7f * scaleRefNow);

        switch (EstimateRank(pts, epsClass)) 
        {
            case PointSetRank::Degenerate:
                res.log.emplace_back("Point set is degenerate (all same / too close).");
                return;
            case PointSetRank::Line:
                res.log.emplace_back("Point set is nearly colinear; cannot build a 3D convex hull.");
                return;
            case PointSetRank::Plane:
            {
                res.log.emplace_back("Point set is nearly coplanar; building thin prism hull.");
                BuildThinPrismFromPlanarPoints(pts, planarExtruded, epsClass);
                if (planarExtruded.size() < 4)
                {
                    res.log.emplace_back("Planar extrusion produced insufficient vertices.");
                    return;
                }

                std::vector<std::uint32_t> dummy;
                RemoveDuplicates(planarExtruded, epsClass, dummy);
                if (planarExtruded.size() < 4)
                {
                    res.log.emplace_back("Planar extrusion became too small after dedup.");
                    return;
                }
                usedPlanarPath = true;
                break;
            }
            case PointSetRank::Full3D:
                break;
        }

        std::vector<float> flat;
        if (usedPlanarPath)
        {
            flat.reserve(planarExtruded.size() * 3);
            for (auto& p : planarExtruded)
            {
                flat.push_back(p.x); flat.push_back(p.y); flat.push_back(p.z);
            }
        }
        else
        {
            flat.reserve(pts.size() * 3);
            for (auto& p : pts) { flat.push_back(p.x); flat.push_back(p.y); flat.push_back(p.z); }
        }

        rp3d::VertexArray vtxArray(
            flat.data(),
            sizeof(float)*3,
            static_cast<uint32_t>( (usedPlanarPath ? planarExtruded.size() : pts.size()) ),
            rp3d::VertexArray::DataType::VERTEX_FLOAT_TYPE
        );

        std::vector<rp3d::Message> rpmsg;
        rp3d::ConvexMesh* convex = physicsCommon->createConvexMesh(vtxArray, rpmsg);

        for (const auto& m : rpmsg) 
        {
            std::string t = (m.type == rp3d::Message::Type::Information) ? "Info" :
                            (m.type == rp3d::Message::Type::Warning)     ? "Warn" : "Error";

            res.log.push_back(std::format("{}: {}", t, m.text));
        }

        if (!convex) 
        {
            res.log.emplace_back("rp3d failed to create ConvexMesh (see messages above).");
            return;
        }

        rp3d::ConvexMeshShape* shape = physicsCommon->createConvexMeshShape(convex, scaling);
        if (!shape) 
        {
            res.log.emplace_back("Failed to create ConvexMeshShape.");
            physicsCommon->destroyConvexMesh(convex);
            return;
        }

        res.mesh  = convex;
        res.shape = shape;
    }

    inline void PartitionPointsByGrid(const std::vector<glm::vec3>& points, float cellSize, std::vector<std::vector<glm::vec3>>& outClusters)
    {
        outClusters.clear();
        if (points.empty()) return;

        std::unordered_map<CellKey, std::vector<glm::vec3>, CellKeyHash> buckets;
        buckets.reserve(points.size());

        for (const auto& p : points)
        {
            CellKey k
            {
                static_cast<int64_t>(std::floor(p.x / cellSize)),
                static_cast<int64_t>(std::floor(p.y / cellSize)),
                static_cast<int64_t>(std::floor(p.z / cellSize))
            };

            buckets[k].push_back(p);
        }

        outClusters.reserve(buckets.size());
        for (auto& kv : buckets)
        {
            if (!kv.second.empty())
                outClusters.emplace_back(std::move(kv.second));
        }
    }

    inline void ComputeConvexCompound(rp3d::PhysicsCommon* physicsCommon, const std::vector<glm::vec3>& inputPoints, CompoundHullResult& out, const rp3d::Vector3 scaling = rp3d::Vector3{1,1,1}, float degeneracyEps = 1e-6f, float voxelFrac = 0.2f, std::uint32_t maxClusters = 64)
    {
        out.meshes.clear();
        out.shapes.clear();
        out.localOffsets.clear();
        out.log.clear();

        if (inputPoints.size() < 4)
        {
            out.log.emplace_back("Not enough points for compound: need >= 4.");
            return;
        }

        std::vector<glm::vec3> pts;
        pts.reserve(inputPoints.size());
        for (const auto& p : inputPoints) if (IsFinite(p)) pts.push_back(p);

        if (pts.size() < 4)
        {
            out.log.emplace_back("Insufficient finite points after filtering.");
            return;
        }

        AABB aabb = ComputeAABB(pts);
        glm::vec3 ext = aabb.maxv - aabb.minv;
        const float maxExt = std::max<float>(std::max<float>(std::abs(ext.x), std::abs(ext.y)), std::max<float>(std::abs(ext.z), 1.0f));
        const float epsAdapt = std::max<float>(degeneracyEps, 1e-7f * maxExt);

        {
            std::vector<std::uint32_t> tmp;
            RemoveDuplicates(pts, epsAdapt, tmp);
        }

        if (pts.size() < 4)
        {
            out.log.emplace_back("Not enough points after deduplication.");
            return;
        }

        float cellSize = std::max<float>(maxExt * voxelFrac, epsAdapt * 100.0f);
        std::vector<std::vector<glm::vec3>> clusters;
        PartitionPointsByGrid(pts, cellSize, clusters);

        if (clusters.empty())
        {
            out.log.emplace_back("No clusters after partitioning.");
            return;
        }

        if (clusters.size() > maxClusters)
        {
            out.log.push_back(std::format("Cluster count {} exceeds cap {}; truncating.", clusters.size(), maxClusters));
            clusters.resize(maxClusters);
        }

        for (std::size_t i = 0; i < clusters.size(); ++i)
        {
            HullBuildResult hres;
            ComputeConvexHull(physicsCommon, clusters[i], hres, scaling, degeneracyEps);

            for (auto& s : hres.log)
                out.log.push_back(std::format("[cluster {}] {}", i, s));

            if (hres.mesh && hres.shape)
            {
                out.meshes.push_back(hres.mesh);
                out.shapes.push_back(hres.shape);
                out.localOffsets.push_back(hres.localOffset);
            }
            else
            {
                out.log.push_back(std::format("[cluster {}] hull build failed; skipping.", i));
            }
        }

        if (out.shapes.empty())
        {
            out.log.emplace_back("Compound build produced no valid convex shapes.");
        }
        else
        {
            out.log.push_back(std::format("Compound build OK: {} convex parts.", out.shapes.size()));
        }
    }

    inline void CreateRigidBody(rp3d::PhysicsWorld* world, entt::registry* r, const entt::entity& e)
    {
        MOTION_ASSERT(world, "Physics world not initialized");
        if(!r->any_of<RigidBodyComponent>(e)) r->emplace_or_replace<TransformComponent>(e);   
        auto& TC    = r->get<TransformComponent>(e);
        auto& RBC   = r->get<RigidBodyComponent>(e);

        rp3d::RigidBody* RB = static_cast<rp3d::RigidBody*>(RBC.PhysicsBody);
        if (!RB) { RB = world->createRigidBody(Transform(TC)); RBC.PhysicsBody = RB; }

        RB->setType(GetBodyType(RBC.Type));
        RB->setIsDebugEnabled(true);
        RB->enableGravity(true);
        RB->setIsAllowedToSleep(true);
        RB->setIsActive(true);  

        const auto linLock  = rp3d::Vector3(
            RBC.LockX ? 0.0f : 1.0f,
            RBC.LockY ? 0.0f : 1.0f,
            RBC.LockZ ? 0.0f : 1.0f
        );

        const auto angLock  = rp3d::Vector3(
            RBC.LockRotX ? 0.0f : 1.0f,
            RBC.LockRotY ? 0.0f : 1.0f,
            RBC.LockRotZ ? 0.0f : 1.0f
        );

        RB->setLinearLockAxisFactor(linLock);
        RB->setAngularLockAxisFactor(angLock);
        RB->setAngularDamping(RBC.AngularDamping);
        RB->setLinearDamping(RBC.LinearDamping);
        RB->setUserData((void*)e);
    }

    inline void DestroyRigidBody(rp3d::PhysicsWorld* world, rp3d::PhysicsCommon* common, entt::registry* r, const entt::entity& e)
    {
        if(!r->any_of<RigidBodyComponent>(e)) return;

        auto& RBC = r->get<RigidBodyComponent>(e);
        auto* RB  = RBC.PhysicsBody;
        if(!RB) return;

        if (r->any_of<ColliderComponent>(e)) 
        {
            auto& CC = r->get<ColliderComponent>(e);

            if (CC.Collider)
            {
                RB->removeCollider(static_cast<rp3d::Collider*>(CC.Collider));
                CC.Collider = nullptr;
            }

            if (CC.Shape)
            {
                if (CC.Type == ShapeType::Box)      common->destroyBoxShape(dynamic_cast<rp3d::BoxShape*>(CC.Shape));
                if (CC.Type == ShapeType::Sphere)   common->destroySphereShape(dynamic_cast<rp3d::SphereShape*>(CC.Shape));
                if (CC.Type == ShapeType::Capsule)  common->destroyCapsuleShape(dynamic_cast<rp3d::CapsuleShape*>(CC.Shape));
                if (CC.Type == ShapeType::Convex)   common->destroyConvexMeshShape(dynamic_cast<rp3d::ConvexMeshShape*>(CC.Shape));
                if (CC.Type == ShapeType::Concave)  common->destroyConcaveMeshShape(dynamic_cast<rp3d::ConcaveMeshShape*>(CC.Shape));

                CC.Shape = nullptr;
            }
        }

        world->destroyRigidBody(RB);
        RBC.PhysicsBody = nullptr;
    }

    inline void CreateBoxCollider(rp3d::PhysicsCommon* common, entt::registry* r, const entt::entity& e) 
    {
        auto& RBC   = r->get<RigidBodyComponent>(e);
        auto& CC    = r->get<ColliderComponent>(e);
        auto& TRC   = r->get<TransformComponent>(e);

        auto* RB    = RBC.PhysicsBody;
        RB->setTransform(Transform(TRC));

        auto* shape         = common->createBoxShape(ToVec3(CC.BoxHalfExtents));
        auto* collider      = RB->addCollider(shape, rp3d::Transform::identity());
        rp3d::Material& mat = collider->getMaterial();

        collider->setUserData((void*)entt::to_integral(e));
        collider->setIsSimulationCollider(true);
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape            = shape;
        CC.Collider         = collider;
        CC.Type             = ShapeType::Box;

        if (RB->getType() == rp3d::BodyType::DYNAMIC)
            RB->updateMassPropertiesFromColliders();        
    }

    inline void CreateSphereCollider(rp3d::PhysicsCommon* common, entt::registry* rg, const entt::entity& e) 
    {
        auto& RBC   = rg->get<RigidBodyComponent>(e);
        auto& CC    = rg->get<ColliderComponent>(e);
        auto& TRC   = rg->get<TransformComponent>(e);
        auto* RB    = RBC.PhysicsBody;
        RB->setTransform(Transform(TRC));

        const glm::vec3 S       = TRC.Scale;
        const float s           = (S.x + S.y + S.z) / 3.0f;
        const float r           = std::max<float>(0.0f, CC.SphereRadius * s);
        auto* shape             = common->createSphereShape(r);
        auto* collider          = RB->addCollider(shape, rp3d::Transform::identity());
        rp3d::Material& mat     = collider->getMaterial();

        collider->setUserData((void*)entt::to_integral(e));
        collider->setIsSimulationCollider(true);
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape        = shape;
        CC.Collider     = collider;
        CC.Type         = ShapeType::Sphere;
        CC.SphereRadius = r;

        if (RB->getType() == rp3d::BodyType::DYNAMIC)
            RB->updateMassPropertiesFromColliders();
    }

    inline void CreateCapsuleCollider(rp3d::PhysicsCommon* common, entt::registry* rg, const entt::entity& e) 
    {
        auto& RBC = rg->get<RigidBodyComponent>(e);
        auto& CC  = rg->get<ColliderComponent>(e);
        auto& TRC = rg->get<TransformComponent>(e);

        auto* RB  = RBC.PhysicsBody;
        RB->setTransform(Transform(TRC));

        const glm::vec3 S           = TRC.Scale;
        const std::int32_t axis     = CC.Capsule.Axis;

        float sx = S.x, sy = S.y, sz = S.z;
        float rScale = 1.0f, hScale = 1.0f;

        if(axis == 0) { rScale = std::max<float>(sy, sz); hScale = sx; }
        else if(axis == 1) { rScale = std::max<float>(sx, sz); hScale = sy; }
        else { rScale = std::max<float>(sx, sy); hScale = sz; }

        const float r = std::max<float>(0.0f, CC.Capsule.Radius * rScale);
        const float h = std::max<float>(0.0f, CC.Capsule.Height * hScale);

        auto* shape             = common->createCapsuleShape(r, h);
        auto* collider          = RB->addCollider(shape, rp3d::Transform::identity());
        rp3d::Material& mat     = collider->getMaterial();

        collider->setUserData((void*)entt::to_integral(e));
        collider->setIsSimulationCollider(true);
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape            = shape;
        CC.Collider         = collider;
        CC.Type             = ShapeType::Capsule;
        CC.Capsule.Radius   = r;
        CC.Capsule.Height   = h;
        CC.Capsule.Axis     = axis;

        if (RB->getType() == rp3d::BodyType::DYNAMIC)
            RB->updateMassPropertiesFromColliders();
    }

    inline void CreateConvexCollider(rp3d::PhysicsCommon* common, entt::registry* rg, const entt::entity& e, const std::vector<glm::vec3>& inVertices)
    {
        const std::uint32_t simplifyTarget = 256;
        const float dedupEps = 1e-6f;
        const rp3d::Vector3 scaling(1,1,1);
        const float degeneracyEps = 1e-6f;
        
        std::vector<glm::vec3> vertices;
        vertices.reserve(inVertices.size());
        vertices.assign(inVertices.begin(), inVertices.end());

        if(simplifyTarget > 0 && vertices.size() > simplifyTarget)
        {
            Simplify(vertices, simplifyTarget);
            MOTION_CORE_INFO("Simplified convex mesh to {} vertices", vertices.size());
        }

        std::vector<std::uint32_t> oldToNew{};
        RemoveDuplicates(vertices, dedupEps, oldToNew);
        MOTION_CORE_INFO("Removed duplicates, {} vertices remain", vertices.size());

        if(vertices.size() < 4)
        {
            MOTION_CORE_WARN("Convex mesh has less than 4 unique vertices, cannot create convex collider");
            return;
        }

        HullBuildResult hull{};
        ComputeConvexHull(common, vertices, hull, scaling, degeneracyEps);

        if(!hull.ok())
        {
            for(const auto& msg : hull.log)
                MOTION_CORE_WARN(msg);

            MOTION_CORE_ERROR("Failed to compute convex hull");
            return;
        }

        auto* shape     = hull.shape;
        auto& RBC       = rg->get<RigidBodyComponent>(e);
        auto& CC        = rg->get<ColliderComponent>(e);
        auto* RB        = RBC.PhysicsBody;

        auto* collider = RB->addCollider(shape, rp3d::Transform::identity());
        collider->setUserData((void*)entt::to_integral(e));
        collider->setIsSimulationCollider(true);

        rp3d::Material& mat = collider->getMaterial();
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape            = shape;
        CC.Collider         = collider;
        CC.Type             = ShapeType::Convex;
        CC.ConvexMesh       = hull.mesh;
    }

    inline void CreateConvexColliderCompound(rp3d::PhysicsCommon* common, entt::registry* rg, const entt::entity& e, const std::vector<glm::vec3>& inVertices)
    {
        const std::uint32_t simplifyTarget = 256;
        const float dedupEps = 1e-6f;
        const rp3d::Vector3 scaling(1,1,1);
        const float degeneracyEps = 1e-6f;
        
        std::vector<glm::vec3> vertices;
        vertices.reserve(inVertices.size());
        vertices.assign(inVertices.begin(), inVertices.end());

        if(simplifyTarget > 0 && vertices.size() > simplifyTarget)
        {
            Simplify(vertices, simplifyTarget);
            MOTION_CORE_INFO("Simplified convex mesh to {} vertices", vertices.size());
        }

        std::vector<std::uint32_t> oldToNew{};
        RemoveDuplicates(vertices, dedupEps, oldToNew);
        MOTION_CORE_INFO("Removed duplicates, {} vertices remain", vertices.size());

        if(vertices.size() < 4)
        {
            MOTION_CORE_WARN("Convex mesh has less than 4 unique vertices, cannot create convex collider");
            return;
        }

        HullBuildResult hull{};
        ComputeConvexHull(common, vertices, hull, scaling, degeneracyEps);

        if(!hull.ok())
        {
            for(const auto& msg : hull.log)
                MOTION_CORE_WARN(msg);

            MOTION_CORE_ERROR("Failed to compute convex hull");
            return;
        }

        auto* shape     = hull.shape;
        auto& RBC       = rg->get<RigidBodyComponent>(e);
        auto& CC        = rg->get<ColliderComponent>(e);
        auto* RB        = RBC.PhysicsBody;

        auto* collider = RB->addCollider(shape, rp3d::Transform::identity());
        collider->setUserData((void*)e);
        collider->setIsSimulationCollider(true);

        rp3d::Material& mat = collider->getMaterial();
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape            = shape;
        CC.Collider         = collider;
        CC.Type             = ShapeType::Convex;
        CC.ConvexMesh       = hull.mesh;
    }

    struct RaycastCallBack : public rp3d::RaycastCallback 
    {
        bool hasHit = false;
        rp3d::decimal bestFraction = rp3d::decimal(1);
        rp3d::Vector3 bestPoint{0,0,0};
        rp3d::Vector3 bestNormal{0,0,0};
        const rp3d::Collider* bestCollider = nullptr;
        const rp3d::Body*     bestBody     = nullptr;

        rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info) override
        {
            if (info.hitFraction < bestFraction) 
            {
                hasHit       = true;
                bestFraction = info.hitFraction;
                bestPoint    = info.worldPoint;
                bestNormal   = info.worldNormal;
                bestCollider = info.collider;
                bestBody     = info.body;
            }

            return info.hitFraction; 
        }
    };

    struct RayHitResults
    {
        bool Hit{false};
        glm::vec3 Point{0.0f};
        glm::vec3 Normal{0.0f};
        entt::entity Entity{};
    };


    inline bool RaycastFirstHit(rp3d::PhysicsWorld* world, const glm::vec3& from, const glm::vec3& to, glm::vec3* hitPointWorld, glm::vec3* hitNormalWorld)
    {
        if (!world) return false;

        RaycastCallBack cb;
        rp3d::Ray ray(ToVec3(from), ToVec3(to));
        world->raycast(ray, &cb);

        if (!cb.hasHit) return false;

        if (hitPointWorld)  *hitPointWorld  = ToVec3(cb.bestPoint);
        if (hitNormalWorld) *hitNormalWorld = ToVec3(cb.bestNormal);
        
        return true;
    }

    inline bool RaycastFirstHit(rp3d::PhysicsWorld* world, const glm::vec3& from, const glm::vec3& to, RayHitResults& out)
    {
        if(!world) return false;

        RaycastCallBack cb;
        rp3d::Ray ray(ToVec3(from), ToVec3(to));
        world->raycast(ray, &cb);

        if(!cb.hasHit) return false;

        out.Hit = true;
        out.Point = ToVec3(cb.bestPoint);
        out.Normal = ToVec3(cb.bestNormal);

        out.Entity = entt::null;
        if (cb.bestCollider && cb.bestCollider->getUserData())
            out.Entity = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(cb.bestCollider->getUserData()));
        else if (cb.bestBody && cb.bestBody->getUserData())
            out.Entity = static_cast<entt::entity>(reinterpret_cast<std::uintptr_t>(cb.bestBody->getUserData()));

        return true;
    }











}
