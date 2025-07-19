#pragma once

#include <entt/entt.hpp>
#include "Timer.hpp"

namespace Motion::Core
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
        /**
         * @brief Returns the singleton instance of EntityFactory.
         *
         * This method ensures that only one instance of EntityFactory exists throughout the application.
         * It initializes the instance if it does not already exist.
         *
         * @return Reference to the singleton EntityFactory instance.
         */
        static EntityFactory& GetInstance() noexcept
        {
            static EntityFactory instance;
            return instance;
        }

    public:
        [[nodiscard]] std::shared_ptr<Entity> CreateEntity(const std::string& name) noexcept;
        [[nodiscard]] std::shared_ptr<Entity> Nullify() const noexcept;

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

        /**
         * @brief Checks if the entity has a component of type T.
         *
         * This function verifies whether the entity, represented by this instance,
         * currently possesses a component of the specified type T. If the entity
         * is alive, it queries the registry to determine the presence of the component.
         * If the entity has already been destroyed, an assertion is triggered and
         * the function returns false.
         *
         * @tparam T The type of the component to check for.
         * @return true if the entity has the component of type T and is alive, false otherwise.
         */
        template<typename T>
        bool HasComponent() const
        {
            if (m_IsAlive)
                return EntityFactory::Registry.any_of<T>(m_EntityHandle);

            MOTION_ASSERT(false, "Entity already been destroyed");
            return false;
        }

        /**
         * @brief Adds a component of type T to the entity with the provided arguments.
         *
         * This function asserts that the entity does not already have a component of type T.
         * If the entity is alive, it constructs and attaches the component to the entity using
         * the provided arguments. If the entity has already been destroyed, an assertion fails.
         *
         * @tparam T The type of the component to add.
         * @tparam Args The types of the arguments to forward to the component's constructor.
         * @param args Arguments to forward to the component's constructor.
         * @return Reference to the newly added component of type T.
         * @throws Assertion failure if the entity already has the component or has been destroyed.
         */
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            MOTION_ASSERT(!HasComponent<T>(), "Entity already has component!");
            if (m_IsAlive)
                return EntityFactory::Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);

            MOTION_ASSERT(false, "Entity already been destroyed");

            // To satisfy the compiler, throw or return a reference to a static dummy object
            static T dummy{};
            return dummy;
        }

        /**
         * @brief Retrieves a reference to the component of type T attached to this entity.
         *
         * This function asserts that the entity has the requested component type T.
         * If the entity is alive, it returns a reference to the component from the registry.
         * If the entity has been destroyed, an assertion will fail and a reference to a static dummy object is returned to satisfy the compiler.
         *
         * @tparam T The type of the component to retrieve.
         * @return T& Reference to the requested component.
         * @throws Assertion failure if the entity does not have the component or has been destroyed.
         */
        template<typename T>
        T& GetComponent() const
        {
            MOTION_ASSERT(HasComponent<T>(), "Entity does not have component!");
            if (m_IsAlive)
                return EntityFactory::Registry.get<T>(m_EntityHandle);

            MOTION_ASSERT(false, "Entity already been destroyed");

            // To satisfy the compiler, throw or return a reference to a static dummy object
            static T dummy{};
            return dummy;
        }

        /**
         * @brief Removes a component of type T from the entity.
         *
         * This function asserts that the entity currently has the component of type T before attempting removal.
         * If the entity is alive, it removes the component from the registry.
         * After removal, it asserts false to indicate that the entity has already been destroyed, which may be used for debugging purposes.
         *
         * @tparam T The type of the component to remove.
         *
         * @note The function will trigger an assertion failure if the entity does not have the component,
         *       or if the code path after removal is reached (which may indicate a logic error).
         */
        template<typename T>
        void RemoveComponent()
        {
            MOTION_ASSERT(HasComponent<T>(), "Entity does not have component!");
            if (m_IsAlive)
                EntityFactory::Registry.remove<T>(m_EntityHandle);

            MOTION_ASSERT(false, "Entity already been destroyed");
        }

        /**
         * @brief Destroys the entity associated with this instance.
         *
         * This function removes the entity from the registry, invalidates its handle,
         * and marks it as no longer alive. After calling this method, the entity
         * should not be used.
         */
        void Destroy()
        {
            auto& entityFactory = EntityFactory::GetInstance();
            entityFactory.Registry.destroy(m_EntityHandle);
            m_EntityHandle = entt::null;
            m_IsAlive = false;
        }

        /**
         * @brief Checks if the entity is alive.
         *
         * This function returns true if the entity is currently alive (i.e., it has not been destroyed).
         * If the entity has been destroyed, it returns false.
         *
         * @return true if the entity is alive, false otherwise.
         */
        [[nodiscard]] bool IsAlive() const
        {
            return m_IsAlive;
        }

        /**
         * @brief Gets the handle of the entity.
         *
         * This function returns the entt::entity handle associated with this entity instance.
         * If the entity has been destroyed, it will return entt::null.
         *
         * @return entt::entity The handle of the entity.
         */
        [[nodiscard]] entt::entity GetHandle() const
        {
            return m_EntityHandle;
        }

        /**
         * @brief Creates a new entity handle and marks it as alive.
         *
         * This function initializes a new entity handle, assigns it to this instance,
         * and sets the alive status to true. It is typically used when creating a new entity.
         *
         * @param name The name of the entity (not used in this implementation).
         */
        void CreateNewHandle(const std::string& name)
        {
            auto& entityFactory = EntityFactory::GetInstance();
            m_EntityHandle = entityFactory.Registry.create();
            m_IsAlive = true;
        }

        /**
         * @brief Converts the entity to a boolean value.
         *
         * This function allows the entity to be used in boolean contexts, returning true
         * if the entity is alive (i.e., it has a valid handle and has not been destroyed).
         *
         * @return true if the entity is alive, false otherwise.
         */
        operator bool() const { return m_EntityHandle != entt::null; }

        /**
         * @brief Converts the entity to an unsigned integer value.
         *
         * This function allows the entity to be used in contexts that require an unsigned integer,
         * returning the underlying handle as a uint32_t.
         *
         * @return uint32_t The underlying handle of the entity.
         */
        operator uint32_t() const { return (uint32_t)m_EntityHandle; }

        /**
         * @brief Converts the entity to an entt::entity type.
         *
         * This function allows the entity to be used in contexts that require an entt::entity,
         * returning the underlying handle as an entt::entity.
         *
         * @return entt::entity The underlying handle of the entity.
         */
        operator entt::entity() const { return m_EntityHandle; }

        /**
         * @brief Compares two entities for equality.
         *
         * This function checks if the underlying handles of two entities are equal.
         * It returns true if both entities have the same handle, indicating they are the same entity.
         *
         * @param other The other entity to compare against.
         * @return true if the two entities are equal, false otherwise.
         */
        bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle; }

        /**
         * @brief Compares two entities for inequality.
         *
         * This function checks if the underlying handles of two entities are not equal.
         * It returns true if the entities have different handles, indicating they are distinct entities.
         *
         * @param other The other entity to compare against.
         * @return true if the two entities are not equal, false otherwise.
         */
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