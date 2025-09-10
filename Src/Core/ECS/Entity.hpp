#pragma once

#include <entt/entt.hpp>
#include "Timer.hpp"

namespace Motion
{
    class Entity; // Forward Declaration

    class EntityFactory
    {
        private:
            EntityFactory() = default;
            ~EntityFactory() = default;

            EntityFactory(const EntityFactory&) = delete;
            EntityFactory& operator=(const EntityFactory&) = delete;
            EntityFactory(EntityFactory&&) = delete;
            EntityFactory& operator=(EntityFactory&&) = delete;

        public:
            static EntityFactory& GetInstance() noexcept
            {
                static EntityFactory instance;
                return instance;
            }

        public:
            [[nodiscard]] std::shared_ptr<Entity> CreateEntity(const std::string& name) noexcept;
            [[nodiscard]] std::shared_ptr<Entity> Nullify() const noexcept;
            [[nodiscard]] const entt::registry& GetRegistry() const { return Registry; }

            void DestroyEntity(const std::shared_ptr<Entity>& entity) noexcept;

        public:
            static std::shared_ptr<Entity> EMPTYENTITY;

        private:
            entt::registry Registry;
            friend class Entity;
    };

    class Entity
    {
        public:
            Entity() = default;
            Entity(entt::entity handle) :m_EntityHandle(handle), m_IsAlive(true) {}
            ~Entity() = default;

            template<typename T>
            bool HasComponent() const
            {
                MOTION_ASSERT(m_IsAlive, "Entity already been destroyed");
                auto& entityFactory = EntityFactory::GetInstance();
                return entityFactory.Registry.any_of<T>(m_EntityHandle);
            }

            template<typename T, typename... Args>
            T& AddComponent(Args&&... args)
            {
                MOTION_ASSERT(m_IsAlive, "Entity already been destroyed");
                MOTION_ASSERT(!HasComponent<T>(), "Entity already has component!");
                
                auto& entityFactory = EntityFactory::GetInstance();
                auto& component = entityFactory.Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
                return component;
            }

            template<typename T>
            T& GetComponent() const
            {
                MOTION_ASSERT(m_IsAlive, "Entity already been destroyed");
                MOTION_ASSERT(HasComponent<T>(), "Entity does not have component!");

                auto& entityFactory = EntityFactory::GetInstance();
                return entityFactory.Registry.get<T>(m_EntityHandle);
            }

            template<typename T>
            void RemoveComponent() 
            {
                MOTION_ASSERT(m_IsAlive, "Entity already been destroyed");
                MOTION_ASSERT(HasComponent<T>(), "Entity does not have component!");
                auto& entityFactory = EntityFactory::GetInstance();
                entityFactory.Registry.remove<T>(m_EntityHandle);
            }

            void Destroy()
            {
                auto& entityFactory = EntityFactory::GetInstance();
                entityFactory.Registry.destroy(m_EntityHandle);

                m_EntityHandle  = entt::null;
                m_IsAlive       = false;
            }

            [[nodiscard]] bool IsAlive() const
            {
                return m_IsAlive;
            }

            [[nodiscard]] entt::entity GetHandle() const
            {
                return m_EntityHandle;
            }

            operator bool() const { return m_EntityHandle != entt::null; }
            operator uint32_t() const { return (uint32_t)m_EntityHandle; }
            operator entt::entity() const { return m_EntityHandle; }
            bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle; }
            bool operator!=(const Entity& other) const { return !(*this == other); }

        private:
            entt::entity m_EntityHandle{ entt::null };
            bool m_IsAlive{ false };
    };

    class ScriptbleEntity
    {
        public:
            ScriptbleEntity() = default;
            virtual ~ScriptbleEntity() = default;

            template<typename T>
            T& GetComponent()
            {
                return m_Entity.GetComponent<T>();
            }

        protected:
            virtual void OnCreate() {}
            virtual void OnUpdate(Timer deltaTime) {}
            virtual void OnDestroy() {}

        private:
            Entity m_Entity;
    };
}