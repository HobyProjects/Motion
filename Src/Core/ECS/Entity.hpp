#pragma once

#include <entt/entt.hpp>
#include "Timer.hpp"

namespace Motion
{
    class Entity
    {
        public:
            struct Node
            {
                bool IsRoot{false};
                std::shared_ptr<Entity> Next{nullptr};
            };

        public:
            Entity() = default;
            Entity(entt::entity handle) :m_EntityHandle(handle){}
            ~Entity() = default;

            template<typename T>
            bool Has() const
            {
                return m_EntityRegistry.any_of<T>(m_EntityHandle);
            }

            template<typename T, typename... Args>
            T& Emplace(Args&&... args)
            {
                auto& component = m_EntityRegistry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
                return component;
            }

            template<typename T>
            T& Get() 
            {
                return m_EntityRegistry.get<T>(m_EntityHandle);
            }

            template<typename T>
            void Remove() 
            {
                m_EntityRegistry.remove<T>(m_EntityHandle);
            }
            
            [[nodiscard]] entt::entity Handle() const
            {
                return m_EntityHandle;
            }

        public:
            static std::shared_ptr<Entity> Create(const std::string& name) noexcept;
            static void Destroy(const std::shared_ptr<Entity>& entity);
            static std::shared_ptr<Entity> Empty();


        public:
            operator bool() const { return m_EntityHandle != entt::null; }
            operator uint32_t() const { return (uint32_t)m_EntityHandle; }
            operator entt::entity() const { return m_EntityHandle; }
            bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle; }
            bool operator!=(const Entity& other) const { return !(*this == other); }
            static entt::registry& GetRegistry() { return m_EntityRegistry; }

        private:
            entt::entity m_EntityHandle{ entt::null };
            inline static entt::registry m_EntityRegistry;
    };

    class ScriptbleEntity
    {
        public:
            ScriptbleEntity() = default;
            virtual ~ScriptbleEntity() = default;

            template<typename T>
            T& GetComponent()
            {
                return m_Entity.Get<T>();
            }

        protected:
            virtual void OnCreate() {}
            virtual void OnUpdate(Timer deltaTime) {}
            virtual void OnDestroy() {}

        private:
            Entity m_Entity;
    };
}