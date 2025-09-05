#include "CorePCH.hpp"

namespace Motion
{
    void PhyX::Setp(const std::vector<std::shared_ptr<Entity>>& e, float deltaTime)
    {
        IntegrateForces(e, deltaTime);
        Broadphase(e);
        Narrowphase();
        SolveContacts(deltaTime);
        IntegrateVelocities(e, deltaTime);
        ClearAccumulators(e);
    }

    void PhyX::IntegrateForces(const std::vector<std::shared_ptr<Entity>>& e, float deltaTime)
    {
        for(auto& entity : e)
        {
            if (entity->HasComponent<RigidBodyComponent>() && entity->HasComponent<TransformComponent>())
            {
                auto& TR = entity->GetComponent<TransformComponent>();
                auto& RB = entity->GetComponent<RigidBodyComponent>();
                if (RB.InvMass == 0.0f || RB.Sleeping) continue;

                // Gravity as force
                RB.ForceAccum += (RB.Mass * m_Gravity);

                // a = F * invMass
                const glm::vec3 accel = RB.ForceAccum * RB.InvMass;
                RB.Velocity += accel * deltaTime;
                RB.Velocity *= std::max(0.0f, 1.0f - RB.LinearDamping * deltaTime);

                // Angular: τ = I * α  =>  α = I^-1_world * τ
                if (RB.InvInertiaDiag != glm::vec3(0.0f)) 
                {
                    const glm::mat3 IinvW = InvInertiaWorld(RB, TR.Rotation);
                    const glm::vec3 alpha = IinvW * RB.TorqueAccum;

                    RB.AngularVelocity += alpha * deltaTime;
                    RB.AngularVelocity *= std::max(0.0f, 1.0f - RB.AngularDamping * deltaTime);
                }
            }
        }
    }

    void PhyX::Broadphase(const std::vector<std::shared_ptr<Entity>>& e)
    {
        m_Pairs.clear();

        // 1) Refresh world AABBs
        for(auto& entity : e)
        {
            auto& TR    = entity->GetComponent<TransformComponent>();
            auto& COL   = entity->GetComponent<ColliderComponent>();
            if(COL.Type == ColliderType::None) continue;
            switch (COL.Type) 
            {
                case ColliderType::Sphere:  COL.WorldAABB = MakeWorldAABB(TR, COL); break;
                case ColliderType::Box:     COL.WorldAABB = MakeWorldAABB(TR, COL); break;
                case ColliderType::Capsule: COL.WorldAABB = CapsuleWorldAABB(TR, COL); break; // <-- NEW
                default: break;
            }
        }

        // 2) Build spans along X
        struct SpanX
        {
            std::shared_ptr<Entity> e;
            AABB aabb;
            float minX, maxX;
        };

        std::vector<SpanX> spans;
        spans.reserve(e.size());

        for(auto& entity : e)
        {
            const auto& col = entity->GetComponent<ColliderComponent>();
            if(col.Type == ColliderType::None) continue;
            const auto& aabb = col.WorldAABB;
            spans.push_back(SpanX{entity, aabb, aabb.MIN.x, aabb.MAX.x});
        }

        // 3) Sort by minX
        std::sort(spans.begin(), spans.end(),
              [](const SpanX& a, const SpanX& b){ return a.minX < b.minX; });

        // 4) Sweep active list
        std::vector<SpanX> active;
        active.reserve(spans.size());

        for (const auto& s : spans)
        {
            // Remove spans that end before this begins
            for (size_t i = 0; i < active.size();)
            {
                if (active[i].maxX < s.minX) 
                {
                    active[i] = active.back();
                    active.pop_back();
                } 
                else 
                {
                    ++i;
                }
            }

            // Test against active for YZ overlap, skip static-static
            for (const auto& a : active)
            {
                const auto aEnt = a.e;
                const auto bEnt = s.e;

                const auto& rbA = aEnt->GetComponent<RigidBodyComponent>();
                const auto& rbB = bEnt->GetComponent<RigidBodyComponent>();
                if (rbA.InvMass == 0.0f && rbB.InvMass == 0.0f) continue;

                // Fast Y/Z overlap test
                const bool overY = (a.aabb.MIN.y <= s.aabb.MAX.y) && (a.aabb.MAX.y >= s.aabb.MIN.y);
                const bool overZ = (a.aabb.MIN.z <= s.aabb.MAX.z) && (a.aabb.MAX.z >= s.aabb.MIN.z);
                if (!overY || !overZ) continue;

                // Canonicalize order to avoid duplicates
                std::shared_ptr<Entity> A = aEnt, B = bEnt;
                if (static_cast<uint32_t>(B->GetHandle()) < static_cast<uint32_t>(A->GetHandle())) std::swap(A, B);
                m_Pairs.emplace_back(A, B);
            }

            // Add current span to active
            active.push_back(s);
        }
    }

