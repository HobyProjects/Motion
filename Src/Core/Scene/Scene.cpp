#include "CorePCH.hpp"
#include "Scene.hpp"

namespace Motion
{
    Scene::Scene(UUID sceneID, const std::string& name, const glm::vec2& viewport)
    {
        m_SceneID   = sceneID;
        m_SceneName = name;

        m_Camera.AspectRatio        = viewport.x / viewport.y;
        m_Camera.ViewportWidth      = viewport.x;
        m_Camera.ViewportHeight     = viewport.y;
        m_Camera.RotationEnabled    = false;
        m_Camera.Position           = glm::vec3(0.0f, 0.0f, 15.0f);
        m_Camera.TranslationSpeed   = 0.01;

        FrameBufferSpecification spec{};
        spec.Name       = name;
        spec.Width      = (std::int32_t)viewport.x;
        spec.Height     = (std::int32_t)viewport.y;
        spec.Samples    = 1;
        m_Framebuffer = IFrameBuffer::Create(spec);

        m_PhySettings.gravity = rp3d::Vector3(0.0f, -SI_GRAVITY, 0.0f);
        m_PhySettings.defaultVelocitySolverNbIterations = 20;
        m_PhySettings.isSleepingEnabled = true;

        m_PhyWorld = m_PhyCommon.createPhysicsWorld(m_PhySettings);
        MOTION_ASSERT(m_PhyWorld, "Failed to create physics world");
        m_PhyWorld->setNbIterationsVelocitySolver(15); 
        m_PhyWorld->setNbIterationsPositionSolver(6);

        m_PhyWorld->setIsDebugRenderingEnabled(true);
        
        auto& dr = m_PhyWorld->getDebugRenderer();
        dr.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
        dr.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::CONTACT_POINT, true);
    
