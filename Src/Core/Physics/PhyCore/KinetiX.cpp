#include "CorePCH.hpp"

namespace Motion
{
    struct UnionFind 
    {
        std::vector<std::uint32_t> Parent{}, Rank{};

        void Reset(std::uint32_t n) 
        { 
            Parent.resize(n); 
            Rank.assign(n,0); 
            for (std::uint32_t i = 0; i < n; ++i) Parent[i] = i; 
        }

        std::uint32_t Find(std::uint32_t x) 
        { 
            return Parent[x]==x? x : Parent[x]=Find(Parent[x]); 
        }

        void Union(std::uint32_t a, std::uint32_t b) 
        {
            a = Find(a); b = Find(b); if (a==b) return;
            if (Rank[a]<Rank[b]) std::swap(a,b);
            Parent[b]=a; if (Rank[a]==Rank[b]) ++Rank[a];
        }
    };

    struct Island 
    {
        std::vector<std::uint32_t> BodyIdx;          
        std::vector<std::uint32_t> PairIdx;          

        std::vector<std::uint32_t> FixedIdx;
        std::vector<std::uint32_t> BallIdx;
        std::vector<std::uint32_t> HingeIdx;
        std::vector<std::uint32_t> SliderIdx;

        bool HasDynamic{false};
    };


    static AABB FattenAABB(const AABB& a, const glm::vec3& extra) 
    {
        return { a.MIN - extra, a.MAX + extra };
    }

    static AABB SweptAABB(const glm::vec3& o, const glm::vec3& dir, float tMax, float r) 
    {
        glm::vec3 a     = o;
        glm::vec3 b     = o + dir * tMax;
        glm::vec3 mn    = glm::min(a, b) - glm::vec3(r);
        glm::vec3 mx    = glm::max(a, b) + glm::vec3(r);

        return { mn, mx };
    }




    void KinetiX::SetCanSleep(BodyHandle h, bool can) 
    {
        Bodies[h.Index].CanSleep = can;
        if (!can) Bodies[h.Index].WakeUp();
    }

    void KinetiX::WakeBody(BodyHandle h) 
    { 
        Bodies[h.Index].WakeUp(); 
    }
 
    bool KinetiX::IsSleeping(BodyHandle h) const
    {
        return Bodies[h.Index].Sleeping;
    } 

    bool KinetiX::IsSleeping(BodyHandle h) const 
    { 
        return Bodies[h.Index].Sleeping; 
    }

    void KinetiX::ApplyForce(BodyHandle h, const glm::vec3& F, float dt) 
    {
        auto& b = Bodies[h.Index];
        if (b.InvMass == 0.0f) return;
        b.WakeUp();
        b.LinearVelocity += F * (b.InvMass * (Config.UseFixedDeltaTime) ? Config.FixedDeltaTime : dt);
    }

    void KinetiX::ApplyImpulse(BodyHandle h, const glm::vec3& P, const glm::vec3& rWS = glm::vec3(0)) 
    {
        auto& b = Bodies[h.Index];
        if (b.InvMass == 0.0f) return;
        b.WakeUp();
        b.ApplyLinearImpulse(P);
        if (glm::length2(rWS) > 0) b.ApplyAngularImpulse(glm::cross(rWS, P));
    }

    BodyHandle KinetiX::CreateRigidBody(const MassProperties& massProp, const glm::vec3& pos, const glm::quat& rot, bool staticObj = false)
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

    const RigidBody& KinetiX::GetRigidBody(BodyHandle h)
    {
        return Bodies[h.Index];
    }

    ColliderHandle KinetiX::CreateCollider(BodyHandle bh, const Collider& c) 
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

    const WorldCollider& KinetiX::GetCollider(ColliderHandle ch)
    {
        return Colliders[ch.Index];
    }

    std::size_t KinetiX::CreateFixedJoint(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local)
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

    std::size_t KinetiX::CreateBallSocket(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, float baumgarte = 0.2f)
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