    static bool NarrowSphere(const std::shared_ptr<Entity>& a, const std::shared_ptr<Entity>& b, Contact& out)
    {
        auto& ta = a->GetComponent<TransformComponent>();
        auto& tb = b->GetComponent<TransformComponent>();
        auto& ca = a->GetComponent<ColliderComponent>();
        auto& cb = b->GetComponent<ColliderComponent>();

        const glm::vec3 pa = ta.Translation;
        const glm::vec3 pb = tb.Translation;

        const float ra = ca.Sphere.Radius; // * scale if needed
        const float rb = cb.Sphere.Radius;

        glm::vec3 ab    = pb - pa;
        float dist2     = glm::dot(ab, ab);
        float rsum      = ra + rb;

        if (dist2 >= rsum * rsum) return false;

        float dist      = glm::sqrt(glm::max(dist2, 1e-12f));
        glm::vec3 n     = (dist > 1e-6f) ? (ab / dist) : glm::vec3(0,1,0);  // arbitrary when centers coincide
        float pen       = rsum - dist;
        glm::vec3 p     = pa + n * (ra - 0.5f * pen);                       // approx mid point on overlap

        out = { a, b, n, p, pen };
        return true;
    }

    static bool NarrowAABB(const std::shared_ptr<Entity>& a, const std::shared_ptr<Entity>& b, Contact& out)
    {
        auto& ca = a->GetComponent<ColliderComponent>().WorldAABB;
        auto& cb = b->GetComponent<ColliderComponent>().WorldAABB;

        // Compute overlap on each axis
        float dx1 = cb.MAX.x - ca.MIN.x; // b right - a left
        float dx2 = ca.MAX.x - cb.MIN.x; // a right - b left
        float dy1 = cb.MAX.y - ca.MIN.y;
        float dy2 = ca.MAX.y - cb.MIN.y;
        float dz1 = cb.MAX.z - ca.MIN.z;
        float dz2 = ca.MAX.z - cb.MIN.z;

        if (dx1 <= 0 || dx2 <= 0 || dy1 <= 0 || dy2 <= 0 || dz1 <= 0 || dz2 <= 0) return false;

        // pick the smallest penetration axis
        float px = std::min(dx1, dx2);
        float py = std::min(dy1, dy2);
        float pz = std::min(dz1, dz2);

        glm::vec3 n{0.0f};
        float pen = 0.0f;

        if (px < py && px < pz) { n = (dx1 < dx2) ? glm::vec3(+1,0,0) : glm::vec3(-1,0,0); pen = px; }
        else if (py < pz)       { n = (dy1 < dy2) ? glm::vec3(0,+1,0) : glm::vec3(0,-1,0); pen = py; }
        else                    { n = (dz1 < dz2) ? glm::vec3(0,0,+1) : glm::vec3(0,0,-1); pen = pz; }

        // contact point: center between closest faces (rough)
        auto& ta        = a->GetComponent<TransformComponent>();
        auto& tb        = b->GetComponent<TransformComponent>();
        glm::vec3 p     = 0.5f * (ta.Translation + tb.Translation);

        out = { a, b, n, p, pen };
        return true;
    }

