#include "CorePCH.hpp"
#include "KinetiX.hpp"

namespace Motion 
{
    static constexpr float kFixedStep = 1.0f / 120.0f;
    static float accumulator = 0.0f;

    void KinetiX::Init() 
    {
        MOTION_ASSERT(m_World == nullptr, "Physics world already initialized");

        m_Settings.gravity = rp3d::Vector3(0.0f, -Motion::Units::SI_GRAVITY, 0.0f);
        m_Settings.defaultVelocitySolverNbIterations = 20;
        m_Settings.isSleepingEnabled = true;

        m_World = m_Common.createPhysicsWorld(m_Settings);
        MOTION_ASSERT(m_World, "Failed to create physics world");
        m_World->setNbIterationsVelocitySolver(15); 
        m_World->setNbIterationsPositionSolver(6);   

        m_World->setIsDebugRenderingEnabled(true);
        auto& dr = m_World->getDebugRenderer();
        dr.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
        dr.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
    }

    void KinetiX::DestroyAll() 
    {
        for(std::uint32_t i = 0; i < m_World->getNbRigidBodies(); ++i)
        {
            rp3d::RigidBody* body = m_World->getRigidBody(i);
            for(std::uint32_t k = 0; k < body->getNbColliders(); ++k)
            {
                rp3d::Collider* collider = body->getCollider(k);
                body->removeCollider(collider);
            }
            m_World->destroyRigidBody(body);
        }

        for(auto& [_, res] : m_ConvexCache) 
        {
            if(res.shape) m_Common.destroyConvexMeshShape(res.shape);
            if(res.mesh) m_Common.destroyConvexMesh(res.mesh);
        }
    }

    void KinetiX::Reset() 
    {
        MOTION_ASSERT(m_World != nullptr, "World not initialized");
        DestroyAll();
        if (m_World) { m_Common.destroyPhysicsWorld(m_World); m_World = nullptr; }
        Init();
    }

    void KinetiX::Quit() 
    {
        DestroyAll();
        if (m_World) m_Common.destroyPhysicsWorld(m_World);
        m_World = nullptr;
    }

    void KinetiX::CreateRigidBody(const std::shared_ptr<Entity>& e) 
    {
        MOTION_ASSERT(m_World, "Physics world not initialized");

        auto& tc  = e->Get<TransformComponent>();
        if (!e->Has<RigidBodyComponent>()) e->Emplace<RigidBodyComponent>();
        auto& rbc = e->Get<RigidBodyComponent>();

        rp3d::RigidBody* rb = static_cast<rp3d::RigidBody*>(rbc.PhysicsBody);
        if (!rb) { rb = m_World->createRigidBody(Transform(tc)); rbc.PhysicsBody = rb; }

        rb->setType(GetBodyType(rbc.Type));
        rb->setIsDebugEnabled(true);
        rb->enableGravity(true);
        rb->setIsAllowedToSleep(true);
        rb->setIsActive(true);  

        const auto linLock  = rp3d::Vector3(
            rbc.LockX ? 0.0f : 1.0f,
            rbc.LockY ? 0.0f : 1.0f,
            rbc.LockZ ? 0.0f : 1.0f
        );

        const auto angLock  = rp3d::Vector3(
            rbc.LockRotX ? 0.0f : 1.0f,
            rbc.LockRotY ? 0.0f : 1.0f,
            rbc.LockRotZ ? 0.0f : 1.0f
        );

        rb->setLinearLockAxisFactor(linLock);
        rb->setAngularLockAxisFactor(angLock);
        rb->setAngularDamping(rbc.AngularDamping);
        rb->setLinearDamping(rbc.LinearDamping);
        rb->setUserData(e.get());
    }

