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
    inline rp3d::Vector3 ToRp3dVec3(const glm::vec3& v) { return rp3d::Vector3(v.x, v.y, v.z); }
    inline glm::vec3     ToGlmVec3(const rp3d::Vector3& v) { return glm::vec3(v.x, v.y, v.z); }

    inline rp3d::Quaternion ToRp3dQuat(const glm::quat& q) { return rp3d::Quaternion(q.w, q.x, q.y, q.z); }
    inline glm::quat       ToGlmQuat(const rp3d::Quaternion& q) { return glm::quat(q.w, q.x, q.y, q.z); }

    inline rp3d::Transform ToRp3dTransform(const TransformComponent& t)
    {
        return rp3d::Transform(ToRp3dVec3(t.Translation), ToRp3dQuat(t.Rotation));
    }

    inline void FromRp3dTransform(const rp3d::Transform& tr, TransformComponent& out) 
    {
        if (Renderer::GetAPI() == RenderingAPI::OpenGL) 
        {
            float M[16];
            tr.getOpenGLMatrix(M);
            const glm::mat4 mat = glm::make_mat4(M);
            out.Translation = glm::vec3(mat[3]);            
            out.Rotation    = glm::quat_cast(mat);         
        } 
        else 
        {
            out.Translation = ToGlmVec3(tr.getPosition());
            out.Rotation    = ToGlmQuat(tr.getOrientation());
        }
    }


    inline rp3d::BodyType ToRp3dBodyType(BodyType t)
    {
        switch (t) { case BodyType::Static: return rp3d::BodyType::STATIC; case BodyType::Dynamic: return rp3d::BodyType::DYNAMIC; }
        return rp3d::BodyType::STATIC;
    }

    struct CleanVtxElement 
    {
        std::vector<glm::vec3> vertices;
        std::vector<uint32_t> indices;
    };

    inline bool isFinite(const glm::vec3& v) 
    {
        return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
    }
    inline float triArea2(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) 
    {
        return glm::length(glm::cross(b - a, c - a));
    }

    struct Key3 
    { 
        int x,y,z; 
        bool operator==(const Key3&o)const
        {
            return x == o.x && y==o.y &&  z == o.z;
        } 
    };
    struct KeyHash 
    { 
        size_t operator()(const Key3&k)const
        { 
            return ((size_t)k.x * 73856093) ^ ((size_t)k.y * 19349663) ^ ((size_t) k.z * 83492791);
        } 
    };

    inline Key3 quantize(const glm::vec3& v,float eps)
    {
        return {(int)std::floor(v.x/eps+0.5f), (int)std::floor(v.y/eps+0.5f), (int)std::floor(v.z/eps+0.5f) };
    }

    inline CleanVtxElement SanitizeVtxElement(const std::vector<glm::vec3>& inVerts,const std::vector<uint32_t>& inIdx,float eps,float minA)
    {
        std::unordered_map<Key3,uint32_t,KeyHash> map; std::vector<glm::vec3> verts; 
        verts.reserve(inVerts.size());

        std::vector<uint32_t> remap(inVerts.size(), UINT32_MAX);
        for(size_t i=0;i<inVerts.size();++i)
        { 
            if(!isFinite(inVerts[i])) continue; 
            Key3 k  = quantize(inVerts[i], eps); 
            auto it = map.find(k); 

            if(it == map.end())
            {
                uint32_t ni=verts.size();
                verts.push_back(inVerts[i]);
                map[k]=ni;remap[i]=ni;
            } 
            else 
            {
                remap[i]=it->second;
            }
        }

        std::vector<uint32_t> idx; 
        idx.reserve(inIdx.size());

        glm::vec3 centroid(0); 
        size_t count{0};

        for(size_t t = 0; t + 2 < inIdx.size(); t += 3)
        { 
            uint32_t a = remap[inIdx[t]], b = remap[inIdx[t+1]], c = remap[inIdx[t+2]]; 
            if(a == UINT32_MAX || b == UINT32_MAX || c == UINT32_MAX || a == b || b == c || c == a) continue; 

            float A = triArea2(verts[a], verts[b], verts[c]); 
            if(A<minA) continue; 

            centroid += verts[a] + verts[b] + verts[c]; 
            count    += 3; 
            
            idx.insert(idx.end(), {a, b, c}); 
        }

        if(count)
        { 
            centroid *= 1.0f/(float) count; 
            for(size_t t = 0; t + 2 < idx.size(); t += 3)
            {
                auto a = idx[t], b = idx[t + 1], c = idx[t + 2];
                glm::vec3 n = glm::cross(verts[b] - verts[a], verts[c] - verts[a]); 
                if(glm::dot(n, verts[a] - centroid) < 0) std::swap(idx[t + 1], idx[t + 2]); 
            }
        }

        return {std::move(verts),std::move(idx)};
    }

    inline CleanVtxElement SanitizeConcave(const std::vector<glm::vec3>& verts, const std::vector<uint32_t>& indices, float weldEps = 1e-5f, float minArea = 1e-10f)
    {
        return SanitizeVtxElement(verts, indices, weldEps, minArea);
    }

    inline CleanVtxElement SanitizeConvex(const std::vector<glm::vec3>& verts, const std::vector<uint32_t>& indices, float weldEps = 1e-5f, float minArea = 1e-10f)
    {
        return SanitizeVtxElement(verts, indices, weldEps, minArea);
    }
}