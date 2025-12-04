#include "CorePCH.hpp"
#include "SceneUtils.hpp"
#include "Scene.hpp"

namespace Motion
{
    /**
     * @brief Constructor for Scene
     * 
     * @param spec The specification of the scene.
     * @param viewport The viewport size of the scene.
     * 
     * This constructor initializes the scene with the given specification and
     * viewport size. It also sets up the physics world, viewport, and simulation
     * context.
     */
    Scene::Scene(const SceneSpecification& spec, const glm::vec2& viewport)
    {
        m_Specification = spec;

        auto& camera              = m_Viewport.Camera;
        camera.AspectRatio        = viewport.x / viewport.y;
        camera.ViewportWidth      = viewport.x;
        camera.ViewportHeight     = viewport.y;
        camera.RotationEnabled    = false;
        camera.Position           = glm::vec3(0.0f, 1.0f, 5.0f);
        camera.TranslationSpeed   = 0.01;

        auto& frameSpec         = m_Viewport.FrameSpecification;
        frameSpec.Name          = spec.Name;
        frameSpec.Width         = (std::int32_t)viewport.x;
        frameSpec.Height        = (std::int32_t)viewport.y;
        frameSpec.Samples       = 1;
        m_Viewport.Framebuffer  = IFrameBuffer::Create(frameSpec);

        auto& phySettings = m_PhysicsWorld.Settings;
        phySettings.gravity = rp3d::Vector3(0.0f, -ScenePhysicsWorld::SI_GRAVITY, 0.0f);
        phySettings.defaultVelocitySolverNbIterations = 20;
        phySettings.isSleepingEnabled = true;

        m_PhysicsWorld.World = m_PhysicsWorld.Properties.createPhysicsWorld(phySettings);
        MOTION_ASSERT(m_PhysicsWorld.World, "Failed to create physics world");
        m_PhysicsWorld.World->setNbIterationsVelocitySolver(15); 
        m_PhysicsWorld.World->setNbIterationsPositionSolver(6);

        m_Context.MyScene       = this;
        m_Context.Physics       = &m_Physics;
        m_Context.PhysicsWorld  = &m_PhysicsWorld;
        m_Context.Entities      = &m_Entities;
        m_Context.View          = &m_Viewport;
        m_Context.Simulation    = &m_Simulation;
        m_Context.Specification = &m_Specification;
        m_Context.Panels        = &m_Panels;
    }

    /**
     * @brief Destructor for Scene
     *
     * This destructor cleans up all the entities in the scene and also
     * destroys the physics world.
     */
    Scene::~Scene() 
    {
        m_Entities.EntryPoints.clear();

        if (m_PhysicsWorld.World) m_PhysicsWorld.Properties.destroyPhysicsWorld(m_PhysicsWorld.World);
        m_PhysicsWorld.World = nullptr;
    }

