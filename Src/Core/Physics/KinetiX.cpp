#include "CorePCH.hpp"
#include "KinetiX.hpp"

namespace Motion 
{
    static constexpr float kFixedStep = 1.0f / 120.0f;

    void KinetiX::Init() 
    {
        MOTION_ASSERT(m_World == nullptr, "Physics world already initialized");

        m_Settings.gravity = rp3d::Vector3(0.0f, -Motion::Units::g_mps2, 0.0f);
        m_Settings.defaultVelocitySolverNbIterations = 20;
        m_Settings.isSleepingEnabled = true;

        m_World = m_Common.createPhysicsWorld(m_Settings);
        MOTION_ASSERT(m_World, "Failed to create physics world");
        m_World->setNbIterationsVelocitySolver(15);
        m_World->setNbIterationsPositionSolver(8);

        m_Logger = m_Common.createDefaultLogger();
        std::uint32_t logLevel = static_cast<std::uint32_t>(static_cast<std::uint32_t>(rp3d::Logger::Level::Warning) | static_cast<std::uint32_t>(rp3d::Logger::Level::Error));
        m_Logger->addStreamDestination(std::cout, logLevel, rp3d::DefaultLogger::Format::Text);
        m_Logger->addFileDestination("MotionPhysicsLogs.txt", logLevel, rp3d::DefaultLogger::Format::Text);
        m_Common.setLogger(m_Logger);

        m_World->setIsDebugRenderingEnabled(true);
        auto& dr = m_World->getDebugRenderer();
        dr.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
        dr.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
    }

    void KinetiX::DestroyAll() 
    {
        for (auto& p : m_ConvexMeshes)   m_Common.destroyConvexMesh(p.second);
        for (auto& p : m_TriangleMeshes) m_Common.destroyTriangleMesh(p.second);

        m_ConvexMeshes.clear();
        m_TriangleMeshes.clear();

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
    }

    void KinetiX::DestroyCachedMeshesFor(Entity* key)
    {
        if (auto it = m_ConvexMeshes.find(key); it != m_ConvexMeshes.end())
        {
            if (it->second) m_Common.destroyConvexMesh(it->second);
            m_ConvexMeshes.erase(it);
        }

        if (auto it2 = m_TriangleMeshes.find(key); it2 != m_TriangleMeshes.end())
        {
            if (it2->second) m_Common.destroyTriangleMesh(it2->second);
            m_TriangleMeshes.erase(it2);
        }
    }

    void KinetiX::ApplyDefaultMaterial(rp3d::Collider* collider) 
    {
        if (!collider) return;

        rp3d::Material mat = collider->getMaterial();
        mat.setFrictionCoefficient(0.5f);
        mat.setBounciness(0.1f);
        mat.setMassDensity(1.0f);

        collider->setMaterial(mat);
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
        MOTION_ASSERT(e && e->IsAlive(), "Invalid entity");

        auto& tc  = e->GetComponent<TransformComponent>();
        if (!e->HasComponent<RigidBodyComponent>()) e->AddComponent<RigidBodyComponent>();
        auto& rbc = e->GetComponent<RigidBodyComponent>();

        rp3d::RigidBody* rb = static_cast<rp3d::RigidBody*>(rbc.PhysicsBody);
        if (!rb) { rb = m_World->createRigidBody(ToRp3dTransform(tc)); rbc.PhysicsBody = rb; }

        rb->setType(ToRp3dBodyType(rbc.Type));
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
        rb->setUserData(e.get());
    }

    void KinetiX::DestroyRigidBody(const std::shared_ptr<Entity>& e) 
    {
        if (!e->HasComponent<RigidBodyComponent>()) return;
        auto& rbc = e->GetComponent<RigidBodyComponent>();
        auto* rb  = static_cast<rp3d::RigidBody*>(rbc.PhysicsBody);
        if (!rb) return;

        if (e->HasComponent<ColliderComponent>()) 
        {
            auto& cc = e->GetComponent<ColliderComponent>();

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
                cc.Attributes = nullptr;
            }
        }

        DestroyCachedMeshesFor(e.get());