    void KinetiX::DestroyRigidBody(const std::shared_ptr<Entity>& e) 
    {
        if (!e->Has<RigidBodyComponent>()) return;
        auto& rbc = e->Get<RigidBodyComponent>();
        auto* rb  = static_cast<rp3d::RigidBody*>(rbc.PhysicsBody);
        if (!rb) return;

        if (e->Has<ColliderComponent>()) 
        {
            auto& cc = e->Get<ColliderComponent>();

            if (cc.Collider)
            {
                rb->removeCollider(static_cast<rp3d::Collider*>(cc.Collider));
                cc.Collider = nullptr;
            }

            if (cc.Shape)
            {
                if (cc.Type == ShapeType::Box)      m_Common.destroyBoxShape(dynamic_cast<rp3d::BoxShape*>(cc.Shape));
                if (cc.Type == ShapeType::Sphere)   m_Common.destroySphereShape(dynamic_cast<rp3d::SphereShape*>(cc.Shape));
                if (cc.Type == ShapeType::Capsule)  m_Common.destroyCapsuleShape(dynamic_cast<rp3d::CapsuleShape*>(cc.Shape));
                if (cc.Type == ShapeType::Convex)   m_Common.destroyConvexMeshShape(dynamic_cast<rp3d::ConvexMeshShape*>(cc.Shape));
                if (cc.Type == ShapeType::Concave)  m_Common.destroyConcaveMeshShape(dynamic_cast<rp3d::ConcaveMeshShape*>(cc.Shape));

                cc.Shape = nullptr;
            }
        }

        m_World->destroyRigidBody(rb);
        rbc.PhysicsBody = nullptr;
    }

    void KinetiX::ChangeCollider(const std::shared_ptr<Entity>& e, ShapeType type)
    {
        auto& tr    = e->Get<TransformComponent>();
        auto& mc    = e->Get<MeshComponent>();
        auto& rbc   = e->Get<RigidBodyComponent>();
        auto& cc    = e->Get<ColliderComponent>();
        if(cc.Type  == type) return;
        
        auto* rb = rbc.PhysicsBody;
        if(cc.Collider) rb->removeCollider(cc.Collider);

        if(cc.Shape)
        {
            if(cc.Type == ShapeType::Box)       m_Common.destroyBoxShape(dynamic_cast<rp3d::BoxShape*>(cc.Shape));
            if(cc.Type == ShapeType::Sphere)    m_Common.destroySphereShape(dynamic_cast<rp3d::SphereShape*>(cc.Shape));
            if(cc.Type == ShapeType::Capsule)   m_Common.destroyCapsuleShape(dynamic_cast<rp3d::CapsuleShape*>(cc.Shape));
            if(cc.Type == ShapeType::Convex)    m_Common.destroyConvexMeshShape(dynamic_cast<rp3d::ConvexMeshShape*>(cc.Shape));
            if(cc.Type == ShapeType::Concave)   m_Common.destroyConcaveMeshShape(dynamic_cast<rp3d::ConcaveMeshShape*>(cc.Shape));
        }

        cc.Shape        = nullptr;
        if(type == ShapeType::Box)      CreateBoxCollider(e);
        if(type == ShapeType::Sphere)   CreateSphereCollider(e);
        if(type == ShapeType::Capsule)  CreateCapsuleCollider(e);
        if(type == ShapeType::Convex)   CreateConvexCollider(e, mc.MeshPointer->Positions, mc.MeshPointer->Faces);
        //if(type == ShapeType::Concave)  CreateConcaveCollider(e, mc.MeshPointer->Positions, mc.MeshPointer->Faces);
    }


    void KinetiX::CreateBoxCollider(const std::shared_ptr<Entity>& e) 
    {
        auto& RBC   = e->Get<RigidBodyComponent>();
        auto& CC    = e->Get<ColliderComponent>();
        auto& TRC   = e->Get<TransformComponent>();

        auto* RB    = RBC.PhysicsBody;
        RB->setTransform(Transform(TRC));

        auto* shape         = m_Common.createBoxShape(ToVec3(CC.BoxHalfExtents));
        auto* collider      = RB->addCollider(shape, rp3d::Transform::identity());
        rp3d::Material& mat = collider->getMaterial();

        collider->setIsSimulationCollider(true);
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape            = shape;
        CC.Collider         = collider;
        CC.Type             = ShapeType::Box;

        if (RB->getType() == rp3d::BodyType::DYNAMIC)
            RB->updateMassPropertiesFromColliders();        
    }

