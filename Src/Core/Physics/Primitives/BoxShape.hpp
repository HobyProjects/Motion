#pragma once

#include "Types.hpp"
#include "AABB.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtx/compatibility.hpp> 

namespace Motion
{
    inline glm::mat3 Mat3FromMat4(const glm::mat4& M) 
    {
        return glm::mat3(glm::vec3(M[0]), glm::vec3(M[1]), glm::vec3(M[2]));
    }

    inline glm::mat3 AbsMat3(const glm::mat3& M) 
    {
        return glm::mat3(glm::abs(M[0]), glm::abs(M[1]), glm::abs(M[2]));
    }

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

    struct BoxShape final : Shape
    {
        glm::vec3 HalfExtents{0.5f};

        BoxShape(const glm::vec3& he, float margin = 0.02f)
        {
            Type            = ShapeType::Box;
            HalfExtents     = he;
            ConvexRadius    = margin;
        }

        MassProperties Mass(float density) const override
        {
            const glm::vec3 e   = HalfExtents;
            const float volume  = 8.0f * e.x * e.y * e.z;   
            const float m       = density * volume;

            const float Ixx = (1.0f/3.0f) * m * (e.y * e.y + e.z * e.z);
            const float Iyy = (1.0f/3.0f) * m * (e.x * e.x + e.z * e.z);
            const float Izz = (1.0f/3.0f) * m * (e.x * e.x + e.y * e.y);

            MassProperties mp;
            mp.MASS = m;
            mp.Inertia = glm::mat3(
                glm::vec3(Ixx, 0,   0),
                glm::vec3(0,   Iyy, 0),
                glm::vec3(0,   0,   Izz)
            );
            mp.COM = glm::vec3(0.0f);
            return mp;
        }

        AABB LocalAABB() const override
        {
            const glm::vec3 e = HalfExtents + glm::vec3(ConvexRadius);
            return FromCenterExtent(glm::vec3(0), e);
        }

        glm::vec3 SupportLocal(const glm::vec3& dir) const override
        {
            glm::vec3 s(
                dir.x >= 0 ? HalfExtents.x : -HalfExtents.x,
                dir.y >= 0 ? HalfExtents.y : -HalfExtents.y,
                dir.z >= 0 ? HalfExtents.z : -HalfExtents.z
            );

            return s;            
        }

        static AABB WorldAABB(const glm::mat4& world, const glm::vec3& he, float convexRadius)
        {
            const glm::vec3 c   = TransformPoint(world, glm::vec3(0.0f));
            const glm::mat3 R   = Mat3FromMat4(world);      
            const glm::mat3 AR  = AbsMat3(R);
            const glm::vec3 e   = AR * (he + glm::vec3(convexRadius));

            return FromCenterExtent(c, e);       
        }

        static void AxesFromWorld(const glm::mat4& world, glm::vec3 axes[3], glm::vec3& center)
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

            center = TransformPoint(world, glm::vec3(0.0f));
        }
    };
}