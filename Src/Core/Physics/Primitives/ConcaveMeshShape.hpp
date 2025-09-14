#pragma once

#include <vector>
#include <cstdint>
#include <limits>
#include <glm/glm.hpp>

#include "AABB.hpp"
#include "PhyCore.hpp"
#include "PhyMath.hpp"

namespace Motion
{
    struct TriMeshTri
    {
        std::uint32_t I0{0}, I1{0}, I2{0};
    };

    struct BVHNode
    {
        AABB Box{};
        std::uint32_t Left{0};
        std::uint32_t Right{0};
        bool IsLeaf{false};
    };


    struct ConcaveMeshShape : Shape
    {
        std::vector<glm::vec3> Vertice{};
        std::vector<TriMeshTri> Tris{};
        std::vector<AABB> TriAABBs{};
        std::vector<BVHNode> Nodes{};
        AABB LocalBounds{};
        float WeldSlop{1e-4f};

        ConcaveMeshShape() { Type = ShapeType::Concave; }

        void BuidBHV();

        AABB WorldAABB(const glm::mat4& M) const
        {
            const glm::vec3 mn = LocalBounds.MIN;
            const glm::vec3 mx = LocalBounds.MAX;
            glm::vec3 corners[8] = 
            {
                {mn.x,mn.y,mn.z},{mx.x,mn.y,mn.z},{mn.x,mx.y,mn.z},{mx.x,mx.y,mn.z},
                {mn.x,mn.y,mx.z},{mx.x,mn.y,mx.z},{mn.x,mx.y,mx.z},{mx.x,mx.y,mx.z}
            };

            glm::vec3 wmn(+FLT_MAX), wmx(-FLT_MAX);
            
            for (auto& c : corners) 
            {
                glm::vec3 p = glm::vec3(M * glm::vec4(c,1));
                wmn = glm::min(wmn, p); wmx = glm::max(wmx, p);
            }
            return { wmn, wmx };
        }
    };

}