    void KinetiX::CreateSphereCollider(const std::shared_ptr<Entity>& e) 
    {
        auto& RBC   = e->Get<RigidBodyComponent>();
        auto& CC    = e->Get<ColliderComponent>();
        auto& TRC   = e->Get<TransformComponent>();
        auto* RB    = RBC.PhysicsBody;
        RB->setTransform(Transform(TRC));

        const glm::vec3 S       = TRC.Scale;
        const float s           = (S.x + S.y + S.z) / 3.0f;
        const float r           = std::max(0.0f, CC.SphereRadius * s);
        auto* shape             = m_Common.createSphereShape(r);
        auto* collider          = RB->addCollider(shape, rp3d::Transform::identity());
        rp3d::Material& mat     = collider->getMaterial();

        collider->setIsSimulationCollider(true);
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape        = shape;
        CC.Collider     = collider;
        CC.Type         = ShapeType::Sphere;
        CC.SphereRadius = r;

        if (RB->getType() == rp3d::BodyType::DYNAMIC)
            RB->updateMassPropertiesFromColliders();
    }

    void KinetiX::CreateCapsuleCollider(const std::shared_ptr<Entity>& e) 
    {
        auto& RBC = e->Get<RigidBodyComponent>();
        auto& CC  = e->Get<ColliderComponent>();
        auto& TRC = e->Get<TransformComponent>();

        auto* RB  = RBC.PhysicsBody;
        RB->setTransform(Transform(TRC));

        const glm::vec3 S           = TRC.Scale;
        const std::int32_t axis     = CC.Capsule.Axis;

        float sx = S.x, sy = S.y, sz = S.z;
        float rScale = 1.0f, hScale = 1.0f;

        if(axis == 0) { rScale = std::max(sy, sz); hScale = sx; }
        else if(axis == 1) { rScale = std::max(sx, sz); hScale = sy; }
        else { rScale = std::max(sx, sy); hScale = sz; }

        const float r = std::max(0.0f, CC.Capsule.Radius * rScale);
        const float h = std::max(0.0f, CC.Capsule.Height * hScale);

        auto* shape             = m_Common.createCapsuleShape(r, h);
        auto* collider          = RB->addCollider(shape, rp3d::Transform::identity());
        rp3d::Material& mat     = collider->getMaterial();

        collider->setIsSimulationCollider(true);
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape            = shape;
        CC.Collider         = collider;
        CC.Type             = ShapeType::Capsule;
        CC.Capsule.Radius   = r;
        CC.Capsule.Height   = h;
        CC.Capsule.Axis     = axis;

        if (RB->getType() == rp3d::BodyType::DYNAMIC)
            RB->updateMassPropertiesFromColliders();
    }

    void KinetiX::CreateConvexCollider(const std::shared_ptr<Entity>& e, const std::vector<glm::vec3>& inVertices, const std::vector<uint32_t>&  inIndices)
    {
        const std::uint32_t simplifyTarget = 256;
        const float dedupEps = 1e-6f;
        const rp3d::Vector3 scaling(1,1,1);
        const float degeneracyEps = 1e-6f;
        
        std::vector<glm::vec3> vertices;
        vertices.reserve(inVertices.size());
        vertices.assign(inVertices.begin(), inVertices.end());

        if(simplifyTarget > 0 && vertices.size() > simplifyTarget)
        {
            Simplify(vertices, simplifyTarget);
            MOTION_CORE_INFO("Simplified convex mesh to {} vertices", vertices.size());
        }

        std::vector<std::uint32_t> oldToNew{};
        RemoveDuplicates(vertices, dedupEps, oldToNew);
        MOTION_CORE_INFO("Removed duplicates, {} vertices remain", vertices.size());

        if(vertices.size() < 4)
        {
            MOTION_CORE_WARN("Convex mesh has less than 4 unique vertices, cannot create convex collider");
            return;
        }

        HullBuildResult hull{};
        ComputeConvexHull(m_Common, vertices, hull, scaling, degeneracyEps);

        if(!hull.ok())
        {
            for(const auto& msg : hull.log)
                MOTION_CORE_WARN(msg);

            MOTION_CORE_ERROR("Failed to compute convex hull");
            return;
        }

        auto* shape = hull.shape;
        auto& RBC = e->Get<RigidBodyComponent>();
        auto& CC  = e->Get<ColliderComponent>();
        auto* RB  = RBC.PhysicsBody;

        auto* collider = RB->addCollider(shape, rp3d::Transform::identity());
        rp3d::Material& mat = collider->getMaterial();

        collider->setIsSimulationCollider(true);
        mat.setFrictionCoefficient(CC.Friction);
        mat.setBounciness(CC.Restitution);
        mat.setMassDensity(CC.MassDensity);

        CC.Shape            = shape;
        CC.Collider         = collider;
        CC.Type             = ShapeType::Convex;
    }

