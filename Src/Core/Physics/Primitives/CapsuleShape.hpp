#pragma once

#include "PhyCore.hpp"
#include "AABB.hpp"
#include "ManifoldClip.hpp"

namespace Motion
{

    struct CapsuleShape final : Shape
    {
        float HalfHeight{0.5f};
        float Radius{0.25f};

        CapsuleShape(float hh, float r, float margin = 0.02f)
        {
            Type            = ShapeType::Capsule;
            HalfHeight      = hh;
            Radius          = r;
            ConvexRadius    = margin;
        }

        MassProperties Mass(float density) const override
        {
            const float r = Radius;
            const float L = 2.0f * HalfHeight;

            const float vol_cyl         = glm::pi<float>() * r * r * L;
            const float vol_sph_total   = (4.0f/3.0f) * glm::pi<float>() * r * r * r;   
            const float m_c             = density * vol_cyl;
            const float m_s_total       = density * vol_sph_total;
            const float m               = m_c + m_s_total;

            const float Iyy_c = 0.5f  * m_c * r * r;                  
            const float Ixx_c = 1.0f/12.0f * m_c * (3 * r * r + L * L);   
            const float Izz_c = Ixx_c;

            const float m_e         = 0.5f * m_s_total;
            const float d           = HalfHeight;                           
            const float I_s_center  = 0.4f * m_e * r * r;              

            const float Iyy_s = 2.0f * I_s_center;
            const float Ixx_s = 2.0f * (I_s_center + m_e * d * d);
            const float Izz_s = Ixx_s;

            MassProperties mp{};
            mp.MASS     = m;
            mp.Inertia  = glm::mat3(
                glm::vec3(Ixx_c + Ixx_s, 0, 0),
                glm::vec3(0, Iyy_c + Iyy_s, 0),
                glm::vec3(0, 0, Izz_c + Izz_s)
            );
            mp.COM = glm::vec3(0.0f);
            return mp;
        }

        AABB LocalAABB() const override
        {
            const glm::vec3 e(Radius + ConvexRadius, HalfHeight + Radius + ConvexRadius, Radius + ConvexRadius);
            return FromCenterExtent(glm::vec3(0.0f), e);
        }

        glm::vec3 SupportLocal(const glm::vec3& dir) const override
        {
            const float len = glm::length(dir);
            if (len <= 1e-12f) return glm::vec3(0, HalfHeight, 0) + glm::vec3(Radius,0,0);
            
            const glm::vec3 nd  = dir / len;
            const float s       = (glm::dot(nd, glm::vec3(0,1,0)) >= 0.0f) ? 1.0f : -1.0f;
            
            return glm::vec3(0, s * HalfHeight, 0) + nd * Radius;
        }

        static void WorldParams(const CapsuleShape& cap, const glm::mat4& world, glm::vec3& axisY, glm::vec3& center, float& rWorld, float& hWorld)
        {
            float sx{0.0f}, sy{0.0f}, sz{0.0f};
            const glm::vec3 x = AxisFromCol(world, 0, sx);
            const glm::vec3 y = AxisFromCol(world, 1, sy);
            const glm::vec3 z = AxisFromCol(world, 2, sz);

            axisY  = y;
            center = TransformPoint(world, glm::vec3(0.0f));
            rWorld = cap.Radius * glm::max(sx, sz);
            hWorld = cap.HalfHeight * sy;
        }

        static void WorldSegment(const CapsuleShape& cap, const glm::mat4& world, glm::vec3& A, glm::vec3& B, float& rWorld) 
        {
            glm::vec3 axis, c; float r, h;
            WorldParams(cap, world, axis, c, r, h);
            A = c + axis * h;
            B = c - axis * h;
            rWorld = r;
        }

        static AABB WorldAABB(const CapsuleShape& cap, const glm::mat4& world, float convexRadius = 0.0f) 
        {
            glm::vec3 A{0.0f}, B{0.0f}; float r{0.0f};
            WorldSegment(cap, world, A, B, r);

            const float R   = r + convexRadius;
            glm::vec3 mn    = glm::min(A, B);
            glm::vec3 mx    = glm::max(A, B);

            mn -= glm::vec3(R);
            mx += glm::vec3(R);

            return { mn, mx };
        }
    };
}