    static bool NarrowSphereBox(const std::shared_ptr<Entity>& a, const std::shared_ptr<Entity>& b, Contact& out)
    {
        auto& ta = a->GetComponent<TransformComponent>();
        auto& tb = b->GetComponent<TransformComponent>();
        auto& ca = a->GetComponent<ColliderComponent>();
        auto& cb = b->GetComponent<ColliderComponent>();

        const glm::vec3 center  = ta.Translation;
        const float radius      = ca.Sphere.Radius;

        const AABB& box = cb.WorldAABB;
        const glm::vec3 q = ClosestPointOnAABB(center, box);

        const glm::vec3 d   = q - center;
        const float dist2   = glm::dot(d, d);
        if(dist2 > radius * radius) return false;

        const float dist = glm::sqrt(glm::max(dist2, 1e-12f));
        glm::vec3 n = (dist > 1e-6f) ? (d / dist) : glm::vec3(0, 1, 0);   // from sphere -> box
        const float penetration = radius - dist;

        // Contact point: on sphere surface toward box
        const glm::vec3 point = center + n * (radius - 0.5f * penetration);

        out = { a, b, n, point, penetration };
        return true;
    }

    inline bool NarrowCapsuleSphere(const std::shared_ptr<Entity>& capEnt, const std::shared_ptr<Entity>& sphEnt, Contact& out)
    {
        const auto& trC = capEnt->GetComponent<TransformComponent>();
        const auto& trS = sphEnt->GetComponent<TransformComponent>();
        const auto& cC  = capEnt->GetComponent<ColliderComponent>();
        const auto& cS  = sphEnt->GetComponent<ColliderComponent>();

        glm::vec3 a, b; CapsuleWorldEnds(trC, cC, a, b);
        const glm::vec3 center  = trS.Translation;
        const float rC          = cC.Capsule.Radius;
        const float rS          = cS.Sphere.Radius;

        // Closest point on capsule segment to sphere center
        const glm::vec3 q = ClosestPointOnSegment(center, a, b);
        const glm::vec3 d = center - q;
        const float dist2 = glm::dot(d, d);
        const float rsum = rC + rS;

        if (dist2 > rsum * rsum) return false;

        const float dist = std::sqrt(std::max(dist2, 1e-12f));
        const glm::vec3 n = (dist > 1e-6f) ? (d / dist) : glm::vec3(0,1,0); // from capsule -> sphere
        const float pen = rsum - dist;
        const glm::vec3 p = center - n * (rS - 0.5f * pen); // point roughly at contact

        out = { capEnt, sphEnt, n, p, pen };
        return true;
    }