    void KinetiX::CreateConcaveCollider(const std::shared_ptr<Entity>& e, const std::vector<glm::vec3>& inVertices, const std::vector<uint32_t>& inIndices)
    {
        MOTION_ASSERT(false, "Not implemented yet");
    }

    void KinetiX::Refresh(const std::vector<std::shared_ptr<Entity>>& entities)
    {
        for(const auto& e : entities)
        {
            std::shared_ptr<Entity> current = e;
            while(current)
            {
                std::shared_ptr<Entity> next = nullptr;
                if(current->Has<NodeComponent>())
                {
                    next = current->Get<NodeComponent>().EnTTNext;
                    if(current->Get<NodeComponent>().IsRoot)
                    {
                        current = next;
                        continue;
                    }
                }

                auto& TRC = current->Get<TransformComponent>();
                auto& RBC = current->Get<RigidBodyComponent>();
                auto& CC  = current->Get<ColliderComponent>();

                rp3d::RigidBody* body = RBC.PhysicsBody;
                if(!body || !CC.Collider || !CC.Shape) { current = next; continue; }

                const auto type                 = body->getType();
                const bool isDynamic            = (type == rp3d::BodyType::DYNAMIC);
                const bool isKinematicOrStatic  = !isDynamic;

                body->setTransform(Transform(TRC));

                const glm::vec3 S = TRC.Scale;
                if(S != CC.LastAppliedScale)
                {
                    rp3d::CollisionShape* newShape = nullptr;
                    switch(CC.Type)
                    {
                        case ShapeType::Box:
                        {
                            auto* boxShape = dynamic_cast<rp3d::BoxShape*>(CC.Shape);
                            const glm::vec3 H
                            (
                                CC.BoxHalfExtents.x * S.x,
                                CC.BoxHalfExtents.y * S.y,
                                CC.BoxHalfExtents.z * S.z
                            );

                            if(boxShape) boxShape->setHalfExtents(ToVec3(H));
                            break;
                        }
                        case ShapeType::Sphere:
                        {
                            auto* sphereShape   = dynamic_cast<rp3d::SphereShape*>(CC.Shape);
                            const float s       = (S.x + S.y + S.z) / 3.0f;
                            const float r       = std::max(0.0f, CC.SphereRadius * s);
                            if(sphereShape) sphereShape->setRadius(r);
                            break;
                        }
                        case ShapeType::Capsule:
                        {
                            auto* capsuleShape = dynamic_cast<rp3d::CapsuleShape*>(CC.Shape);
                            const std::int32_t axis = CC.Capsule.Axis;
                            float sx = S.x, sy = S.y, sz = S.z;
                            float rScale = 1.0f, hScale = 1.0f;

                            if(axis == 0) 
                            { 
                                rScale = std::max(sy, sz); 
                                hScale = sx; 
                            }
                            else if(axis == 1) 
                            { 
                                rScale = std::max(sx, sz); 
                                hScale = sy; 
                            }
                            else 
                            { 
                                rScale = std::max(sx, sy); 
                                hScale = sz; 
                            }

                            const float radius  = capsuleShape->getRadius() * rScale;
                            const float height  = capsuleShape->getHeight() * hScale;
                            const float r = std::max(0.0f, radius);
                            const float h = std::max(0.0f, height);

                            CC.Capsule.Radius   = r;
                            CC.Capsule.Height   = h;
                            CC.Capsule.Axis     = axis;

                            capsuleShape->setRadius(r);
                            capsuleShape->setHeight(h);

                            break;
                        }
                        case ShapeType::Convex:
                        {
                            auto* convexShape = dynamic_cast<rp3d::ConvexMeshShape*>(CC.Shape);
                            if(convexShape) convexShape->setScale(ToVec3(S));
                            break;
                        }
                        case ShapeType::Concave:
                        {
                            MOTION_ASSERT(false, "Not implemented yet");
                            break;
                        }

                        default: break;
                    }
                }

                if(body->getType() == rp3d::BodyType::DYNAMIC)
                    body->updateMassPropertiesFromColliders();

                CC.LastAppliedScale = S;
                current = next;
            }
        }
    }

