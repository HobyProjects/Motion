#pragma once

#include <limits>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>

#include <reactphysics3d/reactphysics3d.h>

#include "Components.hpp"
#include "Entity.hpp"
#include "PhyUtils.hpp"

namespace Motion
{
    struct ContactEvent
    {
        Entity* A{nullptr};
        Entity* B{nullptr};
        bool    enter{false};
        bool    persist{false};
        bool    exit{false};
    };

    class KinetiX : public rp3d::EventListener
    {
        public:
            static KinetiX& GetInstance() { static KinetiX instance; return instance; }
            
            void Init();
            void Reset(); 
            void Quit();

            void Step(const std::vector<std::shared_ptr<Entity>>& entities, float dtSeconds);

            void CreateRigidBody(const std::shared_ptr<Entity>& e);
            void DestroyRigidBody(const std::shared_ptr<Entity>& e);

            void ChangeCollider(const std::shared_ptr<Entity>& e, ShapeType type);
            void CreateConvexCollider(const std::shared_ptr<Entity>& e, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);
            void CreateConcaveCollider(const std::shared_ptr<Entity>& e, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);
            void CreateBoxCollider(const std::shared_ptr<Entity>& e);
            void CreateSphereCollider(const std::shared_ptr<Entity>& e);
            void CreateCapsuleCollider(const std::shared_ptr<Entity>& e);

            void Refresh(const std::vector<std::shared_ptr<Entity>>& entities);
            bool RaycastFirstHit(const glm::vec3& from, const glm::vec3& to, glm::vec3* hitPointWorld = nullptr, glm::vec3* hitNormalWorld = nullptr) const;

            rp3d::PhysicsWorld* GetWorld() const { return m_World; }
            rp3d::PhysicsWorld::WorldSettings& GetConfig() { return m_Settings; }

        private:
            KinetiX() = default; 
            ~KinetiX() override = default;
            
            void DestroyAll();
            void DestroyCachedMeshesFor(Entity* key);

        private:
            rp3d::PhysicsCommon m_Common;
            rp3d::PhysicsWorld* m_World{nullptr};
            rp3d::PhysicsWorld::WorldSettings m_Settings{};
            rp3d::DefaultLogger* m_Logger{nullptr};

            std::unordered_map<Entity*, rp3d::ConvexMesh*>   m_ConvexMeshes{};
            std::unordered_map<Entity*, rp3d::TriangleMesh*> m_TriangleMeshes{};
    };
}