    inline bool NarrowCapsuleBox(const std::shared_ptr<Entity>& capEnt, const std::shared_ptr<Entity>& boxEnt, Contact& out)
    {
        using namespace Motion;
        const auto& trC = capEnt->GetComponent<TransformComponent>();
        const auto& trB = boxEnt->GetComponent<TransformComponent>();
        const auto& cC  = capEnt->GetComponent<ColliderComponent>();
        const auto& cB  = boxEnt->GetComponent<ColliderComponent>();

        glm::vec3 a, b; CapsuleWorldEnds(trC, cC, a, b);

        // Approximation: project the segment endpoints into the AABB's closest points,
        // then find closest point on segment to that projection (works well for axis-aligned box).
        // 1) Find representative point on box closest to the segment
        //    We test by sampling the closest to endpoints and to the segment midpoint.
        glm::vec3 mid = 0.5f * (a + b);

        const glm::vec3 qb0 = ClosestPointOnAABB(a,   cB.WorldAABB);
        const glm::vec3 qb1 = ClosestPointOnAABB(b,   cB.WorldAABB);
        const glm::vec3 qbM = ClosestPointOnAABB(mid, cB.WorldAABB);

        // 2) For each qb*, get closest point on the segment, keep the best
        auto candidate = [&](const glm::vec3& qbox) {
            const glm::vec3 qc = ClosestPointOnSegment(qbox, a, b);
            const float d2 = glm::length2(qbox - qc);
            return std::pair<float, glm::vec3>(d2, qc);
        };

        auto c0 = candidate(qb0);
        auto c1 = candidate(qb1);
        auto cM = candidate(qbM);

        auto best = c0;
        if (c1.first < best.first) best = c1;
        if (cM.first < best.first) best = cM;

        const glm::vec3 qc = best.second;                         // closest point on capsule segment
        const glm::vec3 qbox = ClosestPointOnAABB(qc, cB.WorldAABB); // refine: closest on box to segment point

        const glm::vec3 d = qbox - qc;       // from capsule surface point toward box
        const float dist2 = glm::dot(d, d);
        const float r = cC.Capsule.Radius;

        if (dist2 > r * r) return false;

        const float dist = std::sqrt(std::max(dist2, 1e-12f));
        const glm::vec3 n = (dist > 1e-6f) ? (d / dist) : glm::vec3(0,1,0); // from capsule -> box
        const float pen = r - dist;

        // Contact point roughly between surfaces
        const glm::vec3 p = qc + n * (r - 0.5f * pen);

        out = { capEnt, boxEnt, n, p, pen };
        return true;
    }


    void PhyX::Narrowphase()
    {
        m_Contacts.clear();

        auto HasAllComponents = [&](const std::shared_ptr<Entity>& e) -> bool
        {
            if(e->HasComponent<TransformComponent>() && e->HasComponent<ColliderComponent>() && e->HasComponent<RigidBodyComponent>())
                return true;
            else
                return false;
        };

        for(auto [a, b] : m_Pairs)
        {
            if(!HasAllComponents(a) || !HasAllComponents(b)) continue;

            auto& ca = a->GetComponent<ColliderComponent>();
            auto& cb = b->GetComponent<ColliderComponent>();

            Contact c{};
            bool HIT{false};

            if(ca.Type == ColliderType::Sphere && cb.Type == ColliderType::Sphere)
                HIT = NarrowSphere(a, b, c);
            
            if(ca.Type == ColliderType::Box && cb.Type == ColliderType::Box);
                HIT = NarrowAABB(a, b, c);

            if(ca.Type == ColliderType::Box && cb.Type == ColliderType::Sphere)
                HIT = NarrowSphereBox(a, b, c);

            if(ca.Type == ColliderType::Sphere && cb.Type == ColliderType::Box)
            {
                Contact temp{};
                HIT = NarrowSphereBox(b, a, temp);
                if(HIT) { temp.Normal = -temp.Normal; std::swap(temp.A, temp.B); c = temp; }
            }

            if (ca.Type == ColliderType::Capsule && cb.Type == ColliderType::Sphere) 
            {
                HIT = NarrowCapsuleSphere(a, b, c);
            }

            if (ca.Type == ColliderType::Sphere && cb.Type == ColliderType::Capsule) 
            {
                Contact tmp{};
                HIT = NarrowCapsuleSphere(b, a, tmp); // capsule first
                if (HIT) { std::swap(tmp.A, tmp.B); tmp.Normal = -tmp.Normal; c = tmp; }
            }

            if (ca.Type == ColliderType::Capsule && cb.Type == ColliderType::Box) 
            {
                HIT = NarrowCapsuleBox(a, b, c);
            }

            if (ca.Type == ColliderType::Box && cb.Type == ColliderType::Capsule) 
            {
                Contact tmp{};
                HIT = NarrowCapsuleBox(b, a, tmp); // capsule first
                if (HIT) { std::swap(tmp.A, tmp.B); tmp.Normal = -tmp.Normal; c = tmp; }
            }

            if(HIT) m_Contacts.push_back(c);
        }
    }