        m_PanelManager.Emplace<SceneViewportPanel>(std::format("{} - Viewport", name));
        m_PanelManager.Emplace<ScenePropertyPanel>(std::format("{} - Properties", name));
    }

    Scene::~Scene() 
    {
        if(SceneSerializer::Serialize(this, std::format("{0}.scene", m_SceneName)))
            MOTION_CORE_INFO("Scene {0} saved", m_SceneName);
        else
            MOTION_CORE_ERROR("Failed to save scene {0}", m_SceneName);

        m_RootEntities.clear();
        if (m_PhyWorld) m_PhyCommon.destroyPhysicsWorld(m_PhyWorld);
        m_PhyWorld = nullptr;
    }

    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        if ((InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED))
        {
            glm::vec3 forward   = glm::normalize(m_Camera.Oriantaion);
            glm::vec3 right     = glm::normalize(glm::cross(forward, m_Camera.WorldUp));
    
            if (InputsHandler::GetKeyState(handle, KEY_W))              m_Camera.Position += forward * m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_S))              m_Camera.Position -= forward * m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_A))              m_Camera.Position -= right * m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_D))              m_Camera.Position += right * m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_LEFT_CONTROL))   m_Camera.Position.y -= m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_SPACE))          m_Camera.Position.y += m_Camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
        }

        if (m_SimulationState == Simulation::RUNNING)
            Steps(deltaTime.GetDeltaTimeSeconds());
        
        RefreshPhysicBodies();  
        m_Camera.RefreshCameraMatrix();
    }

    void Scene::OnEvent(WindowHandle handle, IEvent& e)
    {
        EventHandler handler(handle, e);
        handler.Dispatch<EventMouseCursorMove>(EVENT_CALLBACK(OnMouseCursorPosChange));
        handler.Dispatch<EventMouseWheelScroll>(EVENT_CALLBACK(OnMouseWheelScrollEvent));
    }

    void Scene::OnUIRender(WindowHandle handle)
    {
        ScenePanelContext context{};
        context.FrameTexture        = m_FrameTextureID;
        context.ScenePointer        = this;
        context.SceneRegistry       = &m_SceneRegistry;
        context.PhysicsWorld        = m_PhyWorld;
        context.PhysicsCommon       = &m_PhyCommon;
        context.WorldSettings       = &m_PhySettings;

        for(const auto& pnl : m_PanelManager)
            pnl->RenderUI(context);
    }

    bool Scene::OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e)
    {
        static bool firstMouseMovement = true;
        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            float currentX = e.GetX();
            float currentY = e.GetY();

            if (firstMouseMovement)
            {
                m_MouseX = currentX;
                m_MouseY = currentY;
                firstMouseMovement = false;
                return false; 
            }

            float xOffset = currentX - m_MouseX;
            float yOffset = currentY - m_MouseY;

            m_MouseX = currentX;
            m_MouseY = currentY;

            xOffset *= m_Camera.Sensitivity;
            yOffset *= m_Camera.Sensitivity;

            m_Yaw += xOffset;
            m_Pitch -= yOffset;

            m_Pitch = glm::clamp(m_Pitch, -89.0f, 89.0f);

            glm::vec3 direction;
            direction.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
            direction.y = sin(glm::radians(m_Pitch));
            direction.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));

            m_Camera.Oriantaion = glm::normalize(direction);
            m_Camera.RefreshCameraMatrix();
        }
        else
        {
            firstMouseMovement = true; 
        }

        return false;
    }

    bool Scene::OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e)
    {
        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            m_Camera.PerspectiveFov -= (float)e.OffsetY();
            if (m_Camera.PerspectiveFov < 1.0f)     m_Camera.PerspectiveFov = 1.0f;
            if (m_Camera.PerspectiveFov > 45.0f)    m_Camera.PerspectiveFov = 45.0f;

            m_Camera.RefreshCameraMatrix();
        }

        return false;
    }

    void Scene::Steps(float deltaTime)
    {
        deltaTime = std::clamp(deltaTime, 0.0f, 0.1f);
        ACCUMULATOR = std::min<float>(ACCUMULATOR + deltaTime, 0.25f);

        std::int32_t steps = 0;
        while(ACCUMULATOR >= FIXED_STEPS && steps < FIXED_STEPS_PERFRAME)
        {
            m_PhyWorld->update(FIXED_STEPS);
            ACCUMULATOR -= FIXED_STEPS;
            ++steps;
        }

        ForEachActiveEntity([&](entt::registry& r, entt::entity e)
        {
            auto* rigidBody = m_SceneRegistry.try_get<RigidBodyComponent>(e);
            auto* transform = m_SceneRegistry.try_get<TransformComponent>(e);
            if(!rigidBody || !transform) return;

            if(rigidBody->PhysicsBody && rigidBody->PhysicsBody->getType() == rp3d::BodyType::DYNAMIC)
            {
                Transform(rigidBody->PhysicsBody->getTransform(), *transform);
            }
        });
    }

    void Scene::RefreshPhysicBodies()
    {
        ForEachActiveEntity([&](entt::registry& r, entt::entity e)
        {
            auto* transform = m_SceneRegistry.try_get<TransformComponent>(e);
            auto* rigidBody = m_SceneRegistry.try_get<RigidBodyComponent>(e);
            auto* collider  = m_SceneRegistry.try_get<ColliderComponent>(e);
            if (!transform || !rigidBody || !collider) return;
    
            rp3d::RigidBody* body = rigidBody->PhysicsBody;
            if(!body || !collider->Collider || !collider->Shape) return;

            body->setTransform(Transform(*transform));
    
            const glm::vec3 S = transform->Scale;
            if(S != collider->LastAppliedScale)
            {
                switch(collider->Type)
                {
                    case ShapeType::Box:
                    {
                        auto* boxShape = dynamic_cast<rp3d::BoxShape*>(collider->Shape);
                        const glm::vec3 H
                        (
                            collider->BoxHalfExtents.x * S.x,
                            collider->BoxHalfExtents.y * S.y,
                            collider->BoxHalfExtents.z * S.z
                        );
                        if(boxShape) boxShape->setHalfExtents(ToVec3(H));
                        break;
                    }
                    case ShapeType::Sphere:
                    {
                        auto* sphereShape   = dynamic_cast<rp3d::SphereShape*>(collider->Shape);
                        const float s       = (S.x + S.y + S.z) / 3.0f;
                        const float r       = std::max<float>(0.0f, collider->SphereRadius * s);
                        if(sphereShape) sphereShape->setRadius(r);
                        break;
                    }
                    case ShapeType::Capsule:
                    {
                        auto* capsuleShape = dynamic_cast<rp3d::CapsuleShape*>(collider->Shape);
                        const std::int32_t axis = collider->Capsule.Axis;
                        float sx = S.x, sy = S.y, sz = S.z;
                        float rScale = 1.0f, hScale = 1.0f;

                        if(axis == 0)      { rScale = std::max<float>(sy, sz); hScale = sx; }
                        else if(axis == 1) { rScale = std::max<float>(sx, sz); hScale = sy; }
                        else               { rScale = std::max<float>(sx, sy); hScale = sz; }

                        const float radius  = capsuleShape->getRadius() * rScale;
                        const float height  = capsuleShape->getHeight() * hScale;
                        const float r = std::max<float>(0.0f, radius);
                        const float h = std::max<float>(0.0f, height);

                        collider->Capsule.Radius   = r;
                        collider->Capsule.Height   = h;
                        collider->Capsule.Axis     = axis;

                        capsuleShape->setRadius(r);
                        capsuleShape->setHeight(h);
                        break;
                    }
                    case ShapeType::Convex:
                    {
                        auto* convexShape = dynamic_cast<rp3d::ConvexMeshShape*>(collider->Shape);
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

                collider->LastAppliedScale = S;
            }
    
            if(body->getType() == rp3d::BodyType::DYNAMIC)
                body->updateMassPropertiesFromColliders();
        });
    }

    void Scene::SetApectRatio(const glm::vec2& size)
    {
        m_Camera.SetAspectRatio(size.x, size.y);
        m_Framebuffer->ResizeFrame((std::int32_t)size.x, (std::int32_t)size.y);
    }

    void Scene::Submit() 
    {
        m_Framebuffer->Bind();

        Renderer::SetViewport(0, 0, m_Framebuffer->GetSpecification().Width, m_Framebuffer->GetSpecification().Height);
        Renderer::ClearColor({ 0.243f, 0.243f, 0.243f, 1.0f });
        Renderer::Clear();

        Renderer::Begin();

        const CameraViewProjection cam
        {
            .View           = m_Camera.View,
            .Projection     = m_Camera.Projection,
            .CameraPosition = m_Camera.Position
        };

        const DirectionalLight sun
        {
            .Direction = m_Environment.Sun.Direction,
            .Color     = m_Environment.Sun.Color,
            .Intensity = m_Environment.Sun.Intensity
        };

        ForEachActiveEntity([&](entt::registry& r, entt::entity e)
        {
            const auto* mesh      = m_SceneRegistry.try_get<MeshComponent>(e);
            const auto* material  = m_SceneRegistry.try_get<MaterialComponent>(e);
            const auto* transform = m_SceneRegistry.try_get<TransformComponent>(e);

            if (!(mesh && material && transform)) return;

            RenderCommand cmd{};
            cmd.SortKey         = mesh->ID; 
            cmd.MaterialPointer = material->MaterialPointer.get();
            cmd.MeshPointer     = mesh->MeshPointer.get();

            cmd.CameraData      = cam;

            cmd.ModelData.Model  = transform->GetLocalTransform();
            cmd.ModelData.Normal = glm::transpose(glm::inverse(glm::mat3(cmd.ModelData.Model)));
            cmd.LightData        = sun;

            Renderer::Submit(cmd);
        });

        Renderer::End();

        m_Framebuffer->Unbind();
        m_FrameTextureID =  m_Framebuffer->GetAttachment(FrameBufferColorAttachmentStandards::Standard).ID;
    }

    void Scene::SelectedEntity(const entt::entity& enttEntity)
    {
        if (enttEntity != entt::null || m_SceneRegistry.valid(enttEntity))
            m_SelectedEntity = enttEntity;
    }

    void Scene::RemoveEntity(const entt::entity& entity)
    {
        if (entity == entt::null) return;
        if (!m_SceneRegistry.valid(entity)) return;

        if (m_SelectedEntity == entity)
            m_SelectedEntity = entt::null;

        const auto* h = m_SceneRegistry.try_get<HierarchyComponent>(entity);
        if (h)
        {
            for (entt::entity child = h->FirstChild; child != entt::null; )
            {
                const entt::entity next = m_SceneRegistry.get<HierarchyComponent>(child).NextSibling;
                RemoveEntity(child);
                child = next;
            }
        }

        if (RigidBodyComponent* rb = m_SceneRegistry.try_get<RigidBodyComponent>(entity))
        {
            if (rb->PhysicsBody && m_PhyWorld)
            {
                if(ColliderComponent* col = m_SceneRegistry.try_get<ColliderComponent>(entity))
                    rb->PhysicsBody->removeCollider(col->Collider);

                m_PhyWorld->destroyRigidBody(rb->PhysicsBody);
                rb->PhysicsBody = nullptr;
            }
        }
        if (auto* col = m_SceneRegistry.try_get<ColliderComponent>(entity))
        {
            if (col->Shape)
            {
                if (col->Type == ShapeType::Box)      m_PhyCommon.destroyBoxShape(dynamic_cast<rp3d::BoxShape*>(col->Shape));
                if (col->Type == ShapeType::Sphere)   m_PhyCommon.destroySphereShape(dynamic_cast<rp3d::SphereShape*>(col->Shape));
                if (col->Type == ShapeType::Capsule)  m_PhyCommon.destroyCapsuleShape(dynamic_cast<rp3d::CapsuleShape*>(col->Shape));
                if (col->Type == ShapeType::Convex)   m_PhyCommon.destroyConvexMeshShape(dynamic_cast<rp3d::ConvexMeshShape*>(col->Shape));
                if (col->Type == ShapeType::Concave)  m_PhyCommon.destroyConcaveMeshShape(dynamic_cast<rp3d::ConcaveMeshShape*>(col->Shape));

                if(col->ConvexMesh) m_PhyCommon.destroyConvexMesh(col->ConvexMesh);

                col->Shape    = nullptr;
            }
        }

        if (!m_RootEntities.empty())
        {
            auto it = std::remove(m_RootEntities.begin(), m_RootEntities.end(), entity);
            if (it != m_RootEntities.end())
                m_RootEntities.erase(it, m_RootEntities.end());
        }

        m_SceneRegistry.destroy(entity);
    }

    void Scene::ForEachActiveEntity(const std::function<void(entt::registry&, entt::entity)>& fn)
    {
        auto traverse = [&](auto&& self, entt::registry& r, entt::entity e) -> void
        {
            if (auto* tag = r.try_get<TagComponent>(e); tag && tag->IsActive)
                fn(r, e);

            if (auto* h = r.try_get<HierarchyComponent>(e))
            {
                for (entt::entity child = h->FirstChild; child != entt::null;)
                {
                    const auto* ch = r.try_get<HierarchyComponent>(child);
                    entt::entity next = ch ? ch->NextSibling : entt::null;
                    self(self, r, child);
                    child = next;
                }
            }
        };

        for (entt::entity e : m_RootEntities)
            traverse(traverse, m_SceneRegistry, e);
    }

    void Scene::ForEachEntity(const std::function<void(entt::registry&, entt::entity)>& fn)
    {
        auto traverse = [&](auto&& self, entt::registry& r, entt::entity e) -> void
        {
            fn(r, e); 

            if (auto* h = r.try_get<HierarchyComponent>(e))
            {
                for (entt::entity child = h->FirstChild; child != entt::null;)
                {
                    const auto* ch = r.try_get<HierarchyComponent>(child);
                    entt::entity next = ch ? ch->NextSibling : entt::null;
                    self(self, r, child);
                    child = next;
                }
            }
        };

        for (entt::entity e : m_RootEntities)
            traverse(traverse, m_SceneRegistry, e);
    }


    void Scene::ForEachRootEntity(const std::function<void(entt::registry&,entt::entity)>& fn)
    {
        for(entt::entity e : m_RootEntities) if(IsRootEntity(e)) fn(m_SceneRegistry, e);
    }

    void Scene::ForEachNodeEntity(entt::entity root, const std::function<void(entt::registry&, entt::entity)>& fn)
    {
        auto traverse = [&](auto&& self, entt::registry& r, entt::entity e) -> void
        {
            fn(r, e);

            if (auto* h = r.try_get<HierarchyComponent>(e))
            {
                for (entt::entity child = h->FirstChild; child != entt::null;)
                {
                    const auto* ch      = r.try_get<HierarchyComponent>(child);
                    entt::entity next   = ch ? ch->NextSibling : entt::null;

                    self(self, r, child);
                    child = next;
                }
            }
        };

        traverse(traverse, m_SceneRegistry, root);
    }


    const bool Scene::IsRootEntity(entt::entity e) const
    {
        if (const auto* h = m_SceneRegistry.try_get<HierarchyComponent>(e))
            return h->Parent == entt::null;

        return false;
    }

}
