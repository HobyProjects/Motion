#pragma once

#include "PhyCore.hpp"
#include "AABB.hpp"
#include "ManifoldClip.hpp"

namespace Motion
{
    struct SphereShape final : Shape
    {
        float Radius{0.5f};
        glm::vec3 Center{0.0f};

        SphereShape(float r, float margin = 0.02f)
        {
            Type            = ShapeType::Sphere;
            Radius          = r;
            ConvexRadius    = margin;
        }

        MassProperties  Mass(float density) const override
        {
            const float R       = Radius;
            const float volume  = (4.0f/3.0f) * glm::pi<float>() * R * R * R;
            const float m       = density * volume;
            const float I       = 0.4f * m * R * R;

            MassProperties mp{};
            mp.MASS     = m;
            mp.Inertia  = glm::mat3(I);
            mp.COM      = Center;

            return mp;
        }

        AABB LocalAABB() const override
        {
            const float R = Radius * ConvexRadius;
            return FromCenterExtent(Center, glm::vec3(R));
        }

        glm::vec3 SupportLocal(const glm::vec3& dir) const override
        {
            const float len = glm::length(dir);
            if(len <= 1e-12f) return Center + glm::vec3(Radius, 0.0f, 0.0f);
            return Center + (dir / len) * Radius;
        }

        static float WorldRadius(const SphereShape& s, const glm::mat4& world)
        {
            return s.Radius * MaxAxisScale(world); 
        }

        static glm::vec3 WorldCenter(const SphereShape& s, const glm::mat4& world)
        {
            return s.Radius * TransformPoint(world, s.Center);
        }

        static AABB WorldAABB(const SphereShape& s, const glm::mat4& world)
        {
            const glm::vec3 c   = WorldCenter(s, world);
            const float R       = WorldRadius(s, world) + s.ConvexRadius;
            
            return FromCenterExtent(c, glm::vec3(R));
        }
    };
}