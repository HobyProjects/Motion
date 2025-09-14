#pragma once

#include <vector>

#include "PhyConfig.hpp"
#include "PhyCore.hpp"
#include "Pairwise.hpp"
#include "NaiveBroadphase.hpp"
#include "Contact.hpp"
#include "SweepAndPrune.hpp"
#include "AABB.hpp"
#include "SphereShape.hpp"
#include "BoxShape.hpp"
#include "CapsuleShape.hpp"
#include "ConvexHullShape.hpp"
#include "ContactSolver.hpp"
#include "ConstraintSolver.hpp"
#include "RayCast.hpp"
#include "Sweep.hpp"

namespace Motion
{
    struct BodyHandle { std::uint32_t Index{std::numeric_limits<std::uint32_t>::max()}; };
    struct ColliderHandle { std::uint32_t Index{std::numeric_limits<std::uint32_t>::max()};};
    
    struct WorldCollider 
    {
        Collider Coll{};         
        BodyHandle Body{};     
        ProxyID   Proxy{0}; 
        std::uint32_t  Layer{0xFFFFFFFF};
    };

    class KinetiX
    {
        private:
            KinetiX() = default;
            ~KinetiX() = default;

            KinetiX(const KinetiX&) = delete;
            KinetiX(const KinetiX&&) = delete;
            KinetiX& operator=(const KinetiX&) = delete;
            KinetiX& operator=(const KinetiX&&) = delete;

        public:
            const KinetiX& GetInstance() const 
            {
                static KinetiX instance{};
                return instance;
            }

        public:
            void SetCanSleep(BodyHandle h, bool can);
            void WakeBody(BodyHandle h);
            void ApplyForce(BodyHandle h, const glm::vec3& F, float dt);
            void ApplyImpulse(BodyHandle h, const glm::vec3& P, const glm::vec3& rWS = glm::vec3(0));
            bool IsSleeping(BodyHandle h) const;

        public:
            [[nodiscard]] BodyHandle CreateRigidBody(const MassProperties& massProp, const glm::vec3& pos, const glm::quat& rot, bool staticObj = false);
            [[nodiscard]] ColliderHandle CreateCollider(BodyHandle bh, const Collider& c);
            [[nodiscard]] const WorldCollider& GetCollider(ColliderHandle ch);
            [[nodiscard]] const RigidBody& GetRigidBody(BodyHandle bh);

        public:
            [[maybe_unused]] std::size_t CreateFixedJoint(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local);
            [[maybe_unused]] std::size_t CreateBallSocket(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, float baumgarte = 0.2f);
            [[maybe_unused]] std::size_t CreateHinge(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, const glm::vec3& axisA_local, const glm::vec3& axisB_local, bool enableLimits=false, float lo=-1.0f, float hi=+1.0f, bool enableMotor=false, float motorSpeed=0.0f, float motorTorque=0.0f);
            [[maybe_unused]] std::size_t CreateSlider(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, const glm::vec3& axisA_local, const glm::vec3& axisB_local, glm::vec3 refPerpA_local, glm::vec3 refPerpB_local, bool enableLimits=false, float lo=-1.0f, float hi=+1.0f, bool enableMotor=false, float motorSpeed=0.0f, float motorForce=0.0f);
                    
        public:
            bool SweepSphereFirst(const SweepSphere& q, SweepHit& outHit) const;
            bool SweepWorldSphereFirst(const SweepSphere& q, SweepHit& outHit);
            bool SweepCapsuleFirst(const SweepCapsule& q, SweepHit& outHit);
            bool SweepWorldCapsuleFirst(const SweepCapsule& q, SweepHit& outHit);

        public:
            bool RaycastFirst(const Ray& ray, RayHit& outHit) const;
            void RaycastAll(const Ray& ray, std::vector<RayHit>& outHits) const;

        public:
            void Step(float deltaTime = 0.0f);
        
        private:
            glm::mat4 BodyWorldMatrix(BodyHandle h) const;

        public:
            PhyConfig               Config{};
            SolverSettings          SolveSettings{};
            JointSettings           JSettings{};
            std::uint32_t           FrameCounter{0};
            
        private:
            NarrowPhaseContext      NPContext{};
            SweepAndPrune           BroadPhase{};
            ManifoldCache           ContactCache;

            std::vector<RigidBody>          Bodies{};
            std::vector<WorldCollider>      Colliders{};
            std::vector<FixedJointDesc>     FixedJoints{};
            std::vector<BallSocketDesc>     BallJoints{}; 
            std::vector<HingeDesc>          HingsJoints{};  
            std::vector<SliderDesc>         SliderJoints{};  
    };
}