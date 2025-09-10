#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>
#include <entt/entt.hpp>
#include <glm/glm.hpp>

#include "Entity.hpp"
#include "Components.hpp"

namespace Motion
{
    using EntityPtr = std::shared_ptr<Entity>;

    class KinetiX
    {
        public:
            // ---- Singleton ----
            static KinetiX& Get()
            {
                static KinetiX instance;
                return instance;
            }

            // ---- Config/Stats ----
            struct Config
            {
                glm::vec3 GravityDirection    = {0.0f, -1.0f, 0.0f};
                float     GravityMagnitude    = Units::g_mps2;  // uses your Units
                std::uint32_t SolverVelocityIterations = 8;
                std::uint32_t SolverPositionIterations = 2;
                bool EnableCCD                = false;
                bool EnableSleeping           = true;
                bool WarmStartContacts        = true;
                float BaumgarteBias           = 0.2f;
                float AllowedPenetration      = 0.01f;
            };

            struct Stats
            {
                std::uint32_t Bodies            = 0;
                std::uint32_t Colliders         = 0;
                std::uint32_t PairsBroadphase   = 0;
                std::uint32_t PairsNarrowphase  = 0;
                std::uint32_t ContactsGenerated = 0;
                float LastStepDT                = 0.0f;
            };

            struct ContactPoint
            {
                glm::vec3 PositionWorld{0.0f};
                float     NormalImpulse{0.0f};
                float     TangentImpulse1{0.0f};
                float     TangentImpulse2{0.0f};
            };

            struct Manifold
            {
                EntityPtr A{};
                EntityPtr B{};
                glm::vec3 Normal{0.0f};   // A -> B
                float     Depth{0.0f};
                ContactPoint Points[4]{};
                std::uint8_t Count{0};
                // Optional: material mix cached here if you like
            };

            struct Pair
            {
                EntityPtr A{};
                EntityPtr B{};
                glm::vec3 CachedAxis{1,0,0}; // last sep/penetration axis for warm-start
                std::uint32_t LastTouchedFrame{0};
            };

        public:
            // ---- Public API ----
            void SetConfig(const Config& cfg);
            const Config& GetConfig() const;

            const Stats& GetStats() const;

            // Single entry point: you hand me your scene's entity vector every frame
            void Step(float dt, const std::vector<EntityPtr>& entities);

            // Optional decoupling for render/physics ticks
            void SyncFromECS(const std::vector<EntityPtr>& entities);
            void SyncToECS(const std::vector<EntityPtr>& entities);

            // Debug/inspection
            const std::vector<Pair>&     GetActivePairs() const;
            const std::vector<Manifold>& GetActiveManifolds() const;

            // Optional: rebuild mesh collider acceleration (BVH) if shape changes
            void RebuildConcaveAcceleration(const EntityPtr& e);

        private:
            // ---- Singleton plumbing ----
            KinetiX() = default;
            ~KinetiX() = default;
            KinetiX(const KinetiX&) = delete;
            KinetiX& operator=(const KinetiX&) = delete;

        private:
            // ---- Pipeline stages (private; one-class design) ----
            void GatherActive(const std::vector<EntityPtr>& entities); // collect & cache component ptrs
            void UpdateWorldAABBs(const std::vector<EntityPtr>& entities);
            void UpdateSweptAABBs(const std::vector<EntityPtr>& entities, float dt);
            void UpdateInertiaTensors(); // uses RigidBodyComponent::SyncInertia

            void Integrate(float dt);      // uses forces/torques/gravity from Config + Units
            void Broadphase();             // build m_BroadphasePairs from ColliderComponent::WorldAABB
            void Midphase();               // concave mesh refinement using collider Shape
            void Narrowphase(float dt);    // CollisionDetector<>, uses ColliderComponent::Shape and Type
            void BuildManifolds();         // reduce points, combine PhysicalMaterial
            void SolveConstraints(float dt);
            void PostIntegrate(float dt);  // damping, clamping, sleeping (uses thresholds from RigidBodyComponent)
            void CCD(float dt);            // optional swept tests if RigidBodyComponent::CCDEnabled
            void WriteBack();              // write corrected T/R back to TransformComponent

        private:
            // ---- Per-frame caches (parallel arrays for hot loops) ----
            std::vector<EntityPtr>              m_Entities;   // filtered (HasComponent) and alive
            std::vector<TransformComponent*>    m_Tr;         // current
            std::vector<TransformHistoryComponent*> m_TrPrev; // if present
            std::vector<RigidBodyComponent*>    m_Rb;
            std::vector<ColliderComponent*>     m_Co;
            std::vector<DampingComponent*>      m_Dp;

            // ---- Collision data ----
            std::vector<Pair>                   m_BroadphasePairs;
            std::vector<Pair>                   m_MidphasePairs;
            std::vector<Manifold>               m_Manifolds;

            // ---- Bookkeeping ----
            Config                              m_Config{};
            Stats                               m_Stats{};
            std::uint32_t                       m_FrameIndex{0};
    };
}