#pragma once

#include <vector>

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
#include "Pairwise.hpp"
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

    struct WorldSettings
    {
        float DeltaTime{1.0f/60.0f};
        glm::vec3 Gravity{0.0f, -9.81f, 0.0f};
        float FatAABBVelocityPad{0.5f};
    };

    inline AABB SweptAABB(const glm::vec3& o, const glm::vec3& dir, float tMax, float r) 
    {
        glm::vec3 a = o;
        glm::vec3 b = o + dir * tMax;
        glm::vec3 mn = glm::min(a, b) - glm::vec3(r);
        glm::vec3 mx = glm::max(a, b) + glm::vec3(r);
        return { mn, mx };
    }

    struct PhysicWorld
    {
        SweepAndPrune           BroadPhase{};
        NarrowPhaseContext      NPContext{};
        SolverSettings          SolveSettings{};
        WorldSettings           WSettings{};
        JointSettings           JSettings{}; 

        std::vector<RigidBody>          Bodies{};
        std::vector<WorldCollider>      Colliders{};
        std::vector<FixedJointDesc>     FixedJoints{};
        std::vector<BallSocketDesc>     BallJoints{}; 
        std::vector<HingeDesc>          HingsJoints{};  
        std::vector<SliderDesc>         SliderJoints{};  


#pragma region Colliders and RgidBody 

        void SetCanSleep(BodyHandle h, bool can) 
        {
            Bodies[h.Index].CanSleep = can;
            if (!can) Bodies[h.Index].WakeUp();
        }

        void WakeBody(BodyHandle h) { Bodies[h.Index].WakeUp(); }

        bool IsSleeping(BodyHandle h) const { return Bodies[h.Index].Sleeping; }

        void ApplyForce(BodyHandle h, const glm::vec3& F) 
        {
            auto& b = Bodies[h.Index];
            if (b.InvMass == 0.0f) return;
            b.WakeUp();
            b.LinearVelocity += F * (b.InvMass * WSettings.DeltaTime);
        }

        void ApplyImpulse(BodyHandle h, const glm::vec3& P, const glm::vec3& rWS = glm::vec3(0)) 
        {
            auto& b = Bodies[h.Index];
            if (b.InvMass == 0.0f) return;
            b.WakeUp();
            b.ApplyLinearImpulse(P);
            if (glm::length2(rWS) > 0) b.ApplyAngularImpulse(glm::cross(rWS, P));
        }

        BodyHandle CreateBody(const MassProperties& massProp, const glm::vec3& pos, const glm::quat& rot, bool staticObj = false)
        {
            RigidBody rb{};
            rb.Position = pos;
            rb.Rotation = rot;

            if(staticObj)
            {
                rb.InvMass = 0.0f;
                rb.InvInertiaLocal = glm::mat3(0.0f);
            }
            else
            {
                rb.InvMass = (massProp.MASS > 0.0f) ? (1.0f / massProp.MASS) : 0.0f;
                glm::mat3 invI(0.0f);
                for(std::int32_t a = 0; a < 3; ++a)
                {
                    float Iaa = massProp.Inertia[a][a];
                    invI[a][a] = (Iaa > 1e-9f) ? 1.0f / Iaa : 0.0f;
                }

                rb.InvInertiaLocal = invI;
            }

            rb.SyncInertiaWorld();
            Bodies.push_back(rb);
            return {(std::uint32_t)Bodies.size() - 1 };
        }

        ColliderHandle AddCollider(BodyHandle bh, const Collider& c) 
        {
            WorldCollider wc{};
            wc.Coll  = c;
            wc.Body  = bh;
            wc.Layer = c.Filter;

            const glm::mat4 M   = BodyWorldMatrix(bh) * c.LocalPose;
            AABB aabb           = ComputeWorldAABB(*c.ColliderShape, M);
            wc.Proxy            = BroadPhase.CreateProxy(FattenAABB(aabb, glm::vec3(0)), wc.Layer, (void*)(uintptr_t)Colliders.size());

            Colliders.push_back(wc);
            return { (uint32_t)Colliders.size() - 1 };
        }

#pragma endregion
#pragma region Joints Creation

        std::size_t CreateFixedJoint(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local)
        {
            FixedJointDesc jd{};
            jd.A = (int)a.Index; 
            jd.B = (int)b.Index;

            jd.AnchorA      = anchorA_local;
            jd.AnchorB      = anchorB_local;
            jd.RotationA    = glm::quat(1,0,0,0);
            jd.RotationB    = glm::quat(1,0,0,0);

            FixedJoints.push_back(jd);
            return FixedJoints.size() - 1;
        }

        std::size_t CreateBallSocket(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, float baumgarte = 0.2f)
        {
            BallSocketDesc jd{};
            jd.A            = (int)a.Index;
            jd.B            = (int)b.Index;
            jd.AnchorA      = anchorA_local;
            jd.AnchorB      = anchorB_local;
            jd.Baumgarte    = baumgarte;

            BallJoints.push_back(jd);
            return BallJoints.size() - 1;
        }

        std::size_t CreateHinge(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, const glm::vec3& axisA_local, const glm::vec3& axisB_local, bool enableLimits=false, float lo=-1.0f, float hi=+1.0f, bool enableMotor=false, float motorSpeed=0.0f, float motorTorque=0.0f)
        {
            HingeDesc jd{};
            jd.A = (int)a.Index; jd.B = (int)b.Index;

            jd.AnchorA          = anchorA_local; jd.AnchorB = anchorB_local;
            jd.AxisA            = glm::normalize(axisA_local);
            jd.AxisB            = glm::normalize(axisB_local);
            jd.EnableLimits     = enableLimits; jd.LimitLow = lo; jd.LimitHigh = hi;
            jd.EnableMotor      = enableMotor;  jd.MotorSpeed = motorSpeed; jd.MotorTorque = motorTorque;

            HingsJoints.push_back(jd);
            return HingsJoints.size() - 1;
        }

        std::size_t CreateSlider(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, const glm::vec3& axisA_local, const glm::vec3& axisB_local, glm::vec3 refPerpA_local, glm::vec3 refPerpB_local, bool enableLimits=false, float lo=-1.0f, float hi=+1.0f, bool enableMotor=false, float motorSpeed=0.0f, float motorForce=0.0f)
        {
            auto ortho = [](const glm::vec3& axis, glm::vec3 v)
            {
                glm::vec3 a = glm::normalize(axis);
                v           = v - a * glm::dot(a, v);

                if (glm::length2(v) < 1e-8f) 
                {
                    glm::vec3 h     = (std::abs(a.x)<0.577f)? glm::vec3(1,0,0) : (std::abs(a.y)<0.577f)? glm::vec3(0,1,0) : glm::vec3(0,0,1);
                    v               = glm::normalize(glm::cross(a, h));
                } 
                else v            = glm::normalize(v);

                return v;
            };

            SliderDesc jd{};
            jd.A = (int)a.Index; jd.B = (int)b.Index;
            
            jd.AnchorA      = anchorA_local; jd.AnchorB = anchorB_local;
            jd.AxisA        = glm::normalize(axisA_local);
            jd.AxisB        = glm::normalize(axisB_local);
            jd.RefPerpA     = ortho(jd.AxisA, refPerpA_local);
            jd.RefPerpB     = ortho(jd.AxisB, refPerpB_local);
            jd.EnableLimits = enableLimits; jd.LimitLow = lo; jd.LimitHigh = hi;
            jd.EnableMotor  = enableMotor;  jd.MotorSpeed = motorSpeed; jd.MotorForce = motorForce;

            SliderJoints.push_back(jd);
            return SliderJoints.size()-1;
        }


#pragma endregion
#pragma region CCD Helpers

        bool SweepSphereFirst(const SweepSphere& q, SweepHit& outHit) const 
        {
            outHit = SweepHit{};
            bool any = false;
            AABB swept = SweptAABB(q.Origin, q.Dir, q.TMax, q.R);

            for (size_t i = 0; i < Colliders.size(); ++i) 
            {
                const auto& wc = Colliders[i];
                const auto& proxy = BroadPhase.Proxies[wc.Proxy - 1]; 
                if (!Overlap(swept, proxy.Body)) continue;

                SweepHit h; h.T = outHit.T;
                glm::mat4 W = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                if (SweepSphereAgainstCollider(q, wc.Coll, W, h)) 
                {
                    h.HitCollider = &wc.Coll;
                    if (h.T < outHit.T) { outHit = h; any = true; }
                }
            }

            return any;
        }
        void SweepSphereAll(const SweepSphere& q, std::vector<SweepHit>& outHits) const 
        {
            outHits.clear();
            AABB swept = SweptAABB(q.Origin, q.Dir, q.TMax, q.R);

            for (size_t i = 0; i < Colliders.size(); ++i) 
            {
                const auto& wc = Colliders[i];
                const auto& proxy = BroadPhase.Proxies[wc.Proxy - 1];
                if (!Overlap(swept, proxy.Body)) continue;

                SweepHit h; 
                glm::mat4 W = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                if (SweepSphereAgainstCollider(q, wc.Coll, W, h)) 
                {
                    h.HitCollider = &wc.Coll;
                    outHits.push_back(h);
                }
            }

            std::sort(outHits.begin(), outHits.end(), [](const SweepHit& a, const SweepHit& b){ return a.T < b.T; });
        }

        bool SweepWorldSphereFirst(const SweepSphere& q, SweepHit& outHit)
        {
            outHit      = SweepHit{};
            bool any    = false;
            AABB swept = SweptAABB(q.Origin, q.Dir, q.TMax, q.R);

            for (size_t i = 0; i < Colliders.size(); ++i) 
            {
                const auto& wc      = Colliders[i];
                const auto& proxy   = BroadPhase.Proxies[wc.Proxy - 1]; 
                if (!Overlap(swept, proxy.Body)) continue;

                glm::mat4 W     = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                SweepHit h; h.T = outHit.T;
                if (SweepSphereAgainstCollider(q, wc.Coll, W, h)) 
                {
                    h.HitCollider = &wc.Coll;
                    if (h.T < outHit.T) { outHit = h; any = true; }
                }
            }

            return any;
        };

        inline AABB SweptCapsuleAABB(const glm::vec3& A0, const glm::vec3& B0, const glm::vec3& dir, float tMax, float r) 
        {
            glm::vec3 mn = glm::min(A0, B0);
            glm::vec3 mx = glm::max(A0, B0);

            glm::vec3 A1 = A0 + dir * tMax;
            glm::vec3 B1 = B0 + dir * tMax;
            mn = glm::min(mn, glm::min(A1, B1));
            mx = glm::max(mx, glm::max(A1, B1));

            mn -= glm::vec3(r);
            mx += glm::vec3(r);
            return { mn, mx };
        }

        bool SweepCapsuleFirst(const SweepCapsule& q, SweepHit& outHit) 
        {
            outHit          = SweepHit{};
            bool any        = false;
            AABB sweepAABB  = SweptCapsuleAABB(q.A0, q.B0, q.Dir, q.TMax, q.R);

            for (size_t i = 0; i < Colliders.size(); ++i) 
            {
                const auto& wc      = Colliders[i];
                const auto& proxy   = BroadPhase.Proxies[wc.Proxy - 1]; 
                if (!Overlap(sweepAABB, proxy.Body)) continue;

                glm::mat4 W = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                SweepHit h; h.T = outHit.T;

                if (SweepCapsuleApprox3SpheresAgainstCollider(q, wc.Coll, W, h)) 
                {
                    h.HitCollider = &wc.Coll;
                    if (h.T < outHit.T) { outHit = h; any = true; }
                }
            }

            return any;
        }

        bool SweepWorldCapsuleFirst(const SweepCapsule& q, SweepHit& outHit)
        {
            outHit      = SweepHit{};
            bool any    = false;
            AABB swept  = SweptCapsuleAABB(q.A0, q.B0, q.Dir, q.TMax, q.R);

            for (size_t i=0; i<Colliders.size(); ++i) 
            {
                const auto& wc      = Colliders[i];
                const auto& proxy   = BroadPhase.Proxies[wc.Proxy - 1];
                if (!Overlap(swept, proxy.Body)) continue;

                glm::mat4 W = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                SweepHit h; h.T = outHit.T;

                if (SweepCapsuleApprox3SpheresAgainstCollider(q, wc.Coll, W, h)) 
                {
                    h.HitCollider = &wc.Coll;
                    if (h.T < outHit.T) { outHit = h; any = true; }
                }
            }

            return any;
        };

#pragma endregion
#pragma region Physics Pipline

        void Step() 
        {
            const float dt = WSettings.DeltaTime;

            //0 - Gravity
            for (auto& b : Bodies) 
            {
                if (b.InvMass > 0.0f && !b.Sleeping) 
                    b.LinearVelocity += WSettings.Gravity * dt;
            }

            // 1 - Inertia
            for (auto& b : Bodies) b.SyncInertiaWorld();


            // 1.5 - CCD
            std::vector<float> remainDt(Bodies.size(), dt);
            auto BodyCCDRadius = [&](std::size_t bi) -> float 
            {
                float r     = 0.0f;
                bool first  = true;
                AABB uni{};

                for (const auto& wc : Colliders) 
                {
                    if (wc.Body.Index != bi) continue;
                    const glm::mat4 M   = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                    const AABB a        = ComputeWorldAABB(*wc.Coll.ColliderShape, M);

                    if (first) 
                    { 
                        uni = a; 
                        first = false; 
                    }
                    else 
                    {
                        uni.MIN = glm::min(uni.MIN, a.MIN);
                        uni.MAX = glm::max(uni.MAX, a.MAX);
                    }
                }

                if (!first) 
                {
                    r = 0.5f * glm::length(uni.MAX - uni.MIN);
                }

                return r;
            };

            float ccdRatio = 0.5f;
            for (std::size_t bi = 0; bi < Bodies.size(); ++bi) 
            {
                auto& B = Bodies[bi];
                if (B.InvMass == 0.0f || B.Sleeping) continue;

                glm::vec3 motion = B.LinearVelocity * dt;
                float dist = glm::length(motion);
                if (dist <= 1e-6f) continue;

                const Collider* capCol = nullptr;
                for (const auto& wc : Colliders) 
                {
                    if (wc.Body.Index == bi && wc.Coll.ColliderShape && wc.Coll.ColliderShape->Type == ShapeType::Capsule) 
                    {
                        capCol = &wc.Coll; break;
                    }
                }

                if (capCol) 
                {
                    const auto& K   = *static_cast<const CapsuleShape*>(capCol->ColliderShape);
                    glm::mat4 W     = BodyWorldMatrix({(uint32_t)bi}) * capCol->LocalPose;

                    glm::vec3 A0, B0; float rCap;
                    CapsuleShape::WorldSegment(K, W, A0, B0, rCap);
                    rCap += K.ConvexRadius;

                    SweepCapsule q;
                    q.A0 = A0; q.B0 = B0; q.R = rCap;
                    q.Dir = motion / dist; q.TMax = dist;

                    SweepHit hit;
                    if (SweepWorldCapsuleFirst(q, hit)) 
                    {
                        const float slop = 1e-4f;
                        float toi = glm::max(0.0f, hit.T - slop);
                        B.Position += q.Dir * toi;

                        float vn = glm::dot(B.LinearVelocity, hit.Normal);
                        if (vn < 0.0f) 
                        {
                            float e             = 0.1f; 
                            glm::vec3 vT        = B.LinearVelocity - vn * hit.Normal;
                            B.LinearVelocity    = vT - e * vn * hit.Normal;
                        }

                        float usedDt = toi / (dist / dt);
                        remainDt[bi] = glm::max(0.0f, dt - usedDt);
                    }
                } 
                else 
                {
                    float radius = BodyCCDRadius(bi);
                    if (radius <= 0.0f || dist < ccdRatio * radius) continue;

                    SweepSphere q;
                    q.Origin    = B.Position; q.Dir = motion / dist; q.R = radius;
                    q.TMax = dist;

                    SweepHit hit;
                    if (SweepWorldSphereFirst(q, hit)) 
                    {
                        const float slop = 1e-4f;
                        float toi = glm::max(0.0f, hit.T - slop);
                        B.Position += q.Dir * toi;

                        float vn = glm::dot(B.LinearVelocity, hit.Normal);
                        if (vn < 0.0f) 
                        {
                            float e             = 0.1f;
                            glm::vec3 vT        = B.LinearVelocity - vn * hit.Normal;
                            B.LinearVelocity    = vT - e * vn * hit.Normal;
                        }

                        float usedDt = toi / (dist / dt);
                        remainDt[bi] = glm::max(0.0f, dt - usedDt);
                    }
                }
            }

            // 2 - Update proxies at the (possibly advanced) positions 
            for (size_t ci = 0; ci < Colliders.size(); ++ci) 
            {
                auto& wc            = Colliders[ci];
                const auto& B       = Bodies[wc.Body.Index];
                const glm::mat4 M   = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                const AABB aabb     = ComputeWorldAABB(*wc.Coll.ColliderShape, M);

                glm::vec3 velPad(0);
                if (!B.Sleeping) velPad = glm::abs(B.LinearVelocity) * dt * WSettings.FatAABBVelocityPad;
                BroadPhase.MoveProxy(wc.Proxy, FattenAABB(aabb, velPad));
            }

            // 3) Broadphase  
            // 4) Narrow-phase  
            // 5) Solve contacts 
            std::vector<std::pair<void*, void*>> rawPairs;
            BroadPhase.QueryOverlaps(rawPairs);

            std::vector<ContactManifold> manifolds;
            std::vector<std::pair<int,int>> pairBodies;
            manifolds.reserve(rawPairs.size());

            for (auto& pr : rawPairs) 
            {
                const std::uint32_t ia = (std::uint32_t)(std::uintptr_t)pr.first;
                const std::uint32_t ib = (std::uint32_t)(std::uintptr_t)pr.second;

                auto& A     = Colliders[ia];
                auto& B     = Colliders[ib];
                auto& BA    = Bodies[A.Body.Index];
                auto& BB    = Bodies[B.Body.Index];

                if (BA.InvMass == 0.0f && BB.InvMass == 0.0f) continue;
                if (BA.Sleeping && BB.Sleeping) continue;

                ContactManifold m{};
                if (Collide(A.Coll, BodyWorldMatrix(A.Body), B.Coll, BodyWorldMatrix(B.Body), NPContext, m) && m.Count > 0) 
                {
                    manifolds.push_back(m);
                    pairBodies.push_back({ (int)A.Body.Index, (int)B.Body.Index });

                    if (BA.Sleeping && (BB.InvMass>0.0f || !BB.Sleeping)) BA.WakeUp();
                    if (BB.Sleeping && (BA.InvMass>0.0f || !BA.Sleeping)) BB.WakeUp();
                }
            }

            if (!manifolds.empty()) 
            {
                ContactBatch batch;
                batch.Build(manifolds, pairBodies, Bodies, SolveSettings);
                if (SolveSettings.WarmStart) batch.WarmStart(Bodies, SolveSettings);
                batch.Solve(Bodies, SolveSettings);
            }

            if (!FixedJoints.empty() || !BallJoints.empty() || !HingsJoints.empty() || !SliderJoints.empty())
            {
                JointBatch jbatch;
                for (auto& jd : FixedJoints)    jbatch.AddFixed(jd);
                for (auto& jd : BallJoints)     jbatch.AddBall (jd);
                for (auto& jd : HingsJoints)    jbatch.AddHinge(jd);
                for (auto& jd : SliderJoints)   jbatch.AddSlider(jd);
                if (JSettings.WarmStart) jbatch.WarmStart(Bodies, JSettings);
                jbatch.Solve(Bodies, JSettings, WSettings.DeltaTime);
            }

            // 6) Integrate using each body’s *remaining* dt
            for (auto& b : Bodies) 
            {
                if (b.InvMass > 0.0f && !b.Sleeping) b.Integrate(dt);
            }

            // 7) Sleep test (unchanged)
            for (auto& b : Bodies) 
            {
                if (b.InvMass == 0.0f || !b.CanSleep) continue;
                if (b.Sleeping) continue;

                const float vLin = glm::length(b.LinearVelocity);
                const float vAng = glm::length(b.AngularVelocity);

                if (vLin < b.SleepLinearThreshold && vAng < b.SleepAngularThreshold) 
                {
                    b.SleepTimer += dt;
                    if (b.SleepTimer >= b.SleepTime) b.PutToSleep();
                } 
                else 
                {
                    b.SleepTimer = 0.0f; 
                }
            }
        }
#pragma endregion

        glm::mat4 BodyWorldMatrix(BodyHandle h) const 
        {
            const RigidBody& b = Bodies[h.Index];
            return glm::translate(glm::mat4(1.0f), b.Position) * glm::toMat4(b.Rotation);
        }

        static AABB ComputeWorldAABB(const Shape& s, const glm::mat4& M) 
        {
            switch (s.Type) 
            {
                case ShapeType::Sphere: 
                {
                    const auto& S = static_cast<const SphereShape&>(s);
                    return SphereShape::WorldAABB(S, M);
                }
                case ShapeType::Box: 
                {
                    const auto& B = static_cast<const BoxShape&>(s);
                    return BoxShape::WorldAABB(M, B.HalfExtents, B.ConvexRadius);
                }
                case ShapeType::Capsule: 
                {
                    const auto& K = static_cast<const CapsuleShape&>(s);
                    return CapsuleShape::WorldAABB(K, M);
                }
                case ShapeType::Convex:
                {
                    const auto& H = static_cast<const ConvexHullShape&>(s);
                    return ConvexHullShape::WorldAABB(H, M);
                }
            }

            return { {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f} };
        }

        static AABB FattenAABB(const AABB& a, const glm::vec3& extra) 
        {
            return { a.MIN - extra, a.MAX + extra };
        }

        bool RaycastFirst(const Ray& ray, RayHit& outHit) const 
        {
            outHit = RayHit{};
            bool any = false;

            for (size_t i = 0; i < Colliders.size();++i) 
            {
                const auto& wc = Colliders[i];

                const auto& proxy = BroadPhase.Proxies[wc.Proxy - 1]; 
                float tEnter, tExit;
                if (!RayAABB(ray, proxy.Body, tEnter, tExit)) continue;

                RayHit h; h.T = outHit.T;
                glm::mat4 W = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                if (RaycastCollider(ray, wc.Coll, W, h)) 
                {
                    h.HitCollder = &wc.Coll;
                    if (h.T < outHit.T) { outHit = h; any = true; }
                }
            }

            return any;
        }

        void RaycastAll(const Ray& ray, std::vector<RayHit>& outHits) const 
        {
            outHits.clear();
            for (size_t i = 0;i < Colliders.size();++i) 
            {
                const auto& wc      = Colliders[i];
                const auto& proxy   = BroadPhase.Proxies[wc.Proxy - 1];
                float tEnter, tExit;
                if (!RayAABB(ray, proxy.Body, tEnter, tExit)) continue;

                RayHit h; h.T = std::numeric_limits<float>::infinity();
                glm::mat4 W = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                if (RaycastCollider(ray, wc.Coll, W, h)) 
                {
                    h.HitCollder = &wc.Coll;
                    outHits.push_back(h);
                }
            }
            
            std::sort(outHits.begin(), outHits.end(), [](const RayHit& a, const RayHit& b){ return a.T < b.T; });
        }



    };
}