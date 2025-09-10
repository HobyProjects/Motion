#include "CorePCH.hpp"

namespace Motion
{
    std::shared_ptr<Entity> EntityFactory::EMPTYENTITY = EntityFactory::GetInstance().CreateEntity("Empty Entity");

    std::shared_ptr<Entity> EntityFactory::CreateEntity(const std::string& name) noexcept
    {
        std::shared_ptr<Entity> entity = std::make_shared<Entity>(Registry.create());
        auto& tag = entity->AddComponent<TagComponent>(name);
        tag.Tag = name.empty() ? "unnamed" : name;
        return entity;
    }

    void EntityFactory::DestroyEntity(const std::shared_ptr<Entity>& entity) noexcept
    {
        entity->Destroy();
    }

    std::shared_ptr<Entity> EntityFactory::Nullify() const noexcept
    {
        return EMPTYENTITY;
    }
}