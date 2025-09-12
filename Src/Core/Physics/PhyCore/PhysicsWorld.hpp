#pragma once

#include <vector>

#include "Types.hpp"
#include "Pairwise.hpp"
#include "NaiveBroadphase.hpp"
#include "Contact.hpp"

namespace Motion
{
    struct BodyHandle
    {
        std::uint32_t Index{std::numeric_limits<std::uint32_t>::max()};
    };

    struct WorldCollider
    {
        Collider Coll{};
        glm::mat4 World{1.0f};
        ProxyID ID{0};
    };

    struct PhysicWorld
    {
        NarrowPhaseContext CTX{};
        NaiveBroadPhase BroadPhase{};
        std::vector<WorldCollider> Colliders{};

        BodyHandle AddCollider(const Collider& c, const glm::mat4& world) 
        {
            WorldCollider wc{ c, world * c.LocalPose, 0 };
            AABB aabb{};

            switch (c.ColliderShape->Type) 
            {
                case ShapeType::Sphere: 
                {
                    auto& S = *static_cast<const SphereShape*>(c.ColliderShape);
                    aabb = SphereShape::WorldAABB(S, world * c.LocalPose);
                } break;

                case ShapeType::Box: 
                {
                    auto& B = *static_cast<const BoxShape*>(c.ColliderShape);
                    aabb = BoxShape::WorldAABB(world * c.LocalPose, B.HalfExtents, B.ConvexRadius);
                } break;

                case ShapeType::Capsule: 
                {
                    auto& K = *static_cast<const CapsuleShape*>(c.ColliderShape);
                    aabb = CapsuleShape::WorldAABB(K, world * c.LocalPose);
                } break;
            }

            wc.ID = BroadPhase.CreateProxy(aabb, 0xFFFFFFFF, (void*)(uintptr_t)Colliders.size());
            Colliders.push_back(wc);
            return { (uint32_t)(Colliders.size() - 1) };
        }

        void MoveCollider(BodyHandle h, const glm::mat4& newWorld) 
        {
            auto& wc = Colliders[h.Index];
            wc.World = newWorld * wc.Coll.LocalPose;

            AABB aabb{};
            switch (wc.Coll.ColliderShape->Type) 
            {
                case ShapeType::Sphere: 
                {
                    auto& S = *static_cast<const SphereShape*>(wc.Coll.ColliderShape);
                    aabb    = SphereShape::WorldAABB(S, newWorld * wc.Coll.LocalPose);
                } break;

                case ShapeType::Box: 
                {
                    auto& B = *static_cast<const BoxShape*>(wc.Coll.ColliderShape);
                    aabb    = BoxShape::WorldAABB(newWorld * wc.Coll.LocalPose, B.HalfExtents, B.ConvexRadius);
                } break;

                case ShapeType::Capsule: 
                {
                    auto& K = *static_cast<const CapsuleShape*>(wc.Coll.ColliderShape);
                    aabb    = CapsuleShape::WorldAABB(K, newWorld * wc.Coll.LocalPose);
                } break;
            }

            BroadPhase.MoveProxy(wc.ID, aabb);
        }

        struct Pair { uint32_t ia, ib; ContactManifold m; };

        void ComputeContacts(std::vector<Pair>& out) 
        {
            out.clear();
            std::vector<std::pair<void*, void*>> pairs;
            BroadPhase.QueryOverlaps(pairs);

            for (auto& pr : pairs) {
                uint32_t ia = (uint32_t)(uintptr_t)pr.first;
                uint32_t ib = (uint32_t)(uintptr_t)pr.second;
                const auto& A = Colliders[ia];
                const auto& B = Colliders[ib];

                ContactManifold m{};
                if (Collide(A.Coll, glm::mat4(1.0f), B.Coll, glm::mat4(1.0f), CTX, m)) {
                    out.push_back({ ia, ib, m });
                }
            }
        }
    };
}