    std::size_t KinetiX::CreateHinge(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, const glm::vec3& axisA_local, const glm::vec3& axisB_local, bool enableLimits=false, float lo=-1.0f, float hi=+1.0f, bool enableMotor=false, float motorSpeed=0.0f, float motorTorque=0.0f)
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

    std::size_t KinetiX::CreateSlider(BodyHandle a, BodyHandle b, const glm::vec3& anchorA_local, const glm::vec3& anchorB_local, const glm::vec3& axisA_local, const glm::vec3& axisB_local, glm::vec3 refPerpA_local, glm::vec3 refPerpB_local, bool enableLimits=false, float lo=-1.0f, float hi=+1.0f, bool enableMotor=false, float motorSpeed=0.0f, float motorForce=0.0f)
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


    bool KinetiX::SweepSphereFirst(const SweepSphere& q, SweepHit& outHit) const 
    {
        outHit      = SweepHit{};
        bool any    = false;
        AABB swept  = SweptAABB(q.Origin, q.Dir, q.TMax, q.R);

        for (size_t i = 0; i < Colliders.size(); ++i) 
        {
            const auto& wc = Colliders[i];
            const auto& proxy = BroadPhase.Proxies[wc.Proxy - 1]; 
            if (!Overlap(swept, proxy.Body)) continue;

            SweepHit h; h.T = outHit.T;
            glm::mat4 W     = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
            if (SweepSphereAgainstCollider(q, wc.Coll, W, h)) 
            {
                h.HitCollider = &wc.Coll;
                if (h.T < outHit.T) { outHit = h; any = true; }
            }
        }

        return any;
    }

