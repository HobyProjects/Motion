#pragma once

#include <entt/entt.hpp>
#include "Timer.hpp"

namespace Motion::Core
{
    class Entity; // Forward Declaration

    class EntityBuilder
    {
        public:
            static std::shared_ptr<Entity> CreateEntity(const std::string& name);
            static void DestroyEntity(const std::shared_ptr<Entity>& entity);
            static std::shared_ptr<Entity> Empty();

        private:
            EntityBuilder() = default;
            ~EntityBuilder() = default;

        public:
            static std::shared_ptr<Entity> ENULL;

        private:
            static entt::registry Registry;
            friend class Entity;
    };

    class Entity
    {
        public:
            Entity() = default;
            Entity(entt::entity handle) :m_EntityHandle(handle), m_IsAlive(true){}
            ~Entity() = default;

            template<typename T>
            bool HasComponent() const
            {
                if( m_IsAlive )
                    return EntityBuilder::Registry.any_of<T>(m_EntityHandle);

                MOTION_ASSERT(false, "Entity already been destroyed");
                return false;
            }

            template<typename T, typename... Args>
            T& AddComponent(Args&&... args)
            {
                MOTION_ASSERT(!HasComponent<T>(), "Entity already has component!");
                if( m_IsAlive )
                    return EntityBuilder::Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);

                MOTION_ASSERT(false, "Entity already been destroyed");

                // To satisfy the compiler, throw or return a reference to a static dummy object
                static T dummy{};
                return dummy;
            }

            template<typename T>
            T& GetComponent() const
            {
                MOTION_ASSERT(HasComponent<T>(), "Entity does not have component!");
                if( m_IsAlive )
                    return EntityBuilder::Registry.get<T>(m_EntityHandle);

                MOTION_ASSERT(false, "Entity already been destroyed");

                // To satisfy the compiler, throw or return a reference to a static dummy object
                static T dummy{};
                return dummy;
            }

            template<typename T>
            void RemoveComponent()
            {
                MOTION_ASSERT(HasComponent<T>(), "Entity does not have component!");
                if( m_IsAlive )
                    EntityBuilder::Registry.remove<T>(m_EntityHandle);

                MOTION_ASSERT(false, "Entity already been destroyed");
            }

            void Destroy()
            {
                EntityBuilder::Registry.destroy(m_EntityHandle);
                m_EntityHandle = entt::null;
                m_IsAlive = false;
            }

            bool IsAlive() const
            {
                return m_IsAlive;
            }

            entt::entity GetHandle() const
            {
                return m_EntityHandle;
            }

            void CreateNewHandle(const std::string& name)
            {
                m_EntityHandle = EntityBuilder::Registry.create();
                m_IsAlive = true;
            }

            operator bool() const { return m_EntityHandle != entt::null; }
            operator uint32_t() const { return (uint32_t) m_EntityHandle; }
            operator entt::entity() const { return m_EntityHandle; }

            bool operator==(const Entity& other) const
            {
                return m_EntityHandle == other.m_EntityHandle;
            }

            bool operator!=(const Entity& other) const
            {
                return !( *this == other );
            }

        private:
            entt::entity m_EntityHandle{ entt::null };
            bool m_IsAlive{ false };
    };

    class Scene; // Forward Declaration

    class ScriptableEntity
    {
        public:
            ScriptableEntity() = default;
            virtual ~ScriptableEntity() = default;

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
            friend class Scene;
    };
}