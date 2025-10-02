#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <reactphysics3d/reactphysics3d.h>

#include "Renderer.hpp"
#include "Components.hpp"
#include "Entity.hpp"

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

    inline float Distance(const glm::vec3& a, const glm::vec3& b) 
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
        for (auto& p : v) 
        {
            b.minv = glm::min(b.minv, p);
            b.maxv = glm::max(b.maxv, p);
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
                    float d2 = Distance(points[i], pNew);
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
            std::uint32_t matchIdx = std::numeric_limits<std::uint32_t>::max();
            for (int dz = -1; dz <= 1 && matchIdx == std::numeric_limits<std::uint32_t>::max(); ++dz) 
            {
                for (int dy = -1; dy <= 1 && matchIdx == std::numeric_limits<std::uint32_t>::max(); ++dy) 
                {
                    for (int dx = -1; dx <= 1 && matchIdx == std::numeric_limits<std::uint32_t>::max(); ++dx) 
                    {
                        CellKey ncell{ c.x + dx, c.y + dy, c.z + dz };
                        auto it = grid.find(ncell);
                        if (it == grid.end()) continue;

                        const auto& candidates = it->second;
                        for (std::uint32_t u : candidates) {
                            if (Distance(p, uniqueVerts[u]) <= eps2) 
                            {
                                matchIdx = u; 
                                break;
                            }
                        }
                    }
                }
            }

            if (matchIdx == std::numeric_limits<std::uint32_t>::max()) 
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

    inline void ComputeConvexHull(rp3d::PhysicsCommon& physicsCommon, const std::vector<glm::vec3>& inputPoints, HullBuildResult& res, const rp3d::Vector3 scaling = rp3d::Vector3(1,1,1), float degeneracyEps = 1e-6f)
    {
        if (inputPoints.size() < 4) 
        {
            res.log.emplace_back("Not enough unique points to build a 3D hull (need >= 4).");
            return;
        }

        auto pts = inputPoints;
        res.localOffset = Centroid(pts);
        for (auto& p : pts) p -= res.localOffset;

        switch (EstimateRank(pts, degeneracyEps)) 
        {
            case PointSetRank::Degenerate:
                res.log.emplace_back("Point set is degenerate (all same / too close).");
                return;
            case PointSetRank::Line:
                res.log.emplace_back("Point set is nearly colinear; cannot build a 3D convex hull.");
                return;
            case PointSetRank::Plane:
                res.log.emplace_back("Point set is nearly coplanar; 3D hull may fail or be paper-thin.");
                return;
            case PointSetRank::Full3D:
                break;
        }

        std::vector<float> flat; flat.reserve(pts.size()*3);
        for (auto& p : pts) { flat.push_back(p.x); flat.push_back(p.y); flat.push_back(p.z); }

        rp3d::VertexArray vtxArray(
            flat.data(),
            sizeof(float)*3,
            static_cast<uint32_t>(pts.size()),
            rp3d::VertexArray::DataType::VERTEX_FLOAT_TYPE
        );

        std::vector<rp3d::Message> rpmsg;
        rp3d::ConvexMesh* convex = physicsCommon.createConvexMesh(vtxArray, rpmsg);

        for (const auto& m : rpmsg) 
        {
            std::string t = (m.type == rp3d::Message::Type::Information) ? "Info" :
                            (m.type == rp3d::Message::Type::Warning)     ? "Warn" : "Error";

            res.log.push_back(std::format("{}: {}", t, m.text.empty() ? m.text : ""));
        }

        if (!convex) 
        {
            res.log.emplace_back("rp3d failed to create ConvexMesh (see messages above).");
            return;
        }

        rp3d::ConvexMeshShape* shape = physicsCommon.createConvexMeshShape(convex, scaling);
        if (!shape) 
        {
            res.log.emplace_back("Failed to create ConvexMeshShape.");
            physicsCommon.destroyConvexMesh(convex);
            return;
        }

        res.mesh  = convex;
        res.shape = shape;
    }
}