        m_World->destroyRigidBody(rb);
        rbc.PhysicsBody = nullptr;
    }

    void KinetiX::ChangeCollider(const std::shared_ptr<Entity>& e, ShapeType type)
    {
        auto& tr    = e->GetComponent<TransformComponent>();
        auto& mc    = e->GetComponent<MeshComponent>();
        auto& rbc   = e->GetComponent<RigidBodyComponent>();
        auto& cc    = e->GetComponent<ColliderComponent>();
        if(cc.Type == type) return;
        
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

        if ((cc.Type == ShapeType::Convex || cc.Type == ShapeType::Concave) && cc.Type != type)
        {
            DestroyCachedMeshesFor(e.get());
        }

        cc.Shape = nullptr;
        cc.Attributes = nullptr;

        if(type == ShapeType::Box)      CreateBoxCollider(e);
        if(type == ShapeType::Sphere)   CreateSphereCollider(e);
        if(type == ShapeType::Capsule)  CreateCapsuleCollider(e);
        if(type == ShapeType::Convex)   CreateConvexCollider(e);
        if(type == ShapeType::Concave)  CreateConcaveCollider(e);
    }

    void KinetiX::CreateBoxCollider(const std::shared_ptr<Entity>& e) 
    {
        auto& rbc = e->GetComponent<RigidBodyComponent>();
        auto& cc  = e->GetComponent<ColliderComponent>();
        auto& tr  = e->GetComponent<TransformComponent>();

        auto* rb  = rbc.PhysicsBody;
        rb->setTransform(ToRp3dTransform(tr));

        auto* shape = m_Common.createBoxShape(ToRp3dVec3(glm::vec3(0.5f * tr.Scale)));
        auto* col   = rb->addCollider(shape, rp3d::Transform::identity());
        col->setIsSimulationCollider(true);
        ApplyDefaultMaterial(col);

        cc.Shape        = shape;
        cc.Collider     = col;
        cc.Type         = ShapeType::Box;
        cc.Attributes   = &col->getMaterial();

        if (rb->getType() == rp3d::BodyType::DYNAMIC)
            rb->updateMassPropertiesFromColliders();        
    }

    void KinetiX::CreateSphereCollider(const std::shared_ptr<Entity>& e) 
    {
        auto& rbc = e->GetComponent<RigidBodyComponent>();
        auto& cc  = e->GetComponent<ColliderComponent>();
        auto& tr  = e->GetComponent<TransformComponent>();
        auto* rb  = rbc.PhysicsBody;
        rb->setTransform(ToRp3dTransform(tr));

        auto* shape = m_Common.createSphereShape(0.5f * glm::max(tr.Scale.x, tr.Scale.y, tr.Scale.z));
        auto* col   = rb->addCollider(shape, rp3d::Transform::identity());
        col->setIsSimulationCollider(true);
        ApplyDefaultMaterial(col);

        cc.Shape        = shape;
        cc.Collider     = col;
        cc.Type         = ShapeType::Sphere;
        cc.Attributes   = &col->getMaterial();

        if (rb->getType() == rp3d::BodyType::DYNAMIC)
            rb->updateMassPropertiesFromColliders();
    }

    void KinetiX::CreateCapsuleCollider(const std::shared_ptr<Entity>& e) 
    {
        auto& rbc = e->GetComponent<RigidBodyComponent>();
        auto& cc  = e->GetComponent<ColliderComponent>();
        auto& tr  = e->GetComponent<TransformComponent>();

        auto* rb  = rbc.PhysicsBody;
        rb->setTransform(ToRp3dTransform(tr));

        auto radius = 0.5f * glm::max(tr.Scale.x, tr.Scale.z);
        auto height = tr.Scale.y - 2 * radius;

        auto* shape = m_Common.createCapsuleShape(radius, height);
        auto* col   = rb->addCollider(shape, rp3d::Transform::identity());
        col->setIsSimulationCollider(true);
        ApplyDefaultMaterial(col);

        cc.Shape        = shape;
        cc.Collider     = col;
        cc.Type         = ShapeType::Capsule;
        cc.Attributes   = &col->getMaterial();

        if (rb->getType() == rp3d::BodyType::DYNAMIC)
            rb->updateMassPropertiesFromColliders();
    }

    void KinetiX::CreateConvexCollider(const std::shared_ptr<Entity>& e) 
    {
        auto& mc  = e->GetComponent<MeshComponent>();
        auto& rbc = e->GetComponent<RigidBodyComponent>();
        auto& cc  = e->GetComponent<ColliderComponent>();
        auto& tr  = e->GetComponent<TransformComponent>();

        auto* rb = rbc.PhysicsBody;
        rb->setTransform(ToRp3dTransform(tr));

        if (!m_ConvexMeshes.contains(e.get())) 
        {
            auto clean = SanitizeConvex(mc.Model->GetPositions(), mc.Model->GetFaces());
            MOTION_ASSERT(clean.vertices.size() >= 4, "Convex mesh needs ≥ 4 unique vertices");
            MOTION_ASSERT(clean.indices.size() >= 3, "No valid triangles after sanitization");

            std::vector<rp3d::PolygonVertexArray::PolygonFace> faceHeaders;
            faceHeaders.reserve(clean.indices.size() / 3);
            for (uint32_t t = 0; t < clean.indices.size() / 3; ++t) 
            {
                rp3d::PolygonVertexArray::PolygonFace pf; pf.indexBase = t * 3; pf.nbVertices = 3; faceHeaders.push_back(pf);
            }

            rp3d::PolygonVertexArray pva(
                (std::uint32_t)clean.vertices.size(), (const void*)clean.vertices.data(), (std::uint32_t)sizeof(glm::vec3),
                (const void*)clean.indices.data(), (std::uint32_t)sizeof(uint32_t),
                (std::uint32_t)faceHeaders.size(), faceHeaders.data(),
                rp3d::PolygonVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
                rp3d::PolygonVertexArray::IndexDataType::INDEX_INTEGER_TYPE
            );

            std::vector<rp3d::Message> messages;
            rp3d::ConvexMesh* cmesh = m_Common.createConvexMesh(pva, messages);
            for (const auto& msg : messages) 
            {
                const char* t = (msg.type==rp3d::Message::Type::Information)?"INFO":(msg.type==rp3d::Message::Type::Warning)?"WARN":"ERROR";
                MOTION_CORE_ERROR("Convex (Triangles) Collider:[{}]: {}", t, msg.text);
            }

            if (!cmesh) 
            {
                MOTION_CORE_WARN("Polygon convex cook failed; falling back to QuickHull on vertices");
                rp3d::VertexArray va((const void*)clean.vertices.data(), (std::uint32_t)sizeof(glm::vec3), (std::uint32_t)clean.vertices.size(), rp3d::VertexArray::DataType::VERTEX_FLOAT_TYPE);
                messages.clear(); cmesh = m_Common.createConvexMesh(va, messages);
                for (const auto& msg : messages) 
                {
                    const char* t = (msg.type==rp3d::Message::Type::Information)?"INFO":(msg.type==rp3d::Message::Type::Warning)?"WARN":"ERROR";
                    MOTION_CORE_ERROR("Convex (QuickHull) Collider:[{}]: {}", t, msg.text);
                }
            }

            MOTION_ASSERT(cmesh != nullptr, "Convex mesh creation failed (polygon and QuickHull)");

            auto* shape     = m_Common.createConvexMeshShape(cmesh, ToRp3dVec3(glm::max(tr.Scale, glm::vec3(1e-3f))));
            MOTION_ASSERT(shape != nullptr, "ConvexMeshShape creation failed");

            auto* col = rb->addCollider(shape, rp3d::Transform::identity()); 
            col->setIsSimulationCollider(true); 
            ApplyDefaultMaterial(col);

            cc.Shape        = shape;
            cc.Collider     = col;
            cc.Attributes   = &col->getMaterial();
            cc.Type         = ShapeType::Convex;

            m_ConvexMeshes[e.get()] = cmesh;
        } 
        else 
        {
            rp3d::ConvexMesh* cmesh     = m_ConvexMeshes[e.get()];
            auto* shape                 = m_Common.createConvexMeshShape(cmesh);
            MOTION_ASSERT(shape != nullptr, "ConvexMeshShape creation failed");

            auto* col = rb->addCollider(shape, rp3d::Transform::identity());
            col->setIsSimulationCollider(true);
            ApplyDefaultMaterial(col);

            cc.Shape        = shape;
            cc.Collider     = col;
            cc.Attributes   = &col->getMaterial();
            cc.Type         = ShapeType::Convex;
        }

        if (rb->getType() == rp3d::BodyType::DYNAMIC)
            rb->updateMassPropertiesFromColliders();                  
    }

    void KinetiX::CreateConcaveCollider(const std::shared_ptr<Entity>& e) 
    {
        auto& mc  = e->GetComponent<MeshComponent>();
        auto& rbc = e->GetComponent<RigidBodyComponent>();
        auto& cc  = e->GetComponent<ColliderComponent>();
        auto& tr  = e->GetComponent<TransformComponent>();

        const std::vector<glm::vec3>& vert = mc.Model->GetPositions();
        const std::vector<uint32_t>& faces = mc.Model->GetFaces();

        auto* rb = rbc.PhysicsBody;
        rb->setTransform(ToRp3dTransform(tr));

        auto clean = SanitizeConcave(vert, faces);
        if (!m_TriangleMeshes.contains(e.get())) 
        {
            MOTION_ASSERT(faces.size() % 3 == 0, "faces must be multiple of 3");
            const std::uint32_t nbVerts = (std::uint32_t)clean.vertices.size();
            const std::uint32_t nbTri   = (std::uint32_t)(clean.indices.size() / 3);

            rp3d::TriangleVertexArray tva(nbVerts, clean.vertices.data(), (std::uint32_t)sizeof(glm::vec3),
                                          nbTri,  clean.indices.data(), (std::uint32_t)( 3 * sizeof(uint32_t)),
                                          rp3d::TriangleVertexArray::VertexDataType::VERTEX_FLOAT_TYPE,
                                          rp3d::TriangleVertexArray::IndexDataType::INDEX_INTEGER_TYPE);

            std::vector<rp3d::Message> messages;
            rp3d::TriangleMesh* tmesh = m_Common.createTriangleMesh(tva, messages);
            for (const auto& msg : messages) 
            {
                const char* t = (msg.type==rp3d::Message::Type::Information)?"INFO":(msg.type==rp3d::Message::Type::Warning)?"WARN":"ERROR";
                MOTION_CORE_ERROR("Concave Collider:[{}]: {}", t, msg.text);
            }
            MOTION_ASSERT(tmesh != nullptr, "Concave mesh creation failed");

            auto safeScale = glm::max(tr.Scale, glm::vec3(1e-3f));
            auto* shape = m_Common.createConcaveMeshShape(tmesh, ToRp3dVec3(glm::max(tr.Scale, glm::vec3(1e-3f))));
            MOTION_ASSERT(shape != nullptr, "ConcaveMeshShape creation failed");

            auto* col = rb->addCollider(shape, rp3d::Transform::identity()); 
            col->setIsSimulationCollider(true);
            ApplyDefaultMaterial(col);

            cc.Shape        = shape;
            cc.Collider     = col;
            cc.Attributes   = &col->getMaterial();
            cc.Type         = ShapeType::Concave;

            m_TriangleMeshes[e.get()] = tmesh;
        } 
        else 
        {
            rp3d::TriangleMesh* tmesh = m_TriangleMeshes[e.get()];
            auto* shape = m_Common.createConcaveMeshShape(tmesh, ToRp3dVec3(glm::max(tr.Scale, glm::vec3(1e-3f))));
            MOTION_ASSERT(shape != nullptr, "ConcaveMeshShape creation failed");
            auto* col = rb->addCollider(shape,rp3d::Transform::identity());
            col->setIsSimulationCollider(true);
            ApplyDefaultMaterial(col);

            cc.Shape        = shape;
            cc.Collider     = col;
            cc.Attributes   = &col->getMaterial();
            cc.Type         = ShapeType::Concave;
        }

        if (rb->getType() != rp3d::BodyType::STATIC) 
        {
            MOTION_CORE_WARN("ConcaveMeshShape attached to non-static body; forcing STATIC.");
            rb->setType(rp3d::BodyType::STATIC);
        }
    }

    void KinetiX::Refresh(const std::vector<std::shared_ptr<Entity>>& entities) 
    {
        for(auto& e : entities)
        {
            if (!e || e == EntityFactory::EMPTYENTITY) continue;
            auto& tr  = e->GetComponent<TransformComponent>();
            auto& rb  = e->GetComponent<RigidBodyComponent>();
            auto& col = e->GetComponent<ColliderComponent>();

            auto* body = rb.PhysicsBody;
            if (!body) continue;
            
            body->setTransform(ToRp3dTransform(tr));

            if(col.Type == ShapeType::Box) 
            {
                auto* shape = dynamic_cast<rp3d::BoxShape*>(col.Shape);
                shape->setHalfExtents(ToRp3dVec3(glm::vec3(0.5f * tr.Scale)));

                if (body->getType() == rp3d::BodyType::DYNAMIC)
                    body->updateMassPropertiesFromColliders();
            }

            if (col.Type == ShapeType::Sphere) 
            {
                auto* shape = dynamic_cast<rp3d::SphereShape*>(col.Shape);
                shape->setRadius(0.5f * glm::max(tr.Scale.x, tr.Scale.y, tr.Scale.z));

                if (body->getType() == rp3d::BodyType::DYNAMIC)
                    body->updateMassPropertiesFromColliders();
            }

            if (col.Type == ShapeType::Capsule) 
            {
                auto* shape = dynamic_cast<rp3d::CapsuleShape*>(col.Shape);
                auto radius = 0.5f * glm::max(tr.Scale.x, tr.Scale.z);
                auto height = tr.Scale.y - 2 * radius;
                shape->setRadius(radius);
                shape->setHeight(height);

                if (body->getType() == rp3d::BodyType::DYNAMIC)
                    body->updateMassPropertiesFromColliders();
            }

            if(col.Type == ShapeType::Convex)
            {
                auto* s = dynamic_cast<rp3d::ConvexMeshShape*>(col.Shape);
                s->setScale(ToRp3dVec3(tr.Scale));

                if (body->getType() == rp3d::BodyType::DYNAMIC)
                    body->updateMassPropertiesFromColliders();
            }

            if (col.Type == ShapeType::Concave) 
            {
                auto* s = dynamic_cast<rp3d::ConcaveMeshShape*>(col.Shape);
                s->setScale(ToRp3dVec3(tr.Scale));
            }
        }
    }

    void KinetiX::Step(const std::vector<std::shared_ptr<Entity>>& entities, float dtSeconds) 
    {
        try
        {
            MOTION_ASSERT(m_World, "World not initialized");
            static float accumulator = 0.0f;
            accumulator += dtSeconds;

            while (accumulator >= kFixedStep) 
            {
                m_World->update(kFixedStep);
                accumulator -= kFixedStep;
            }

            for (auto& e : entities) 
            {
                if (!e || !e->IsAlive()) continue;
                if (!e->HasComponent<RigidBodyComponent>()) continue;

                auto& rbc = e->GetComponent<RigidBodyComponent>();
                if (rbc.Type != BodyType::Dynamic) continue;

                auto* rb = rbc.PhysicsBody;
                if (!rb) continue;

                auto& tc = e->GetComponent<TransformComponent>();
                FromRp3dTransform(rb->getTransform(), tc);
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
        rp3d::Ray ray(ToRp3dVec3(from), ToRp3dVec3(to));
        m_World->raycast(ray, &cb);

        if (!cb.hasHit) return false;

        if (hitPointWorld)  *hitPointWorld  = ToGlmVec3(cb.bestPoint);
        if (hitNormalWorld) *hitNormalWorld = ToGlmVec3(cb.bestNormal);
        
        return true;
    }

} 
