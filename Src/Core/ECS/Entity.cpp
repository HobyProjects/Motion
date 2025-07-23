#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<Entity> EntityFactory::EMPTYENTITY = EntityFactory::GetInstance().CreateEntity("Empty Entity");

    /**
     * @brief Creates a new Entity with the specified name.
     *
     * This function constructs a new Entity, assigns it a TagComponent with the given name,
     * and returns a shared pointer to the created Entity. If the provided name is empty,
     * the entity will be tagged as "unnamed".
     *
     * @param name The name to assign to the entity's TagComponent.
     * @return std::shared_ptr<Entity> A shared pointer to the newly created Entity.
     */
    std::shared_ptr<Entity> EntityFactory::CreateEntity(const std::string& name) noexcept
    {
        std::shared_ptr<Entity> entity = std::make_shared<Entity>(Registry.create());
        auto& tag = entity->AddComponent<TagComponent>(name);
        tag.Tag = name.empty() ? "unnamed" : name;
        return entity;
    }

    /**
     * @brief Destroys the specified entity and removes it from the registry.
     *
     * This function takes a shared pointer to an Entity, retrieves its handle,
     * removes the entity from the internal registry, and then calls the entity's
     * own Destroy method to perform any additional cleanup.
     *
     * @param entity A shared pointer to the Entity to be destroyed.
     */
    void EntityFactory::DestroyEntity(const std::shared_ptr<Entity>& entity) noexcept
    {
        auto handle = entity->GetHandle();
        Registry.destroy(handle);
        entity->Destroy();
    }

    /**
     * @brief Returns a null entity, which is an empty entity with no components.
     *
     * This function provides a way to obtain a null entity that can be used
     * when no valid entity is available. It returns the static EMPTYENTITY instance.
     *
     * @return std::shared_ptr<Entity> A shared pointer to the null entity.
     */
    std::shared_ptr<Entity> EntityFactory::Nullify() const noexcept
    {
        return EMPTYENTITY;
    }
}