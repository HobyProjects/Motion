#include "ConcaveMeshShape.hpp"
#include <algorithm>

namespace Motion
{
    static AABB TriAABB(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c) 
    {
        glm::vec3 mn = glm::min(a, glm::min(b, c));
        glm::vec3 mx = glm::max(a, glm::max(b, c));
        return { mn, mx };
    }

    static AABB Merge(const AABB& a, const AABB& b) 
    {
        return { glm::min(a.MIN, b.MIN), glm::max(a.MAX, b.MAX) };
    }

    void ConcaveMeshShape::BuidBHV()
    {
        TriAABBs.resize(Tris.size());
        LocalBounds = { glm::vec3(+FLT_MAX), glm::vec3(-FLT_MAX) };
        for (size_t i = 0; i < Tris.size();++i) 
        {
            const auto t    = Tris[i];
            TriAABBs[i]     = TriAABB(Vertice[t.I0], Vertice[t.I1], Vertice[t.I2]);
            LocalBounds     = Merge(LocalBounds, TriAABBs[i]);
        }

        struct Ref { uint32_t tri; AABB box; glm::vec3 center; };
        std::vector<Ref> refs(Tris.size());
        for (uint32_t i=0;i<Tris.size();++i) 
        {
            const AABB& b = TriAABBs[i];
            refs[i] = { i, b, 0.5f * (b.MIN + b.MAX) };
        }

        Nodes.clear(); Nodes.reserve(Tris.size() * 2);

        std::function<uint32_t(uint32_t,uint32_t)> build = [&](uint32_t begin, uint32_t end) -> uint32_t 
        {
            AABB bounds = { glm::vec3(+FLT_MAX), glm::vec3(-FLT_MAX) };
            for (uint32_t i = begin; i < end; ++i) bounds = Merge(bounds, refs[i].box);

            uint32_t n          = end - begin;
            uint32_t nodeIdx    = (uint32_t)Nodes.size();
            Nodes.push_back({ bounds, 0, 0, false});

            if (n <= 8) 
            {
                Nodes[nodeIdx].Left     = begin;
                Nodes[nodeIdx].Right    = n;
                Nodes[nodeIdx].IsLeaf   = true;
                return nodeIdx;
            }

            glm::vec3 ext   = bounds.MAX - bounds.MIN;
            int axis        = (ext.x > ext.y && ext.x > ext.z) ? 0 : (ext.y > ext.z ? 1 : 2);
            uint32_t mid    = (begin + end)/2;

            std::nth_element(refs.begin()+begin, refs.begin()+mid, refs.begin()+end,
                [&](const Ref& a, const Ref& b){ return a.center[axis] < b.center[axis]; });

            uint32_t L = build(begin, mid);
            uint32_t R = build(mid, end);

            Nodes[nodeIdx].Left     = L;
            Nodes[nodeIdx].Right    = R;
            Nodes[nodeIdx].IsLeaf   = false;

            return nodeIdx;
        };

        if (!refs.empty()) build(0, (uint32_t)refs.size());
        if (!refs.empty()) 
        {
            std::vector<TriMeshTri> tris2(Tris.size());
            std::vector<AABB> aabbs2(Tris.size());
            for (size_t i = 0; i < refs.size(); ++i) 
            {
                tris2[i] = Tris[refs[i].tri];
                aabbs2[i]= TriAABBs[refs[i].tri];
            }

            Tris.swap(tris2);
            TriAABBs.swap(aabbs2);
        }
    }
}