    bool KinetiX::SweepWorldSphereFirst(const SweepSphere& q, SweepHit& outHit)
    {
        outHit      = SweepHit{};
        bool any    = false;
        AABB swept  = SweptAABB(q.Origin, q.Dir, q.TMax, q.R);

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

    static AABB SweptCapsuleAABB(const glm::vec3& A0, const glm::vec3& B0, const glm::vec3& dir, float tMax, float r) 
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

    bool KinetiX::SweepCapsuleFirst(const SweepCapsule& q, SweepHit& outHit) 
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

    bool KinetiX::SweepWorldCapsuleFirst(const SweepCapsule& q, SweepHit& outHit)
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


    glm::mat4 KinetiX::BodyWorldMatrix(BodyHandle h) const 
    {
        const RigidBody& b = Bodies[h.Index];
        return glm::translate(glm::mat4(1.0f), b.Position) * glm::toMat4(b.Rotation);
    }

    bool KinetiX::RaycastFirst(const Ray& ray, RayHit& outHit) const 
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

    void KinetiX::RaycastAll(const Ray& ray, std::vector<RayHit>& outHits) const 
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

    void KinetiX::Step(float deltaTime)
    {
        const float dt = (Config.UseFixedDeltaTime) ? Config.FixedDeltaTime : deltaTime;

        // 0) Gravity
        for (auto& b : Bodies)
            if (b.InvMass > 0.0f && !b.Sleeping)
                b.LinearVelocity += Config.Gravity * dt;

        // 1) Inertia
        for (auto& b : Bodies) b.SyncInertiaWorld();

        // 1.5) CCD (sphere/capsule) — compute per-body remaining dt
        std::vector<float> remainDt(Bodies.size(), dt);

        auto BodyCCDRadius = [&](std::size_t bi) -> float
        {
            float r = 0.0f; bool first = true; AABB uni{};
            for (const auto& wc : Colliders)
            {
                if (wc.Body.Index != bi) continue;
                const glm::mat4 M = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
                const AABB a = ComputeWorldAABB(*wc.Coll.ColliderShape, M);
                if (first) { uni = a; first = false; }
                else { uni.MIN = glm::min(uni.MIN, a.MIN); uni.MAX = glm::max(uni.MAX, a.MAX); }
            }
            if (!first) r = 0.5f * glm::length(uni.MAX - uni.MIN);
            return r;
        };

        const float ccdRatio = 0.5f;

        for (std::size_t bi = 0; bi < Bodies.size(); ++bi)
        {
            auto& B = Bodies[bi];
            if (B.InvMass == 0.0f || B.Sleeping) continue;

            const glm::vec3 motion = B.LinearVelocity * dt;
            const float dist = glm::length(motion);
            if (dist <= 1e-6f) continue;

            // Prefer capsule CCD if a capsule is present on this body
            const Collider* capCol = nullptr;
            for (const auto& wc : Colliders)
                if (wc.Body.Index == bi && wc.Coll.ColliderShape && wc.Coll.ColliderShape->Type == ShapeType::Capsule)
                { capCol = &wc.Coll; break; }

            if (capCol)
            {
                const auto& K = *static_cast<const CapsuleShape*>(capCol->ColliderShape);
                glm::mat4 W = BodyWorldMatrix({ (uint32_t)bi }) * capCol->LocalPose;

                glm::vec3 A0, B0; float rCap;
                // FIX: correct signature (no K param)
                CapsuleShape::WorldSegment(K, W, A0, B0, rCap);
                rCap += K.ConvexRadius;

                SweepCapsule q;
                q.A0 = A0; q.B0 = B0; q.R = rCap;
                q.Dir = motion / dist; q.TMax = dist;

                SweepHit hit;
                if (SweepWorldCapsuleFirst(q, hit))
                {
                    const float slop = 1e-4f;
                    const float toi = glm::max(0.0f, hit.T - slop);
                    B.Position += q.Dir * toi;

                    const float vn = glm::dot(B.LinearVelocity, hit.Normal);
                    if (vn < 0.0f)
                    {
                        const float e = 0.1f;
                        const glm::vec3 vT = B.LinearVelocity - vn * hit.Normal;
                        B.LinearVelocity = vT - e * vn * hit.Normal;
                    }

                    const float usedDt = toi / (dist / dt);
                    remainDt[bi] = glm::max(0.0f, dt - usedDt);
                }
            }
            else
            {
                const float radius = BodyCCDRadius(bi);
                if (radius <= 0.0f || dist < ccdRatio * radius) continue;

                SweepSphere q;
                q.Origin = B.Position; q.Dir = motion / dist; q.R = radius; q.TMax = dist;

                SweepHit hit;
                if (SweepWorldSphereFirst(q, hit))
                {
                    const float slop = 1e-4f;
                    const float toi = glm::max(0.0f, hit.T - slop);
                    B.Position += q.Dir * toi;

                    const float vn = glm::dot(B.LinearVelocity, hit.Normal);
                    if (vn < 0.0f)
                    {
                        const float e = 0.1f;
                        const glm::vec3 vT = B.LinearVelocity - vn * hit.Normal;
                        B.LinearVelocity = vT - e * vn * hit.Normal;
                    }

                    const float usedDt = toi / (dist / dt);
                    remainDt[bi] = glm::max(0.0f, dt - usedDt);
                }
            }
        }

        // 2) Update proxies at (possibly advanced) positions
        for (size_t ci = 0; ci < Colliders.size(); ++ci)
        {
            auto& wc = Colliders[ci];
            const auto& B = Bodies[wc.Body.Index];
            const glm::mat4 M = BodyWorldMatrix(wc.Body) * wc.Coll.LocalPose;
            const AABB aabb = ComputeWorldAABB(*wc.Coll.ColliderShape, M);

            glm::vec3 velPad(0);
            if (!B.Sleeping) velPad = glm::abs(B.LinearVelocity) * dt * Config.FatAABBVelocityPad;
            BroadPhase.MoveProxy(wc.Proxy, FattenAABB(aabb, velPad));
        }

        // 3) Broadphase → raw overlaps
        std::vector<std::pair<void*, void*>> rawPairs;
        BroadPhase.QueryOverlaps(rawPairs);

        // 4) Narrow-phase
        std::vector<ContactManifold> manifolds;
        std::vector<std::pair<int, int>> pairBodies;
        manifolds.reserve(rawPairs.size());

        for (auto& pr : rawPairs)
        {
            const std::uint32_t ia = (std::uint32_t)(std::uintptr_t)pr.first;
            const std::uint32_t ib = (std::uint32_t)(std::uintptr_t)pr.second;

            auto& A = Colliders[ia];
            auto& B = Colliders[ib];
            auto& BA = Bodies[A.Body.Index];
            auto& BB = Bodies[B.Body.Index];

            if (BA.InvMass == 0.0f && BB.InvMass == 0.0f) continue;
            if (BA.Sleeping && BB.Sleeping) continue;

            // NEW: Layer/Mask gate (Filter==Layer)
            const std::uint32_t layerA = A.Coll.Filter, layerB = B.Coll.Filter;
            const std::uint32_t maskA  = A.Coll.Mask,   maskB  = B.Coll.Mask;
            if (((layerA & maskB) == 0u) || ((layerB & maskA) == 0u)) continue;

            ContactManifold m{};
            if (Collide(A.Coll, BodyWorldMatrix(A.Body), B.Coll, BodyWorldMatrix(B.Body), NPContext, m) && m.Count > 0)
            {
                manifolds.push_back(m);
                pairBodies.push_back({ (int)A.Body.Index, (int)B.Body.Index });

                if (BA.Sleeping && (BB.InvMass > 0.0f || !BB.Sleeping)) BA.WakeUp();
                if (BB.Sleeping && (BA.InvMass > 0.0f || !BA.Sleeping)) BB.WakeUp();
            }
        }

        // 4.5) Contact persistence (refresh cache)
        PersistSettings pset{};
        for (size_t i = 0; i < manifolds.size(); ++i)
        {
            const auto ab = pairBodies[i];
            PairKey key{ (std::uint32_t)std::min(ab.first, ab.second),
                        (std::uint32_t)std::max(ab.first, ab.second) };

            auto it = ContactCache.find(key);
            if (it == ContactCache.end()) it = ContactCache.emplace(key, PersistentManifold{}).first;

            RefreshPersistent(manifolds[i], pset, it->second);
            it->second.LastTouched = ++FrameCounter;
        }

        // 5) Build islands (contacts + joints)
        UnionFind uf; uf.Reset((std::uint32_t)Bodies.size());

        for (const auto& ab : pairBodies)
        {
            const std::uint32_t ia = (std::uint32_t)ab.first;
            const std::uint32_t ib = (std::uint32_t)ab.second;
            if (Bodies[ia].InvMass > 0.0f || Bodies[ib].InvMass > 0.0f) uf.Union(ia, ib);
        }

        auto unionByJoint = [&](const auto& J)
        {
            for (std::uint32_t i = 0; i < (std::uint32_t)J.size(); ++i)
            {
                const std::int32_t a = J[i].A;
                const std::int32_t b = J[i].B;
                if (Bodies[a].InvMass > 0.0f || Bodies[b].InvMass > 0.0f) uf.Union(a, b);
            }
        };
        unionByJoint(FixedJoints);
        unionByJoint(BallJoints);
        unionByJoint(HingsJoints);
        unionByJoint(SliderJoints);

        std::unordered_map<std::uint32_t, std::uint32_t> rootToIsland;
        std::vector<Island> islands;

        for (std::uint32_t bi = 0; bi < (std::uint32_t)Bodies.size(); ++bi)
        {
            const std::uint32_t r = uf.Find(bi);
            auto it = rootToIsland.find(r);
            if (it == rootToIsland.end())
            {
                std::uint32_t idx = (std::uint32_t)islands.size();
                rootToIsland.emplace(r, idx);
                islands.push_back(Island{});
                it = rootToIsland.find(r);
            }
            islands[it->second].BodyIdx.push_back(bi);
            if (Bodies[bi].InvMass > 0.0f && !Bodies[bi].Sleeping) islands[it->second].HasDynamic = true;
        }

        for (std::uint32_t i = 0; i < (std::uint32_t)pairBodies.size(); ++i)
        {
            const auto [a, b] = pairBodies[i];
            const std::uint32_t isl = rootToIsland[uf.Find((std::uint32_t)a)];
            islands[isl].PairIdx.push_back(i);
        }

        auto collectJ = [&](const auto& J, auto vecField)
        {
            for (std::uint32_t i = 0; i < (std::uint32_t)J.size(); ++i)
            {
                const auto a = J[i].A;
                const std::uint32_t isl = rootToIsland[uf.Find(a)];
                vecField(islands[isl]).push_back(i);
            }
        };
        
        collectJ(FixedJoints,  [](Island& is) -> auto& { return is.FixedIdx;  });
        collectJ(BallJoints,   [](Island& is) -> auto& { return is.BallIdx;   });
        collectJ(HingsJoints,  [](Island& is) -> auto& { return is.HingeIdx;  });
        collectJ(SliderJoints, [](Island& is) -> auto& { return is.SliderIdx; });

        // 6) Solve per-island
        for (auto& isl : islands)
        {
            if (!isl.HasDynamic) continue;

            std::vector<ContactManifold> imans;     imans.reserve(isl.PairIdx.size());
            std::vector<std::pair<int, int>> ipairs; ipairs.reserve(isl.PairIdx.size());
            for (auto pi : isl.PairIdx) { imans.push_back(manifolds[pi]); ipairs.push_back(pairBodies[pi]); }

            if (!imans.empty())
            {
                ContactBatch cb;
                cb.Build(imans, ipairs, Bodies, SolveSettings, &ContactCache);
                if (SolveSettings.WarmStart) cb.WarmStart(Bodies, SolveSettings);
                cb.Solve(Bodies, SolveSettings);
                cb.WriteBackToCache();
            }

            if (!isl.FixedIdx.empty() || !isl.BallIdx.empty() || !isl.HingeIdx.empty() || !isl.SliderIdx.empty())
            {
                JointBatch jb;
                for (auto idx : isl.FixedIdx)  jb.AddFixed (FixedJoints[idx]);
                for (auto idx : isl.BallIdx)   jb.AddBall  (BallJoints[idx]);
                for (auto idx : isl.HingeIdx)  jb.AddHinge (HingsJoints[idx]);
                for (auto idx : isl.SliderIdx) jb.AddSlider(SliderJoints[idx]);

                if (JSettings.WarmStart) jb.WarmStart(Bodies, JSettings);
                jb.Solve(Bodies, JSettings, dt);
            }
        }

        // 7) Island-based sleep test
        for (auto& isl : islands)
        {
            if (!isl.HasDynamic) continue;
            bool canSleep = true;
            for (auto bi : isl.BodyIdx)
            {
                const auto& b = Bodies[bi];
                if (b.InvMass == 0.0f || !b.CanSleep) continue;
                if (glm::length(b.LinearVelocity) > b.SleepLinearThreshold ||
                    glm::length(b.AngularVelocity) > b.SleepAngularThreshold) { canSleep = false; break; }
            }
            if (canSleep)
            {
                for (auto bi : isl.BodyIdx)
                    if (Bodies[bi].InvMass > 0.0f && Bodies[bi].CanSleep)
                    {
                        auto& b = Bodies[bi];
                        b.SleepTimer += dt;
                        if (b.SleepTimer >= b.SleepTime) b.PutToSleep();
                    }
            }
            else
            {
                for (auto bi : isl.BodyIdx) Bodies[bi].SleepTimer = 0.0f;
            }
        }

        // 8) Integrate using each body’s remaining dt
        for (std::size_t bi = 0; bi < Bodies.size(); ++bi)
        {
            auto& b = Bodies[bi];
            if (b.InvMass > 0.0f && !b.Sleeping) b.Integrate(remainDt[bi]);
        }

        // 9) Final per-body sleep check (keep if you want extra hysteresis)
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
            else b.SleepTimer = 0.0f;
        }
    }

}