    void KinetiX::Step(const std::vector<std::shared_ptr<Entity>>& entities, float dtSeconds) 
    {
        try
        {
            MOTION_ASSERT(m_World, "World not initialized");
            dtSeconds = std::clamp(dtSeconds, 0.0f, 0.1f);
            accumulator = std::min(accumulator + dtSeconds, 0.25f);
            
            int steps = 0;
            constexpr int kMaxStepsPerFrame = 8;
            while (accumulator >= kFixedStep && steps < kMaxStepsPerFrame) 
            {
                m_World->update(kFixedStep);
                accumulator -= kFixedStep;
                ++steps;
            }

            // 3) Sync transforms for dynamics
            for (auto& e : entities) 
            {
                for (auto current = e; current; ) 
                {
                    std::shared_ptr<Entity> next = nullptr;
                    if (current->Has<NodeComponent>()) next = current->Get<NodeComponent>().EnTTNext;

                    if (current->Has<RigidBodyComponent>()) 
                    {
                        auto& rb = current->Get<RigidBodyComponent>();
                        if (rb.Type == BodyType::Dynamic && rb.PhysicsBody) 
                        {
                            auto& tc = current->Get<TransformComponent>();
                            Transform(rb.PhysicsBody->getTransform(), tc);
                        }
                    }
                    current = next;
                }
            }
        }
        catch(const std::exception& e)
        {
            MOTION_ASSERT(false, e.what());
        }
    }

    struct RaycastCB : public rp3d::RaycastCallback 
    {
        bool hasHit = false;
        rp3d::decimal bestFraction = rp3d::decimal(1);
        rp3d::Vector3 bestPoint{0,0,0};
        rp3d::Vector3 bestNormal{0,0,0};
        const rp3d::Collider* bestCollider = nullptr;
        const rp3d::Body*     bestBody     = nullptr;

        rp3d::decimal notifyRaycastHit(const rp3d::RaycastInfo& info) override
        {
            if (info.hitFraction < bestFraction) 
            {
                hasHit       = true;
                bestFraction = info.hitFraction;
                bestPoint    = info.worldPoint;
                bestNormal   = info.worldNormal;
                bestCollider = info.collider;
                bestBody     = info.body;
            }

            return info.hitFraction; 
        }
    };

    bool KinetiX::RaycastFirstHit(const glm::vec3& from, const glm::vec3& to, glm::vec3* hitPointWorld, glm::vec3* hitNormalWorld) const 
    {
        if (!m_World) return false;

        RaycastCB cb;
        rp3d::Ray ray(ToVec3(from), ToVec3(to));
        m_World->raycast(ray, &cb);

        if (!cb.hasHit) return false;

        if (hitPointWorld)  *hitPointWorld  = ToVec3(cb.bestPoint);
        if (hitNormalWorld) *hitNormalWorld = ToVec3(cb.bestNormal);
        
        return true;
    }

} 