    void PhyX::SolveContacts(float deltaTime)
    {
        if(m_Contacts.empty()) return;

        static constexpr std::int32_t iterations    = 10;
        static constexpr float restitution          = 0.2f;        // bounce
        static constexpr float frictionStatic       = 0.60f;       // static friction coeff
        static constexpr float frictionDynamic      = 0.45f;       // dynamic friction coeff
        static constexpr float penetrationSlop      = 0.005f;
        static constexpr float baumgarteBeta        = 0.2f;

        for (int it = 0; it < iterations; ++it)
        {
            for (auto& c : m_Contacts)
            {
                auto& rbA = c.A->GetComponent<RigidBodyComponent>();
                auto& rbB = c.B->GetComponent<RigidBodyComponent>();
                auto& trA = c.A->GetComponent<TransformComponent>();
                auto& trB = c.B->GetComponent<TransformComponent>();

                const auto& colA = c.A->GetComponent<ColliderComponent>();
                const auto& colB = c.B->GetComponent<ColliderComponent>();

                const float invMassA = rbA.InvMass;
                const float invMassB = rbB.InvMass;
                if (invMassA + invMassB == 0.0f)  continue;
                if (rbA.Sleeping && rbB.Sleeping) continue;

                const PairMaterial mat = MakePairMaterial(colA, colB);

                // r vectors from COM to contact point
                const glm::vec3 rA = c.Point - trA.Translation;
                const glm::vec3 rB = c.Point - trB.Translation;

                // World inverse inertia
                const glm::mat3 IinvA = InvInertiaWorld(rbA, trA.Rotation);
                const glm::mat3 IinvB = InvInertiaWorld(rbB, trB.Rotation);

                // Relative velocity at contact
                const glm::vec3 vA = rbA.Velocity + glm::cross(rbA.AngularVelocity, rA);
                const glm::vec3 vB = rbB.Velocity + glm::cross(rbB.AngularVelocity, rB);
                const glm::vec3 rv = vB - vA;

                // --- Normal impulse ---
                const float vn = glm::dot(rv, c.Normal);

                const glm::vec3 raXn    = glm::cross(rA, c.Normal);
                const glm::vec3 rbXn    = glm::cross(rB, c.Normal);
                const float angA        = glm::dot(glm::cross(IinvA * raXn, rA), c.Normal);
                const float angB        = glm::dot(glm::cross(IinvB * rbXn, rB), c.Normal);
                const float denomN      = invMassA + invMassB + angA + angB;

                float jn = 0.0f;
                if (vn <= 0.0f && denomN > 0.0f) 
                {
                    jn = -(1.0f + mat.Restitution) * vn / denomN;
                    const glm::vec3 impulseN = jn * c.Normal;

                    rbA.Velocity          -= impulseN * invMassA;
                    rbB.Velocity          += impulseN * invMassB;
                    rbA.AngularVelocity   -= IinvA * glm::cross(rA, impulseN);
                    rbB.AngularVelocity   += IinvB * glm::cross(rB, impulseN);

                    if (glm::length2(impulseN) > 0.0f) 
                    {
                        rbA.Sleeping    = rbB.Sleeping      = false;
                        rbA.SleepTimer  = rbB.SleepTimer    = 0.0f;
                    }
                }

                // --- Friction (Coulomb) ---
                const glm::vec3 vA2 = rbA.Velocity + glm::cross(rbA.AngularVelocity, rA);
                const glm::vec3 vB2 = rbB.Velocity + glm::cross(rbB.AngularVelocity, rB);
                const glm::vec3 rv2 = vB2 - vA2;

                glm::vec3 t         = rv2 - glm::dot(rv2, c.Normal) * c.Normal;
                const float tlen    = glm::length(t);
                if (tlen > 1e-6f) t /= tlen; else t = glm::vec3(0.0f);

                if (t != glm::vec3(0.0f)) {
                    const glm::vec3 raXt = glm::cross(rA, t);
                    const glm::vec3 rbXt = glm::cross(rB, t);
                    const float angAt = glm::dot(glm::cross(IinvA * raXt, rA), t);
                    const float angBt = glm::dot(glm::cross(IinvB * rbXt, rB), t);
                    const float denomT = invMassA + invMassB + angAt + angBt;

                    if (denomT > 0.0f) 
                    {
                        const float vt = glm::dot(rv2, t);
                        const float jt = -vt / denomT;

                        const float maxStatic = mat.MU_S * std::fabs(jn);
                        float jtClamped = std::clamp(jt, -maxStatic, maxStatic);
                        if (std::fabs(jt) > maxStatic) 
                        {
                            jtClamped = -mat.MU_D * std::fabs(jn) * (vt >= 0.0f ? 1.0f : -1.0f);
                        }

                        const glm::vec3 impulseT = jtClamped * t;

                        rbA.Velocity        -= impulseT * invMassA;
                        rbB.Velocity        += impulseT * invMassB;
                        rbA.AngularVelocity -= IinvA * glm::cross(rA, impulseT);
                        rbB.AngularVelocity += IinvB * glm::cross(rB, impulseT);
                    }
                }

                // --- Positional correction (normal only) ---
                const float corrMag = std::max(c.Penetration - penetrationSlop, 0.0f) * baumgarteBeta;
                if (corrMag > 0.0f) 
                {
                    const glm::vec3 correction  = corrMag * c.Normal;
                    const float sumInv          = invMassA + invMassB;
                    if (sumInv > 0.0f) 
                    {
                        trA.Translation -= correction * (invMassA / sumInv);
                        trB.Translation += correction * (invMassB / sumInv);
                    }
                }
            }
        }
    }
        