    /**
     * @brief This function is called on every frame update.
     *
     * This function is responsible for updating the camera based on user input and
     * for updating the physics world if the simulation is running.
     *
     * @param handle The window handle of the application.
     * @param deltaTime The time elapsed since the last frame.
     */
    void Scene::OnUpdate(WindowHandle handle, Timer deltaTime)
    {
        auto& camera = m_Viewport.Camera;
        if ((InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED))
        {
            glm::vec3 forward   = glm::normalize(camera.Oriantaion);
            glm::vec3 right     = glm::normalize(glm::cross(forward, camera.WorldUp));
    
            if (InputsHandler::GetKeyState(handle, KEY_W))              camera.Position += forward * camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_S))              camera.Position -= forward * camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_A))              camera.Position -= right * camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_D))              camera.Position += right * camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            
            if (InputsHandler::GetKeyState(handle, KEY_SPACE))          camera.Position.y += camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            if (InputsHandler::GetKeyState(handle, KEY_LEFT_CONTROL))   camera.Position.y -= camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();

            if(camera.RotationEnabled)
            {
                if(InputsHandler::GetKeyState(handle, KEY_Q)) camera.Position += glm::cross(forward, camera.WorldUp) * camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
                if(InputsHandler::GetKeyState(handle, KEY_E)) camera.Position -= glm::cross(forward, camera.WorldUp) * camera.TranslationSpeed * deltaTime.GetDeltaTimeMilliseconds();
            }
        }

        if (m_Simulation.State == SceneSimulation::SimulationState::RUNNING)
            ApplyPhysics(deltaTime.GetDeltaTimeSeconds());
        
        RefreshPhysicBodies();  
        camera.RefreshCameraMatrix();
    }

    /**
     * @brief Called when an event happens on the scene. This can be a mouse wheel scroll,
     * a mouse move, a key press, or a mouse button press.
     *
     * @param handle The window handle of the application.
     * @param e The event that happened.
     */
    void Scene::OnEvent(WindowHandle handle, IEvent& e)
    {
        EventHandler handler(handle, e);
        handler.Dispatch<EventMouseCursorMove>(EVENT_CALLBACK(OnMouseCursorPosChange));
        handler.Dispatch<EventMouseWheelScroll>(EVENT_CALLBACK(OnMouseWheelScrollEvent));
    }

    /**
     * @brief Called when the mouse cursor moves.
     *
     * This function is responsible for updating the camera orientation based on
     * user input. It is only called when the right mouse button is pressed.
     *
     * @param handle The window handle of the application.
     * @param e The event that happened.
     * @return false always.
     */
    bool Scene::OnMouseCursorPosChange(WindowHandle handle, EventMouseCursorMove& e)
    {
        if(!m_Viewport.ViewportFocusedOrHovered) return false;

        auto& camera = m_Viewport.Camera;
        auto& mouseCtrl = m_Viewport.MouseControls;

        
        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            float currentX = e.GetX();
            float currentY = e.GetY();

            if (mouseCtrl.OnFirstClick)
            {
                mouseCtrl.MouseX = currentX;
                mouseCtrl.MouseY = currentY;
                mouseCtrl.OnFirstClick = false;
                return false; 
            }

            float xOffset = currentX - mouseCtrl.MouseX;
            float yOffset = currentY - mouseCtrl.MouseY;

            mouseCtrl.MouseX = currentX;
            mouseCtrl.MouseY = currentY;

            xOffset *= camera.Sensitivity;
            yOffset *= camera.Sensitivity;

            mouseCtrl.Yaw += xOffset;
            mouseCtrl.Pitch -= yOffset;
            mouseCtrl.Pitch = glm::clamp(mouseCtrl.Pitch, -89.0f, 89.0f);

            glm::vec3 direction;
            direction.x = cos(glm::radians(mouseCtrl.Yaw)) * cos(glm::radians(mouseCtrl.Pitch));
            direction.y = sin(glm::radians(mouseCtrl.Pitch));
            direction.z = sin(glm::radians(mouseCtrl.Yaw)) * cos(glm::radians(mouseCtrl.Pitch));

            camera.Oriantaion = glm::normalize(direction);
            camera.RefreshCameraMatrix();
        }
        else
        {
            mouseCtrl.OnFirstClick = true; 
        }

        return false;
    }

    /**
     * @brief Called when a mouse wheel scroll event occurs
     * @param handle The handle of the window that received the event
     * @param e The event data
     * @return false
     * 
     * This function is called when a mouse wheel scroll event occurs. If the right mouse button is being held down, the camera's field of view is adjusted based on the scroll offset.
     */
    bool Scene::OnMouseWheelScrollEvent(WindowHandle handle, EventMouseWheelScroll& e)
    {
        if(!m_Viewport.ViewportFocusedOrHovered) return false;

        if (InputsHandler::GetMouseButtonState(handle, MOUSE_BUTTON_RIGHT) & MOUSE_BUTTON_PRESSED)
        {
            auto& camera = m_Viewport.Camera;
            camera.PerspectiveFov -= (float)e.OffsetY();

            if (camera.PerspectiveFov < 1.0f) camera.PerspectiveFov = 1.0f;
            if (camera.PerspectiveFov > 45.0f) camera.PerspectiveFov = 45.0f;

            camera.RefreshCameraMatrix();
        }

        return false;
    }

    /**
     * @brief This function is called by the application's main loop to update the physics
     * simulation.
     *
     * This function will update the physics simulation by the given amount of time. The
     * function will use the given amount of time to determine how many fixed steps of
     * simulation to perform. If the given amount of time is greater than the total
     * amount of fixed steps per frame, the simulation will be updated by the total
     * amount of fixed steps per frame.
     *
     * After the simulation has been updated, the function will loop through all the active
     * entities in the scene and update their transforms based on the physics bodies
     * associated with the entities.
     *
     * @param deltaTime The amount of time to simulate in seconds.
     */
    void Scene::ApplyPhysics(float deltaTime)
    {
        deltaTime = std::clamp(deltaTime, 0.0f, 0.1f);
        m_PhysicsWorld.ACCUMULATOR = std::min<float>(m_PhysicsWorld.ACCUMULATOR + deltaTime, 0.25f);

        std::int32_t steps = 0;
        while(m_PhysicsWorld.ACCUMULATOR >= ScenePhysicsWorld::FIXED_STEPS 
            && steps < ScenePhysicsWorld::FIXED_STEPS_PERFRAME)
        {
            m_PhysicsWorld.World->update(ScenePhysicsWorld::FIXED_STEPS);
            m_PhysicsWorld.ACCUMULATOR -= ScenePhysicsWorld::FIXED_STEPS;
            ++steps;
        }

        ForEachActiveEntity([&](entt::entity e)
        {
            auto* rigidBody = m_Entities.Registry.try_get<RigidBodyComponent>(e);
            auto* transform = m_Entities.Registry.try_get<TransformComponent>(e);
            if(!rigidBody || !transform) return;

            if(rigidBody->PhysicsBody && rigidBody->PhysicsBody->getType() == rp3d::BodyType::DYNAMIC)
            {
                Transform(rigidBody->PhysicsBody->getTransform(), *transform);
            }
        });
    }

    /**
     * This function will loop through all the active entities in the scene and update their
     * transforms based on the physics bodies associated with the entities.
     *
     * The function will update the transforms of the entities by getting the current
     * transform of the physics bodies associated with the entities and applying it
     * to the transforms of the entities. This function will also update the colliders
     * associated with the entities by updating the scale of the colliders based on the
     * scale of the transforms of the entities.
     *
     * If the type of the physics body is dynamic, the function will update the mass
     * properties of the physics body based on the colliders associated with it.
     */
    void Scene::RefreshPhysicBodies()
    {
        ForEachActiveEntity([&](entt::entity e)
        {
            auto* transform = m_Entities.Registry.try_get<TransformComponent>(e);
            auto* rigidBody = m_Entities.Registry.try_get<RigidBodyComponent>(e);
            auto* collider  = m_Entities.Registry.try_get<ColliderComponent>(e);
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

    /**
     * @brief Sets the aspect ratio of the scene's viewport camera and frame buffer.
     *
     * Sets the aspect ratio of the scene's viewport camera and frame buffer to the specified size.
     * This will cause the camera's projection matrix to be updated and the frame buffer to be resized.
     *
     * @param size The new size of the viewport camera and frame buffer.
     */
    void Scene::SetApectRatio(const glm::vec2& size)
    {
        m_Viewport.Camera.SetAspectRatio(size.x, size.y);
        m_Viewport.Framebuffer->ResizeFrame((std::int32_t)size.x, (std::int32_t)size.y);
    }

    /**
     * @brief Submits the scene to the renderer.
     *
     * This function binds the scene's frame buffer, sets the viewport, clears the
     * color and depth buffers, and begins a new render pass. It then iterates
     * over all active entities in the scene and submits a render command for each
     * entity that has a mesh, material, and transform component. The render
     * command is constructed from the camera's view and projection matrices, the
     * sun's direction, color, and intensity, and the entity's model, normal, and
     * light data. Finally, the function ends the render pass and unbinds the
     * frame buffer, setting the frame buffer's texture pointer to the newly rendered
     * texture.
     *
     * @note This function should be called once per frame, after all entities have
     * been updated and before the frame buffer is swapped.
     */
    void Scene::Submit() 
    {
        m_Viewport.Framebuffer->Bind();
        
        Renderer::SetViewport(0, 0, m_Viewport.FrameSpecification.Width, m_Viewport.FrameSpecification.Height);
        ImVec4 viewportColor = UserInterface::ThemeManager::GetViewportColor();
        Renderer::ClearColor({ viewportColor.x, viewportColor.y, viewportColor.z, viewportColor.w });
        
        Renderer::Clear();
        Renderer::Begin();

        const CameraViewProjection cam
        {
            .View           = m_Viewport.Camera.View,
            .Projection     = m_Viewport.Camera.Projection,
            .CameraPosition = m_Viewport.Camera.Position,
            .ViewportSize   = m_Viewport.ViewportSize
        };

        const DirectionalLight sun
        {
            .Direction = m_PhysicsWorld.SunLight.Direction,
            .Color     = m_PhysicsWorld.SunLight.Color,
            .Intensity = m_PhysicsWorld.SunLight.Intensity
        };

        ForEachActiveEntity([&](entt::entity e)
        {
            const auto* mesh      = m_Entities.Registry.try_get<MeshComponent>(e);
            const auto* material  = m_Entities.Registry.try_get<MaterialComponent>(e);
            const auto* transform = m_Entities.Registry.try_get<TransformComponent>(e);

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

        m_Viewport.Framebuffer->Unbind();
        m_Viewport.FrameTexturePtr =  m_Viewport.Framebuffer->GetAttachment
            (FrameBufferColorAttachmentStandards::Standard).ID;
    }

    /**
     * @brief Sets the currently selected entity in the scene.
     *
     * Sets the currently selected entity in the scene to the specified entity. If the
     * specified entity is not valid, the currently selected entity is set to null.
     *
     * @param enttEntity The entity to select.
     */
    void Scene::SelectedEntity(const entt::entity& enttEntity)
    {
        if (enttEntity != entt::null || m_Entities.Registry.valid(enttEntity))
            m_Entities.SelectedEntity = enttEntity;
    }

    /**
     * @brief Emplaces an entity in the scene's entry point list.
     *
     * Emplaces the specified entity in the scene's entry point list. If the
     * specified entity is not valid, nothing is done.
     *
     * @param entity The entity to emplace.
     */
    void Scene::EmplaceEntity(const entt::entity& entity)
    {
        if(m_Entities.Registry.valid(entity))
        {
            m_Entities.EntryPoints.emplace_back(std::move(entity));
        }
    }


    /**
     * @brief Destroys an entity in the scene.
     *
     * Destroys an entity in the scene, along with all its components and children.
     * If the specified entity is not valid, nothing is done.
     *
     * If deleteResources is true, any resources associated with the entity and its
     * components are deleted. For example, if the entity has a ModelComponent, the
     * associated model file is deleted.
     *
     * @param entity The entity to destroy.
     * @param deleteResources If true, any resources associated with the entity and its
     * components are deleted.
     */
    void Scene::DestroyEntity(const entt::entity& entity, bool deleteResources)
    {
        auto DestroyComponents = [&](entt::entity nodeEntity)
        {
            if (nodeEntity == entt::null) return;
            if (!m_Entities.Registry.valid(nodeEntity)) return;  
            if (m_Entities.SelectedEntity == nodeEntity) 
                m_Entities.SelectedEntity = entt::null;

            if (RigidBodyComponent* rb = m_Entities.Registry.try_get<RigidBodyComponent>(nodeEntity))
            {
                if (rb->PhysicsBody && m_PhysicsWorld.World)
                {
                    if(ColliderComponent* col = m_Entities.Registry.try_get<ColliderComponent>(nodeEntity))
                        rb->PhysicsBody->removeCollider(col->Collider);

                    m_PhysicsWorld.World->destroyRigidBody(rb->PhysicsBody);
                    rb->PhysicsBody = nullptr;
                }
            }

            if (ColliderComponent* col = m_Entities.Registry.try_get<ColliderComponent>(nodeEntity))
            {
                if (col->Shape)
                {
                    if (col->Type == ShapeType::Box)      m_PhysicsWorld.Properties.destroyBoxShape(dynamic_cast<rp3d::BoxShape*>(col->Shape));
                    if (col->Type == ShapeType::Sphere)   m_PhysicsWorld.Properties.destroySphereShape(dynamic_cast<rp3d::SphereShape*>(col->Shape));
                    if (col->Type == ShapeType::Capsule)  m_PhysicsWorld.Properties.destroyCapsuleShape(dynamic_cast<rp3d::CapsuleShape*>(col->Shape));
                    if (col->Type == ShapeType::Convex)   m_PhysicsWorld.Properties.destroyConvexMeshShape(dynamic_cast<rp3d::ConvexMeshShape*>(col->Shape));
                    if (col->Type == ShapeType::Concave)  m_PhysicsWorld.Properties.destroyConcaveMeshShape(dynamic_cast<rp3d::ConcaveMeshShape*>(col->Shape));

                    if(col->ConvexMesh) m_PhysicsWorld.Properties.destroyConvexMesh(col->ConvexMesh);
                    col->Shape = nullptr;
                }
            }
        };

        entt::entity root = FindRootOf(entity); 
        if(root == entt::null) return;

        if(deleteResources)
        {
            if(ModelComponent* model = m_Entities.Registry.try_get<ModelComponent>(root))
            {
                try
                {
                    std::error_code ec{};
                    std::filesystem::remove(model->FilePath, ec);
                    if(ec) MOTION_ERROR("Failed to delete model file: {}", ec.message());
                }
                catch(const std::exception& e)
                {
                    MOTION_ERROR("Failed to delete model file: {}", e.what());
                }
            }
        }

        if (HierarchyComponent* hier = m_Entities.Registry.try_get<HierarchyComponent>(root))
        {
            if (hier->Parent != entt::null)
            {
                if (auto* parentHier = m_Entities.Registry.try_get<HierarchyComponent>(hier->Parent))
                {
                    if (parentHier->FirstChild == root) parentHier->FirstChild = hier->NextSibling;
                }
            }
        }

        std::vector<entt::entity> toDestroy;
        ForEachNodeEntity(root, [&](entt::entity node)
        {
            toDestroy.push_back(node);
        });

        for (entt::entity node : toDestroy)
        {
            DestroyComponents(node);
            m_Entities.Registry.destroy(node);
        }

        auto it = std::find(m_Entities.EntryPoints.begin(), m_Entities.EntryPoints.end(), root);
        if (it != m_Entities.EntryPoints.end())
        {
            m_Entities.EntryPoints.erase(it);
        }
    }

    /**
     * @brief Duplicates an entity and its children in the scene.
     *
     * Duplicates an entity and its children in the scene. The duplicated entity will
     * have the same name as the original entity, with "_Copy" appended to the end.
     * The duplicated entity will also have the same transform, material, and physics components
     * as the original entity.
     *
     * @param entity The entity to duplicate.
     * @return True if the entity was successfully duplicated, false otherwise.
     */
    bool Scene::DuplicateEntity(const entt::entity& entity)
    {
        if (!m_Entities.Registry.valid(entity)) return false;
        entt::entity srcEntity = FindRootOf(entity);

        auto* srcModel = m_Entities.Registry.try_get<ModelComponent>(srcEntity);
        if(!srcModel) return false;

        // Re-import to get CPU-side mesh data for physics
        ImportSettings settings{};
        settings.ExportPath = m_Specification.SavedPath / "Assets";
        settings.ShouldExport = true;
        settings.FilePath = srcModel->FilePath;

        std::shared_ptr<ImportedResults> results = Importer::ImportEntity(settings);
        if(!results) return false;

        const BufferLayout layout
        {
            { "a_Position",   BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Position)    },
            { "a_TexCoords",  BufferComponents::UV,   BufferStride::F2, false, offsetof(Vertex, TexCoord)    },
            { "a_Normals",    BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Normal)      },
            { "a_Tangents",   BufferComponents::XYZW, BufferStride::F4, false, offsetof(Vertex, Tangent)     },
            { "a_Bitangents", BufferComponents::XYZ,  BufferStride::F3, false, offsetof(Vertex, Bitangent)   },
        };

        // Create root entity
        entt::entity dstEntity = m_Entities.Registry.create();
        
        // Copy root tag with "_Copy" suffix
        if (auto* srcTag = m_Entities.Registry.try_get<TagComponent>(srcEntity))
        {
            auto& dstTag = m_Entities.Registry.emplace<TagComponent>(dstEntity);
            dstTag.Tag = srcTag->Tag + "_Copy";
            dstTag.IsActive = srcTag->IsActive;
        }
        else
        {
            m_Entities.Registry.emplace<TagComponent>(dstEntity, results->Name + "_Copy");
        }

        // Copy root transform with offset
        if (auto* srcTransform = m_Entities.Registry.try_get<TransformComponent>(srcEntity))
        {
            auto& dstTransform = m_Entities.Registry.emplace<TransformComponent>(dstEntity);
            dstTransform.Translation = srcTransform->Translation;
            dstTransform.Rotation = srcTransform->Rotation;
            dstTransform.Scale = srcTransform->Scale;
            dstTransform.Translation.x += 1.5f; // Offset the duplicate
            dstTransform.RebuildLocal();
        }
        else
        {
            m_Entities.Registry.emplace<TransformComponent>(dstEntity);
        }

        // Copy model component
        auto& modelCompo = m_Entities.Registry.emplace<ModelComponent>(dstEntity);
        modelCompo.FilePath = results->FilePath;
        modelCompo.MeshCount = results->MeshCount;
        modelCompo.MaxBounds = results->MAX;
        modelCompo.MinBounds = results->MIN;

        // Build mapping: meshIndex -> source child entity
        std::unordered_map<uint32_t, entt::entity> srcChildrenByIndex;
        if (auto* srcHier = m_Entities.Registry.try_get<HierarchyComponent>(srcEntity))
        {
            for (entt::entity srcChild = srcHier->FirstChild; 
                srcChild != entt::null;)
            {
                if (auto* srcMesh = m_Entities.Registry.try_get<MeshComponent>(srcChild))
                {
                    srcChildrenByIndex[srcMesh->MeshIndex] = srcChild;
                }
                
                if (auto* childHier = m_Entities.Registry.try_get<HierarchyComponent>(srcChild))
                    srcChild = childHier->NextSibling;
                else
                    break;
            }
        }

        std::vector<entt::entity> dstChildren;
        dstChildren.reserve(results->MeshCount);

        // Create child entities with proper mapping
        for (const auto& [index, mesh] : results->Meshes)
        {
            entt::entity dstChild = m_Entities.Registry.create();
            
            // Find corresponding source entity
            entt::entity srcChild = entt::null;
            auto it = srcChildrenByIndex.find(index);
            if (it != srcChildrenByIndex.end())
                srcChild = it->second;

            // Copy tag from source or use imported name
            if (srcChild != entt::null)
            {
                if (auto* srcTag = m_Entities.Registry.try_get<TagComponent>(srcChild))
                {
                    auto& dstTag = m_Entities.Registry.emplace<TagComponent>(dstChild);
                    dstTag.Tag = srcTag->Tag + "_Copy";
                    dstTag.IsActive = srcTag->IsActive;
                }
                else
                {
                    m_Entities.Registry.emplace<TagComponent>(dstChild, mesh.Name + "_Copy");
                }
            }
            else
            {
                m_Entities.Registry.emplace<TagComponent>(dstChild, mesh.Name + "_Copy");
            }

            // Copy transform from source child
            if (srcChild != entt::null)
            {
                if (auto* srcTransform = m_Entities.Registry.try_get<TransformComponent>(srcChild))
                {
                    auto& dstTransform = m_Entities.Registry.emplace<TransformComponent>(dstChild);
                    dstTransform.Translation = srcTransform->Translation;
                    dstTransform.Rotation = srcTransform->Rotation;
                    dstTransform.Scale = srcTransform->Scale;
                    dstTransform.RebuildLocal();
                }
                else
                {
                    m_Entities.Registry.emplace<TransformComponent>(dstChild);
                }
            }
            else
            {
                m_Entities.Registry.emplace<TransformComponent>(dstChild);
            }

            // Create RigidBody and copy properties
            auto& dstRB = m_Entities.Registry.emplace<RigidBodyComponent>(dstChild);
            if (srcChild != entt::null)
            {
                if (auto* srcRB = m_Entities.Registry.try_get<RigidBodyComponent>(srcChild))
                {
                    dstRB.Type = srcRB->Type;
                    dstRB.LinearDamping = srcRB->LinearDamping;
                    dstRB.AngularDamping = srcRB->AngularDamping;
                    dstRB.LockX = srcRB->LockX;
                    dstRB.LockY = srcRB->LockY;
                    dstRB.LockZ = srcRB->LockZ;
                    dstRB.LockRotX = srcRB->LockRotX;
                    dstRB.LockRotY = srcRB->LockRotY;
                    dstRB.LockRotZ = srcRB->LockRotZ;
                }
            }
            CreateRigidBody(m_PhysicsWorld.World, &m_Entities.Registry, dstChild);

            // Create Collider and copy properties
            auto& dstCol = m_Entities.Registry.emplace<ColliderComponent>(dstChild);
            if (srcChild != entt::null)
            {
                if (auto* srcCol = m_Entities.Registry.try_get<ColliderComponent>(srcChild))
                {
                    dstCol.Type = srcCol->Type;
                    dstCol.BoxHalfExtents = srcCol->BoxHalfExtents;
                    dstCol.SphereRadius = srcCol->SphereRadius;
                    dstCol.Capsule = srcCol->Capsule;
                    dstCol.LocalTransform = srcCol->LocalTransform;
                    dstCol.LocalRotation = srcCol->LocalRotation;
                    dstCol.Friction = srcCol->Friction;
                    dstCol.Restitution = srcCol->Restitution;
                    dstCol.MassDensity = srcCol->MassDensity;
                }
            }

            // Create mesh component with uploaded GPU data
            auto& meshCompo = m_Entities.Registry.emplace<MeshComponent>(dstChild);
            meshCompo.MeshPointer = Mesh::Create(mesh.Vertices.data(), mesh.Vertices.size(),
                                                mesh.Indices.data(), mesh.Indices.size(), layout);
            meshCompo.MeshIndex = index;
            meshCompo.Name = mesh.Name;
            meshCompo.MaxBounds = mesh.MAX;
            meshCompo.MinBounds = mesh.MIN;

            // Create material and copy properties
            auto& dstMaterial = m_Entities.Registry.emplace<MaterialComponent>(dstChild);
            dstMaterial.MaterialPointer = Material::Create();
            
            if (srcChild != entt::null)
            {
                if (auto* srcMaterial = m_Entities.Registry.try_get<MaterialComponent>(srcChild))
                {
                    auto srcMatPtr = srcMaterial->MaterialPointer;
                    auto dstMatPtr = dstMaterial.MaterialPointer;
                    
                    if (srcMatPtr && dstMatPtr)
                    {
                        if (srcMatPtr->Has<CoreMaterialComponents>() && 
                            dstMatPtr->Has<CoreMaterialComponents>())
                        {
                            auto& srcCore = srcMatPtr->Get<CoreMaterialComponents>();
                            auto& dstCore = dstMatPtr->Get<CoreMaterialComponents>();
                            dstCore = srcCore;
                        }
                        
                        auto baseMaterial = srcMatPtr->GetBaseMaterial();
                        dstMatPtr->SetBaseMaterial(baseMaterial);
                    }
                }
            }

            // Create physics collider with mesh vertex data
            std::vector<glm::vec3> verts;
            verts.reserve(mesh.Vertices.size());
            std::transform(mesh.Vertices.begin(), mesh.Vertices.end(), 
                        std::back_inserter(verts),
                        [](const Vertex& v) { return v.Position; });

            CreateConvexCollider(&m_PhysicsWorld.Properties, &m_Entities.Registry, dstChild, verts);
            
            dstChildren.push_back(dstChild);
        }

        // Setup hierarchy for children
        entt::entity prev = entt::null;
        for (std::size_t i = 0; i < dstChildren.size(); ++i)
        {
            auto e = dstChildren[i];
            m_Entities.Registry.emplace<HierarchyComponent>(e, dstEntity, entt::null, entt::null);
            if (i > 0) 
                m_Entities.Registry.get<HierarchyComponent>(prev).NextSibling = e;
            prev = e;
        }

        // Setup hierarchy for root
        m_Entities.Registry.emplace<HierarchyComponent>(dstEntity, entt::null,
            dstChildren.empty() ? entt::null : dstChildren.front(), entt::null);

        EmplaceEntity(dstEntity);
        return true;
    }

    /**
     * @brief Iterates over all active entities in the scene.
     *
     * Iterates over all entities in the scene that have a TagComponent with its IsActive
     * property set to true. The provided function is called once for each active entity
     * in the scene.
     *
     * @param fn A function that takes an entt::entity as its argument.
     */
    void Scene::ForEachActiveEntity(const std::function<void(entt::entity)>& fn)
    {
        auto traverse = [&](auto&& self, entt::registry& r, entt::entity e) -> void
        {
            if (auto* tag = r.try_get<TagComponent>(e); tag && tag->IsActive)
                fn(e);

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

        for (entt::entity e : m_Entities.EntryPoints)
            traverse(traverse, m_Entities.Registry, e);
    }

    /**
     * @brief Iterates over all entities in the scene.
     *
     * Iterates over all entities in the scene. The provided function is called once
     * for each entity in the scene.
     *
     * @param fn A function that takes an entt::entity as its argument.
     */
    void Scene::ForEachEntity(const std::function<void(entt::entity)>& fn)
    {
        auto traverse = [&](auto&& self, entt::registry& r, entt::entity e) -> void
        {
            fn(e); 

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

        for (entt::entity e : m_Entities.EntryPoints)
            traverse(traverse, m_Entities.Registry, e);
    }


    /**
     * @brief Iterates over all root entities in the scene.
     *
     * Iterates over all root entities in the scene. The provided function is called
     * once for each root entity in the scene.
     *
     * @param fn A function that takes an entt::entity as its argument.
     */
    void Scene::ForEachRootEntity(const std::function<void(entt::entity)>& fn)
    {
        for(entt::entity e : m_Entities.EntryPoints) if(IsRootEntity(e)) fn(e);
    }

    /**
     * @brief Iterates over all entities in the scene, starting from a given entity.
     *
     * Iterates over all entities in the scene, starting from a given entity. The
     * provided function is called once for each entity in the scene.
     *
     * @param root The root entity to start the iteration from.
     * @param fn A function that takes an entt::entity as its argument.
     */
    void Scene::ForEachNodeEntity(entt::entity root, const std::function<void(entt::entity)>& fn)
    {
        auto traverse = [&](auto&& self, entt::registry& r, entt::entity e) -> void
        {
            fn(e);

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

        traverse(traverse, m_Entities.Registry, root);
    }

    /**
     * @brief Checks if an entity is a root entity in the scene.
     *
     * Checks if an entity is a root entity in the scene. This function checks if the
     * entity has a parent entity.
     *
     * @param e The entity to check.
     * @return True if the entity is a root entity, false otherwise.
     */
    const bool Scene::IsRootEntity(entt::entity e) const
    {
        if (const auto* h = m_Entities.Registry.try_get<HierarchyComponent>(e))
            return h->Parent == entt::null;

        return false;
    }
    
    /**
     * @brief Checks if an entity is a node entity in the scene.
     *
     * Checks if an entity is a node entity in the scene. This function checks if the
     * entity does not have a parent entity.
     *
     * @param entity The entity to check.
     * @return True if the entity is a node entity, false otherwise.
     */
    const bool Scene::IsNodeEntity(entt::entity entity) const
    {
        return !IsRootEntity(entity);
    }

    /**
     * @brief Gets the root node of a given entity.
     *
     * Gets the root node of a given entity. This function retrieves the parent
     * entity from the HierarchyComponent of the specified entity.
     *
     * @param entity The entity to get the root node of.
     * @return The root node entity, or entt::null if the entity has no parent.
     */
    entt::entity Scene::FindRootOf(entt::entity entity)
    {
        if(IsRootEntity(entity)) return entity;

        entt::entity found = entt::null;
        ForEachRootEntity([&](entt::entity root)
        {
            ForEachNodeEntity(root, [&](entt::entity node)
            {
                if(node == entity)
                {
                    found = root;
                    return;
                }
            });

            if(found != entt::null) return;
        });

        if(m_Entities.Registry.valid(found))
        {
            MOTION_INFO("Found root of entity {0}", entt::to_integral(found));
            return found;
        }

        MOTION_INFO("Failed to find root of entity {0}", entt::to_integral(entity));
        return entt::null;
    }
}
