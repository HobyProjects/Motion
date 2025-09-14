#pragma once

#include <glm/glm.hpp>
#include "PhyCore.hpp"

namespace Motion
{
    struct FeatureID 
    {
        std::int32_t FaceA{-1}, EdgeA{-1}, VertA{-1};
        std::int32_t FaceB{-1}, EdgeB{-1}, VertB{-1};
        std::int32_t TriIndex{-1};

        bool SameAs(const FeatureID& o) const 
        {
            return FaceA==o.FaceA && EdgeA==o.EdgeA && VertA==o.VertA &&
                   FaceB==o.FaceB && EdgeB==o.EdgeB && VertB==o.VertB &&
                   TriIndex==o.TriIndex;
        }
    };

    struct ContactWarm 
    {
        float NormalImpulse{0.0f};
        float TangentImpulseU{0.0f};
        float TangentImpulseV{0.0f};
    };

    struct ContactPoint
    {
        glm::vec3 PositionWS{0.0f};
        glm::vec3 NormalWS{0.0f};
        float Penetration{0.0f};
        MaterialProperties MProps{};
    };

    struct PersistentPoint 
    {
        FeatureID FID{};
        ContactPoint CP{};     
        ContactWarm  Warm{};
    };

    struct ContactManifold
    {
        std::int32_t Count{4};
        ContactPoint Points[4];
        glm::vec3 SharedNormalWS;
        float SharedFriction{0.6f};
        float SharedRestitution{0.1f};
    };

    struct NarrowPhaseContext
    {
        float LinearSlop{1e-3};
        float AngularSlop{1e-3};
    };

    struct PersistentManifold 
    {
        glm::vec3 SharedNormalWS{0.0f};
        float SharedFriction{0.5f};
        float SharedRestitution{0.0f};

        PersistentPoint P[4];
        int Count{0};

        glm::vec3 LastGJKDir{1,0,0};
        std::uint32_t LastTouched{0};
    };
}