    void PhyX::IntegrateVelocities(const std::vector<std::shared_ptr<Entity>>& e, float deltaTime)
    {
        static constexpr float sleepVelThresh    = 0.05f;
        static constexpr float sleepTimeRequired = 0.05f;

        for(auto& entity : e)
        {
            if(entity->HasComponent<TransformComponent>() && entity->HasComponent<RigidBodyComponent>())
            {
                auto& TR = entity->GetComponent<TransformComponent>();
                auto& RB = entity->GetComponent<RigidBodyComponent>();
                if(RB.InvMass == 0.0f) return;

                TR.Translation += RB.Velocity * deltaTime;

                // Angular integration (quaternion): q_dot = 0.5 * ω_quat * q
                if (RB.AngularVelocity != glm::vec3(0.0f)) 
                {
                    const glm::quat wq(0.0f, RB.AngularVelocity.x, RB.AngularVelocity.y, RB.AngularVelocity.z);
                    glm::quat q = TR.Rotation;
                    q += 0.5f * (wq * q) * deltaTime;
                    TR.Rotation = glm::normalize(q);
                }

                // Sleeping heuristic
                const float v2          = glm::length2(RB.Velocity);
                const float w2          = glm::length2(RB.AngularVelocity);
                const float thresh2     = sleepVelThresh * sleepVelThresh;
                if (v2 < thresh2 && w2 < (0.25f * thresh2)) 
                {
                    RB.SleepTimer += deltaTime;
                    if (RB.SleepTimer >= sleepTimeRequired) 
                    {
                        RB.Sleeping         = true;
                        RB.Velocity         = glm::vec3(0.0f);
                        RB.AngularVelocity  = glm::vec3(0.0f);
                    }
                } 
                else 
                {
                    RB.SleepTimer   = 0.0f;
                    RB.Sleeping     = false;
                }
            }
        }
    }

    void PhyX::ClearAccumulators(const std::vector<std::shared_ptr<Entity>>& e)
    {
        for(auto& entity : e)
        {
            if(entity->HasComponent<RigidBodyComponent>())
            {
                auto& RB        = entity->GetComponent<RigidBodyComponent>();
                RB.ForceAccum   = glm::vec3(0.0f);
                RB.TorqueAccum  = glm::vec3(0.0f);
            }
        }
    }

}