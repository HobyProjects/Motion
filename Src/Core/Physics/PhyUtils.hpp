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
        out.Translation             = glm::vec3(pos.x, pos.y, pos.z);
        out.Rotation                = glm::quat(q.w, q.x, q.y, q.z);
    }


    inline rp3d::BodyType ToRp3dBodyType(BodyType t)
    {
        switch (t) 
        { 
            case BodyType::Static: return rp3d::BodyType::STATIC; 
            case BodyType::Dynamic: return rp3d::BodyType::DYNAMIC; 
        }

        return rp3d::BodyType::STATIC;
    }

    inline float sqr(float x) 
    { 
        return x * x; 
    }

    inline float length2(const glm::vec3& v) 
    { 
        return glm::dot(v, v); 
    }

    inline bool nearlyEqual(const glm::vec3& a, const glm::vec3& b, float eps) 
    {
        return (fabsf(a.x - b.x) <= eps) && (fabsf(a.y - b.y) <= eps) && (fabsf(a.z - b.z) <= eps);
    }

    inline float triArea2(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) 
    {
        return length2(glm::cross(b - a, c - a)); // squared area * 4
    }

    inline bool isFiniteVec3(const glm::vec3& v) 
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }

    struct QKey 
    {
        int x, y, z;
        bool operator==(const QKey& o) const { return x == o.x && y == o.y && z == o.z; }
    };

    struct QKeyHash 
    {
        size_t operator()(const QKey& k) const 
        {
            size_t h = 1469598103934665603ull;
            auto mix = [&](int v) 
            {
                size_t u = static_cast<size_t>(v);
                h ^= u + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            };

            mix(k.x); mix(k.y); mix(k.z);
            return h;
        }
    };

    struct WeldResult 
    {
        std::vector<glm::vec3> vertices;  
        std::vector<uint32_t>  indices;   
    };

    inline WeldResult weldAndClean(const std::vector<glm::vec3>& inVerts, const std::vector<uint32_t>&  inIndices, float epsWeld, float epsArea) 
    {
        MOTION_ASSERT((inIndices.size() % 3) == 0, "Triangle index list must be a multiple of 3");
        std::unordered_map<QKey, uint32_t, QKeyHash> grid;
        grid.reserve(inVerts.size() * 2 + 1);

        std::vector<glm::vec3> unique;
        unique.reserve(inVerts.size());

        std::vector<uint32_t> remap(inVerts.size(), UINT32_MAX);

        auto quantize = [&](const glm::vec3& v) -> QKey 
        {
            return QKey{
                (int)std::floor(v.x / epsWeld),
                (int)std::floor(v.y / epsWeld),
                (int)std::floor(v.z / epsWeld)
            };
        };

        for (uint32_t i = 0; i < (uint32_t)inVerts.size(); ++i) 
        {
            const glm::vec3 v = inVerts[i];
            if (!isFiniteVec3(v)) continue; 
            QKey k = quantize(v);

            uint32_t idx;
            auto it = grid.find(k);
            if (it == grid.end()) 
            {
                idx = (uint32_t)unique.size();
                unique.push_back(v);
                grid.emplace(k, idx);
            } 
            else 
            {
                idx = it->second;
                if (!nearlyEqual(unique[idx], v, epsWeld)) 
                {
                    static const int off[6][3] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
                    bool snapped = false;
                    for (auto& d : off) 
                    {
                        QKey k2{k.x + d[0], k.y + d[1], k.z + d[2]};
                        auto it2 = grid.find(k2);
                        if (it2 != grid.end() && nearlyEqual(unique[it2->second], v, epsWeld)) 
                        {
                            idx = it2->second;
                            snapped = true;
                            break;
                        }
                    }
                    if (!snapped) 
                    {
                        idx = (uint32_t)unique.size();
                        unique.push_back(v);
                        grid.emplace(quantize(v + glm::vec3(1e-9f)), idx); 
                    }
                }
            }
            remap[i] = idx;
        }

        std::vector<uint32_t> triOut;
        triOut.reserve(inIndices.size());

        auto pushTriIfValid = [&](uint32_t a, uint32_t b, uint32_t c) 
        {
            if (a == UINT32_MAX || b == UINT32_MAX || c == UINT32_MAX) return;
            if (a == b || b == c || c == a) return; 
            const glm::vec3& A = unique[a];
            const glm::vec3& B = unique[b];
            const glm::vec3& C = unique[c];
            if (triArea2(A, B, C) <= epsArea) return;
            triOut.push_back(a); triOut.push_back(b); triOut.push_back(c);
        };

        for (size_t i = 0; i < inIndices.size(); i += 3) 
        {
            const uint32_t ia = inIndices[i + 0];
            const uint32_t ib = inIndices[i + 1];
            const uint32_t ic = inIndices[i + 2];
            MOTION_ASSERT(ia < remap.size() && ib < remap.size() && ic < remap.size(), "Index out of range");
            pushTriIfValid(remap[ia], remap[ib], remap[ic]);
        }

        std::vector<uint8_t> used(unique.size(), 0);
        for (uint32_t idx : triOut) used[idx] = 1;

        std::vector<uint32_t> compactRemap(unique.size(), UINT32_MAX);
        std::vector<glm::vec3> compact;
        compact.reserve(unique.size());
        for (uint32_t i = 0; i < (uint32_t)unique.size(); ++i) 
        {
            if (used[i]) 
            {
                compactRemap[i] = (uint32_t)compact.size();
                compact.push_back(unique[i]);
            }
        }
        for (uint32_t& idx : triOut) idx = compactRemap[idx];

        WeldResult out;
        out.vertices = std::move(compact);
        out.indices  = std::move(triOut);
        return out;
    }

    inline bool hasNonCoplanarSeed(const std::vector<glm::vec3>& pts, float eps) 
    {
        if (pts.size() < 4) return false;
        uint32_t i0 = 0, i1 = UINT32_MAX, i2 = UINT32_MAX;
        for (uint32_t i = 1; i < (uint32_t)pts.size(); ++i) 
        {
            if (length2(pts[i] - pts[i0]) > sqr(eps)) { i1 = i; break; }
        }

        if (i1 == UINT32_MAX) return false;
        for (uint32_t i = i1 + 1; i < (uint32_t)pts.size(); ++i) 
        {
            if (triArea2(pts[i0], pts[i1], pts[i]) > sqr(eps)) { i2 = i; break; }
        }

        if (i2 == UINT32_MAX) return false;
        glm::vec3 n = glm::normalize(glm::cross(pts[i1] - pts[i0], pts[i2] - pts[i0]));
        for (uint32_t i = i2 + 1; i < (uint32_t)pts.size(); ++i) 
        {
            float d = glm::dot(n, pts[i] - pts[i0]);
            if (fabsf(d) > eps) return true;
        }

        return false;
    }

    struct CookScale 
    {
        glm::vec3 toCook   = glm::vec3(1.0f);
        glm::vec3 toModel  = glm::vec3(1.0f);
    };

    inline CookScale chooseCookScale(const std::vector<glm::vec3>& v) 
    {
        glm::vec3 mn( FLT_MAX), mx(-FLT_MAX);
        for (auto& p : v) 
        {
            mn = glm::min(mn, p);
            mx = glm::max(mx, p);
        }

        glm::vec3 ext = glm::max(mx - mn, glm::vec3(1e-6f));
        float maxAxis = glm::compMax(ext);
        float s = (maxAxis < 1e-2f) ? (1.0f / std::max(maxAxis, 1e-6f)) :
                  (maxAxis > 1e+3f) ? (1.0f / maxAxis) : 1.0f;

        CookScale cs;
        cs.toCook  = glm::vec3(s);
        cs.toModel = glm::vec3(1.0f / s);
        return cs;
    }

    inline bool nearlyEqual(float a, float b, float eps = 1e-4f) 
    {
        return std::fabs(a - b) <= eps * std::max(1.0f, std::max(std::fabs(a), std::fabs(b)));
    }

    inline bool nearlyEqualVec3(const glm::vec3& a, const glm::vec3& b, float eps = 1e-4f) 
    {
        return nearlyEqual(a.x,b.x,eps) && nearlyEqual(a.y,b.y,eps) && nearlyEqual(a.z,b.z,eps);
    }


}