#pragma once

#include <memory>

#include "PhyCore.hpp"
#include "Contact.hpp"
#include "GJK.hpp"
#include "ManifoldClip.hpp"

#include "SphereShape.hpp"
#include "BoxShape.hpp"
#include "CapsuleShape.hpp"
#include "ConvexHullShape.hpp"
#include "ConcaveMeshShape.hpp"

namespace Motion
{
    struct PairKey 
    {
        std::uint32_t A{0}, B{0};
        bool operator==(const PairKey& o) const { return A==o.A && B==o.B; }
    };
    struct PairKeyHash 
    {
        size_t operator()(const PairKey& k) const noexcept 
        {
            return (size_t(k.A) << 32) ^ size_t(k.B * 0x9e3779b1u);
        }
    };
    struct PersistSettings 
    {
        float KeepDistance{0.02f};    
        float KeepNormalCos{0.95f};     
        float MaxPoints{4.0f};          
    };

    using ManifoldCache = std::unordered_map<PairKey, PersistentManifold, PairKeyHash>;

    AABB ComputeWorldAABB(const Shape& s, const glm::mat4& M);
    void RefreshPersistent(const ContactManifold& fresh, const PersistSettings& ps, PersistentManifold& cacheOut);

    bool CollideSphereSphere(const SphereShape& A, const glm::mat4& WA, const SphereShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out);
    bool CollideSphereBox(const SphereShape& A, const glm::mat4& WA, const BoxShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out);
    bool CollideSphereCapsule(const SphereShape& S, const glm::mat4& WS, const CapsuleShape& C, const glm::mat4& WC, const NarrowPhaseContext& ctx, ContactManifold& out);
    
    bool CollideCapsuleCapsule(const CapsuleShape& A, const glm::mat4& WA, const CapsuleShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out);
    bool CollideCapsuleBox(const CapsuleShape& A, const glm::mat4& WA, const BoxShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out);
    
    bool CollideBoxBox(const BoxShape& A, const glm::mat4& WA, const BoxShape& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out);
    
    bool CollideConvexFallback(const Collider& A, const glm::mat4& WA, const Collider& B, const glm::mat4& WB, const NarrowPhaseContext& ctx, ContactManifold& out);
    bool CollideAgainstConcave(const Collider& dynCol, const glm::mat4& Wdyn, const Collider& meshCol, const glm::mat4& Wmesh, const NarrowPhaseContext& ctx, ContactManifold& out);
    
    bool Collide(const Collider& a, const glm::mat4& worldA, const Collider& b, const glm::mat4& worldB, const NarrowPhaseContext& ctx, ContactManifold& out);
}